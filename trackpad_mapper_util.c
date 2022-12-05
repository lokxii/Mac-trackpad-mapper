#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <CoreFoundation/CoreFoundation.h>
#include "MultitouchSupport.h"
#include <Carbon/Carbon.h>
#include "settings.h"

#define try(...) \
    if ((__VA_ARGS__) == -1) { \
        printf("`%s` failed", #__VA_ARGS__); \
        exit(1); \
    }

pthread_mutex_t mutex;
MTPoint fingerPosition = { 0, 0 };

CGEventSourceRef eventSource;

#define MAGIC_SELF_EVENT 12345

int trackpadCallback(
    MTDeviceRef device,
    MTTouch *data,
    size_t nFingers,
    double timestamp,
    size_t frame)
{
    // only get the first finger
    if (nFingers > 0) {
        MTTouch *f = &data[0];

        // update position
        try(pthread_mutex_lock(&mutex));

        fingerPosition = map(f->normalizedVector.position.x, 1 - f->normalizedVector.position.y);

        try(pthread_mutex_unlock(&mutex));
    }
    return 0;
}

// This callback is also called when Quartz recieve the mouseMoved event
// posted by this callback. Then recusion will be indrectly formed.
// Posting event is expensive thus recursion should be avoided.
// Append a magic number to the mouseMoved event and prevent this callback
// from reacting to our own mouseMoved events.
CGEventRef updateCursor(
    CGEventTapProxy proxy,
    CGEventType type,
    CGEventRef event,
    void *reference)
{
    // Retrieve magic number
    int magic = CGEventGetIntegerValueField(event, kCGEventSourceUserData);
    if (magic == MAGIC_SELF_EVENT) {
        return NULL;
    }
    
    int id = CGEventGetIntegerValueField(event, kCGMouseEventNumber);

    // unlock mutex as early as possible
    try(pthread_mutex_lock(&mutex));
    MTPoint position = fingerPosition;
    try(pthread_mutex_unlock(&mutex));

    // generate event
    CGPoint point = (CGPoint){
        .x = (double)position.x,
        .y = (double)position.y,
    };
    
    CGEventRef newEvent;
    
    switch (type) {
        // Move cursor before clicking
        case kCGEventRightMouseDown:
        case kCGEventRightMouseUp:
        case kCGEventLeftMouseDown:
        case kCGEventLeftMouseUp: {
            newEvent = CGEventCreateMouseEvent(
                eventSource,
                kCGEventMouseMoved,
                point,
                kCGMouseButtonLeft);
            break;
        }
        
        default: {
            CGMouseButton button = type == kCGEventRightMouseDragged ?
                kCGMouseButtonRight :
                kCGMouseButtonLeft;
            newEvent = CGEventCreateMouseEvent(
                eventSource,
                type,
                point,
                button);
            break;
        }
    }

    // Append magic number
    CGEventSetIntegerValueField(newEvent, kCGEventSourceUserData, MAGIC_SELF_EVENT);
    CGEventSetIntegerValueField(newEvent, kCGMouseEventNumber, id);
    CGEventPost(kCGHIDEventTap, newEvent);

    CGEventSetIntegerValueField(event, kCGEventSourceUserData, MAGIC_SELF_EVENT);
    return event;
}

bool check_privileges(void) {
    bool result;
    const void *keys[] = { kAXTrustedCheckOptionPrompt };
    const void *values[] = { kCFBooleanTrue };

    CFDictionaryRef options;
    options = CFDictionaryCreate(kCFAllocatorDefault,
                                 keys, values, sizeof(keys) / sizeof(*keys),
                                 &kCFCopyStringDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);

    result = AXIsProcessTrustedWithOptions(options);
    CFRelease(options);

    return result;
}

int main(int argc, char** argv) {
    if (!check_privileges()) {
        printf("Requires accessbility privileges\n");
        return 1;
    }
    // init mutex
    try(pthread_mutex_init(&mutex, NULL));
    
    // start trackpad service
    MTDeviceRef dev = MTDeviceCreateDefault();
    MTRegisterContactFrameCallback(dev, (MTFrameCallbackFunction)trackpadCallback);
    MTDeviceStart(dev, 0);
    
    // CGEventTap loop
    eventSource = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

    // cursor movement
    CGEventMask mask = 1 << kCGEventMouseMoved |
                       1 << kCGEventLeftMouseDragged |
                       1 << kCGEventRightMouseDragged |
                       1 << kCGEventRightMouseDown |
                       1 << kCGEventRightMouseUp |
                       1 << kCGEventLeftMouseDown |
                       1 << kCGEventLeftMouseUp;
    CFMachPortRef handle = CGEventTapCreate(
        kCGHIDEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        mask,
        updateCursor,
        NULL);
    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, handle, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopCommonModes);
    
    CFRunLoopRun();
    return 0;
}
