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

CFRunLoopSourceRef runLoopSource;
CGEventSourceRef eventSource;

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

CGEventRef updateCursor(
    CGEventTapProxy proxy,
    CGEventType type,
    CGEventRef event,
    void *reference)
{
    // This callback is called when Quartz recieve the mouseMoved event
    // posted by this callback. Then recusion will be indrectly formed.
    // Posting event is expensive thus recursion should be avoided.
    // Append a magic number to the mouseMoved event and prevent this callback
    // from reacting to our own mouseMoved events.
    
    // Retrieve magic number
    int id = CGEventGetIntegerValueField(event, kCGEventSourceUserData);
    if (id == 12345) {
        return NULL;
    }

    // unlock mutex as early as possible
    try(pthread_mutex_lock(&mutex));
    MTPoint position = fingerPosition;
    try(pthread_mutex_unlock(&mutex));

    // printf("%.1f %.1f\n", position.x, position.y);
    // generate event
    CGPoint point = (CGPoint){
        .x = (double)position.x,
        .y = (double)position.y,
    };
    
    // Move mouse before click
    switch (type) {
        case kCGEventRightMouseDown:
        case kCGEventLeftMouseDown: {
            CGMouseButton button = type == kCGEventLeftMouseDown ?
                kCGMouseButtonLeft :
                kCGMouseButtonRight;
            event = CGEventCreateMouseEvent(
                eventSource,
                kCGEventMouseMoved,
                point,
                button);
            // Append magic number
            CGEventSetIntegerValueField(event, kCGEventSourceUserData, 12345);
            CGEventPost(kCGHIDEventTap, event);
        }

        default: {
            CGMouseButton button = type == kCGEventRightMouseDragged ?
                kCGMouseButtonRight :
                kCGMouseButtonLeft;
            event = CGEventCreateMouseEvent(
                eventSource,
                type,
                point,
                button);
            // Append magic number
            CGEventSetIntegerValueField(event, kCGEventSourceUserData, 12345);
            CGEventPost(kCGHIDEventTap, event);
            break;
        }
    }

    return NULL;
}

int main(int argc, char** argv) {
    // init mutex
    try(pthread_mutex_init(&mutex, NULL));
    
    // start trackpad service
    MTDeviceRef dev = MTDeviceCreateDefault();
    MTRegisterContactFrameCallback(dev, (MTFrameCallbackFunction)trackpadCallback);
    MTDeviceStart(dev, 0);
    
    // CGEventTap loop
    eventSource = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

    CGEventMask mask = 1 << kCGEventMouseMoved |
                       1 << kCGEventLeftMouseDragged |
                       1 << kCGEventRightMouseDragged |
                       1 << kCGEventLeftMouseDown |
                       1 << kCGEventRightMouseDown;
    CFMachPortRef handle = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionListenOnly, // You may want to use kCGEventTapOptionDefault depending on apps
        mask,
        updateCursor,
        NULL);
    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, handle, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopCommonModes);
    CFRunLoopRun();
    return 0;
}
