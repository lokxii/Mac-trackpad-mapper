#include <pthread.h>
#include <Carbon/Carbon.h>
#include <CoreGraphics/CoreGraphics.h>
#include "MultitouchSupport.h"
#include "settings.h"

#define try(...) \
    if ((__VA_ARGS__) == -1) { \
        printf("`%s` failed", #__VA_ARGS__); \
        exit(1); \
    }

MTPoint fingerPosition = { 0, 0 };
MTPoint oldFingerPosition = { 0, 0 };
int32_t fingerID = 0;

// emitting mouse events here increases sensitivity to finger
int trackpadCallback(
    MTDeviceRef device,
    MTTouch *data,
    size_t nFingers,
    double timestamp,
    size_t frame)
{
    if (nFingers > 0) {
        // remembers currently using which finger
        MTTouch *f = &data[0];
        for (int i = 0; i < nFingers; i++){
            if (data[i].fingerID == fingerID) {
                f = &data[i];
                break;
            }
        }
        fingerID = f->fingerID;

        oldFingerPosition = fingerPosition;
        fingerPosition = map(
                f->normalizedVector.position.x, 1 - f->normalizedVector.position.y);

        // emit event
        CGPoint point = {
            .x = fingerPosition.x,
            .y = fingerPosition.y,
        };
        CGPoint delta = {
            .x = fingerPosition.x - oldFingerPosition.x,
            .y = fingerPosition.y - oldFingerPosition.y,
        };
        CGWarpMouseCursorPosition(point);
    }
    return 0;
}

bool check_privileges(void) {
    bool result;
    const void *keys[] = { kAXTrustedCheckOptionPrompt };
    const void *values[] = { kCFBooleanTrue };

    CFDictionaryRef options;
    options = CFDictionaryCreate(
            kCFAllocatorDefault,
            keys, values, sizeof(keys) / sizeof(*keys),
            &kCFCopyStringDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);

    result = AXIsProcessTrustedWithOptions(options);
    CFRelease(options);

    return result;
}

int main(int argc, char** argv) {
    // if (!check_privileges()) {
    //     printf("Requires accessbility privileges\n");
    //     return 1;
    // }
    
    // start trackpad service
    MTDeviceRef dev = MTDeviceCreateDefault();
    MTRegisterContactFrameCallback(dev, (MTFrameCallbackFunction)trackpadCallback);
    MTDeviceStart(dev, 0);

    // simply an infinite loop waiting for app to quit
    CFRunLoopRun();
    return 0;
}
