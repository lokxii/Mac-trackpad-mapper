#include <pthread.h>
#include <Carbon/Carbon.h>
#include <CoreGraphics/CoreGraphics.h>
#include "MultitouchSupport.h"
#include "../settings.h"

#define try(...) \
    if ((__VA_ARGS__) == -1) { \
        fprintf(stderr, "`%s` failed", #__VA_ARGS__); \
        exit(1); \
    }

#define tapWithMouse 0

typedef struct {
    float lowx, lowy, upx, upy;
} Range;

typedef struct {
    bool useArg;
    Range trackpadRange;
    Range screenRange;
    bool emitMouseEvent;
    bool tapping;
    CGKeyCode keys[2];
} Settings;

Settings settings = {
    .useArg = false,
    .trackpadRange = { 0, 0, 1, 1 },
    .screenRange = { 0, 0, 1, 1, },
    .emitMouseEvent = false,
    .tapping = false,
    .keys = { kVK_ANSI_Z, kVK_ANSI_X },
};
CGSize screenSize;

int mouseEventNumber = 0;
pthread_mutex_t mouseEventNumber_mutex;
#define MAGIC_NUMBER 12345

double _rangeRatio(double n, double lower, double upper) {
    if (n < lower || n > upper) {
        return -1;
    }
    return (n - lower) / (upper - lower);
}

double _reverseRangeRatio(double n, double lower, double upper) {
    if (n < 0) {
        return n;
    }
    return n * (upper - lower) + lower;
}

MTPoint _map(double normx, double normy) {
    MTPoint point = {
        .x = _rangeRatio(
            normx, settings.trackpadRange.lowx, settings.trackpadRange.upx),
        .y = _rangeRatio(
            normy, settings.trackpadRange.lowy, settings.trackpadRange.upy),
    };

    point.x = _reverseRangeRatio(
            point.x, settings.screenRange.lowx, settings.screenRange.upx);
    point.y = _reverseRangeRatio(
            point.y, settings.screenRange.lowy, settings.screenRange.upy);

    point.x *= screenSize.width;
    point.y *= screenSize.height;
    return point;
}

bool inMapRange(MTPoint point, MTPoint* fingerPosition) {
    // use settings.h if no command line arguments are given
    MTPoint p = (settings.useArg ? _map : map)(point.x, 1 - point.y);
    if (fingerPosition) {
        *fingerPosition = p;
    }
    return p.x < 0 || p.y < 0;
}

void moveCursor(double x, double y) {
    CGPoint point = {
        .x = x < 0 ? 0 : x >= screenSize.width ? screenSize.width - 1 : x,
        .y = y < 0 ? 0 : y >= screenSize.height ? screenSize.height - 1: y,
    };

    if (settings.useArg && settings.emitMouseEvent ||
        !settings.useArg && emitMouseEvent)
    {
        CGEventRef event = CGEventCreateMouseEvent(
            NULL,
            kCGEventMouseMoved,
            point,
            kCGMouseButtonLeft);
        CGEventSetIntegerValueField(event, kCGEventSourceUserData, MAGIC_NUMBER);
        CGEventSetIntegerValueField(event, kCGMouseEventSubtype, 3);

        try(pthread_mutex_lock(&mouseEventNumber_mutex));
        CGEventSetIntegerValueField(event, kCGMouseEventNumber, mouseEventNumber);
        try(pthread_mutex_unlock(&mouseEventNumber_mutex));

        CGEventPost(kCGHIDEventTap, event);
    } else {
        CGWarpMouseCursorPosition(point);
    }
}

void keyEvent(bool down, CGPoint point) {
#if tapWithMouse
    CGEventType type = down ? kCGEventLeftMouseDown : kCGEventLeftMouseUp;
    CGEventRef event = CGEventCreateMouseEvent(
        NULL, type, point, 0);
    CGEventSetIntegerValueField(event, kCGEventSourceUserData, MAGIC_NUMBER);
#else
    static int keynum = 0;
    keynum = (keynum + down) % 2;
    CGEventRef event = CGEventCreateKeyboardEvent(NULL, settings.keys[keynum], down);
#endif
    CGEventPost(kCGHIDEventTap, event);
}

void generateClick(MTTouch* data, size_t nFingers, MTPoint fingerPosition) {
    static int path = -1;
    if (!settings.tapping) {
        return;
    }
    
    CGPoint point = { .x = fingerPosition.x, .y = fingerPosition.y };
    // first tapping
    if (path == -1) {
        for (int i = 0; i < nFingers; i++) {
            if (data[i].state == MTTouchStateMakeTouch && inMapRange(data[i].normalizedVector.position, NULL)) {
                keyEvent(true, point);
                path = data[i].pathIndex;
            }
        }
    }
    // find if original finger lifted
    for (int i = 0; i < nFingers; i++) {
        if (data[i].pathIndex == path && data[i].state == MTTouchStateOutOfRange) {
            keyEvent(false, point);
            path = -1;
        }
    }
    // find if other fingers down
    for (int i = 0; i < nFingers; i++) {
        if (data[i].state == MTTouchStateMakeTouch && data[i].pathIndex != path && inMapRange(data[i].normalizedVector.position, NULL)) {
            // usleep(5000);
            keyEvent(false, point);
            usleep(5000);
            keyEvent(true, point);
            path = data[i].pathIndex;
        }
    }
    // ensure finger not touching is lifted
    if (path == -1) {
        return;
    }
    bool found = false;
    for (int i = 0; i < nFingers; i++) {
        if (data[i].pathIndex == path) {
            found = true;
            break;
        }
    }
    if (!found) {
        keyEvent(false, point);
        path = -1;
    }
}

MTPoint getMouseLocation() {
    CGEventRef ourEvent = CGEventCreate(NULL);
    CGPoint point = CGEventGetLocation(ourEvent);
    CFRelease(ourEvent);
    return (MTPoint){
        point.x, point.y
    };
}

// moving cursor here increases sensitivity to finger
// detecting gesture:
// Beginning of a gesture may start with one finger or more than one fingers.
// Simply checking how many fingers touched is not enough.
// Discard coordinates of the first callback call for each cursor movement
// and wait for the second call to make sure it is not a gesture.
int trackpadCallback(
    MTDeviceRef device,
    MTTouch *data,
    size_t nFingers,
    double timestamp,
    size_t frame)
{
    #define GESTURE_PHASE_NONE 0
    #define GESTURE_PHASE_MAYSTART 1
    #define GESTURE_PHASE_BEGAN 2
    #define GESTURE_TIMEOUT 0.02

    static MTPoint fingerPosition = { NAN, NAN },
                   oldFingerPosition = { NAN, NAN };
    static int32_t oldPathIndex = -1;
    static double oldTimeStamp = 0,
                  startTrackTimeStamp = 0;
    static size_t oldFingerCount = 1;
    static int gesturePhase = GESTURE_PHASE_NONE;
    // FIXME: how many fingers can magic trackpad detect?
    static bool gesturePaths[20] = { 0 };
    
    // initialization
    if (isnan(fingerPosition.x)) {
        fingerPosition = oldFingerPosition = getMouseLocation();
    }
    
    if (nFingers == 0) {
        // all fingers lifted, clearing gesture fingers
        for (int i = 0; i < 20; i++) {
            gesturePaths[i] = false;
        }
        gesturePhase = GESTURE_PHASE_NONE;
        oldFingerCount = nFingers;
        startTrackTimeStamp = 0;
        generateClick(data, nFingers, fingerPosition);
        return 0;
    }

    generateClick(data, nFingers, fingerPosition);
    
    if (!startTrackTimeStamp) {
        startTrackTimeStamp = timestamp;
    }
    
    if (oldFingerCount != 1 && nFingers == 1 && !gesturePhase) {
        gesturePhase = GESTURE_PHASE_MAYSTART;
        oldFingerCount = nFingers;
        return 0;
    };
    
    if (nFingers == 1 && timestamp - startTrackTimeStamp < GESTURE_TIMEOUT) {
        return 0;
    }
    
    if (nFingers != 1 && (
        timestamp - startTrackTimeStamp < GESTURE_TIMEOUT ||
        gesturePhase != GESTURE_PHASE_NONE))
    {
        gesturePhase = GESTURE_PHASE_BEGAN;
        for (int i = 0; i < nFingers; i++) {
            gesturePaths[data[i].pathIndex] = true;
        }
        moveCursor(fingerPosition.x, fingerPosition.y);
        oldFingerCount = nFingers;
        return 0;
    };
    
    // keeping one finger on trackpad when lifting up fingers
    // at the end of gesture
    if (gesturePhase == GESTURE_PHASE_BEGAN) {
        for (int i = 0; i < nFingers; i++) {
            if (gesturePaths[data[i].pathIndex]) {
                moveCursor(fingerPosition.x, fingerPosition.y);
                return 0;
            }
        }
    }
    
    gesturePhase = GESTURE_PHASE_NONE;

    // remembers currently using which finger
    MTTouch *f = &data[0];
    for (int i = 0; i < nFingers; i++){
        if (data[i].pathIndex == oldPathIndex) {
            f = &data[i];
            break;
        }
    }

    oldFingerPosition = fingerPosition;
    MTPoint velocity = f->normalizedVector.velocity;
    
    if (inMapRange(f->normalizedVector.position, &fingerPosition)) {
        // Only lock cursor when finger starts path on dead zone
        if (f->pathIndex == oldPathIndex) {
            if (fingerPosition.x < 0) {
                fingerPosition.x = oldFingerPosition.x +
                    velocity.x * (timestamp - oldTimeStamp) * 1000;
            }
            if (fingerPosition.y < 0) {
                fingerPosition.y = oldFingerPosition.y -
                    velocity.y * (timestamp - oldTimeStamp) * 1000;
            }

        } else {
            fingerPosition = oldFingerPosition;
        }
    } else {
        oldPathIndex = f->pathIndex;
    }
    
    moveCursor(fingerPosition.x, fingerPosition.y);

    oldTimeStamp = timestamp;
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

Range parseRange(char* s) {
    char* token[4 + 1];
    int i = 0;
    for (;(token[i] = strsep(&s, ",")) != NULL && i < 4; i++) { }
    if (i != 4 || token[4] != NULL) {
        fputs("Range format: lowx,lowy,upx,upy and numbers in range [0, 1]", stderr);
        exit(1);
    }
    float num[4];
    for (i = 0; i < 4; i++){
        char* endptr;
        num[i] = strtof(token[i], &endptr);
        if (*endptr) {
            fprintf(stderr, "Invalid number %s\n", token[i]);
            exit(1);
        }
    }
    return (Range) {
        num[0], num[1], num[2], num[3]
    };
}

// from https://stackoverflow.com/a/1971027
/* Returns string representation of key, if it is printable.
 * Ownership follows the Create Rule; that is, it is the caller's
 * responsibility to release the returned object. */
CFStringRef createStringForKey(CGKeyCode keyCode)
{
    TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();
    CFDataRef layoutData =
        TISGetInputSourceProperty(currentKeyboard,
                                  kTISPropertyUnicodeKeyLayoutData);
    const UCKeyboardLayout *keyboardLayout =
        (const UCKeyboardLayout *)CFDataGetBytePtr(layoutData);

    UInt32 keysDown = 0;
    UniChar chars[4];
    UniCharCount realLength;

    UCKeyTranslate(keyboardLayout,
                   keyCode,
                   kUCKeyActionDisplay,
                   0,
                   LMGetKbdType(),
                   kUCKeyTranslateNoDeadKeysBit,
                   &keysDown,
                   sizeof(chars) / sizeof(chars[0]),
                   &realLength,
                   chars);
    CFRelease(currentKeyboard);    

    return CFStringCreateWithCharacters(kCFAllocatorDefault, chars, 1);
}

/* Returns key code for given character via the above function, or UINT16_MAX
 * on error. */
CGKeyCode keyCodeForChar(const char c)
{
    static CFMutableDictionaryRef charToCodeDict = NULL;
    CGKeyCode code;
    UniChar character = c;
    CFStringRef charStr = NULL;

    /* Generate table of keycodes and characters. */
    if (charToCodeDict == NULL) {
        size_t i;
        charToCodeDict = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                   128,
                                                   &kCFCopyStringDictionaryKeyCallBacks,
                                                   NULL);
        if (charToCodeDict == NULL) return UINT16_MAX;

        /* Loop through every keycode (0 - 127) to find its current mapping. */
        for (i = 0; i < 128; ++i) {
            CFStringRef string = createStringForKey((CGKeyCode)i);
            if (string != NULL) {
                CFDictionaryAddValue(charToCodeDict, string, (const void *)i);
                CFRelease(string);
            }
        }
    }

    charStr = CFStringCreateWithCharacters(kCFAllocatorDefault, &character, 1);

    /* Our values may be NULL (0), so we need to use this function. */
    if (!CFDictionaryGetValueIfPresent(charToCodeDict, charStr,
                                       (const void **)&code)) {
        code = UINT16_MAX;
    }

    CFRelease(charStr);
    return code;
}

void parseTappingKey(CGKeyCode keys[2], char* s) {
    if (strlen(s) != 2) {
        fputs("Expects 2 character for -t", stderr);
        exit(1);
    }
    keys[0] = keyCodeForChar(s[0]);
    keys[1] = keyCodeForChar(s[1]);
    if (keys[0] == UINT16_MAX || keys[1] == UINT16_MAX) {
        fputs("Invalid key for -t", stderr);
        exit(1);
    }
}

void parseSettings(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, "i:o:et:")) != -1) {
        switch (opt) {
            case 'i':
                settings.trackpadRange = parseRange(optarg);
                settings.useArg = true;
                break;
            case 'o':
                settings.screenRange = parseRange(optarg);
                settings.useArg = true;
                break;
            case 'e':
                settings.emitMouseEvent = true;
                settings.useArg = true;
                break;
            case 't':
                settings.tapping = true;
                parseTappingKey(settings.keys, optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-i lowx,lowy,upx,upy] [-o lowx,lowy,upx,upy] [-t key] [-e]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

CGEventRef loggerCallback(
    CGEventTapProxy proxy,
    CGEventType type,
    CGEventRef event,
    void* context)
{
    int magic_number = CGEventGetIntegerValueField(event, kCGEventSourceUserData);
    if (magic_number == MAGIC_NUMBER) {
        return event;
    }
    switch (type) {
        case kCGEventMouseMoved: {
            int eventNumber = CGEventGetIntegerValueField(event, kCGMouseEventNumber);
            try(pthread_mutex_lock(&mouseEventNumber_mutex));
            mouseEventNumber = eventNumber;
            try(pthread_mutex_unlock(&mouseEventNumber_mutex));
            return event;
        }
        default:
            if (settings.tapping) {
                return NULL;
            } else {
                return event;
            }
    }
}

void hookMouseCallback() {
#if tapWithMouse
    CGEventMask mask = 1 << kCGEventMouseMoved |
                       1 << kCGEventLeftMouseDown |
                       1 << kCGEventLeftMouseUp |
                       1 << kCGEventRightMouseDown |
                       1 << kCGEventRightMouseUp;
#else
    CGEventMask mask = 1 << kCGEventMouseMoved;
#endif
    CFMachPortRef tap = CGEventTapCreate(
        kCGHIDEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        mask,
        loggerCallback,
        NULL);
    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(
        kCFAllocatorDefault, tap, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(tap, true);
}

int main(int argc, char** argv) {
    puts("start util");
    // if (!check_privileges()) {
    //     printf("Requires accessbility privileges\n");
    //     return 1;
    // }
    parseSettings(argc, argv);
    screenSize = CGDisplayBounds(CGMainDisplayID()).size;
    
    try(pthread_mutex_init(&mouseEventNumber_mutex, NULL));
    
    // start trackpad service
    MTDeviceRef dev = MTDeviceCreateDefault();
    MTRegisterContactFrameCallback(dev, (MTFrameCallbackFunction)trackpadCallback);
    MTDeviceStart(dev, 0);
    
    hookMouseCallback();

    // simply an infinite loop waiting for app to quit
    CFRunLoopRun();
    return 0;
}
