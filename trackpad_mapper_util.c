#include <pthread.h>
#include <Carbon/Carbon.h>
#include <CoreGraphics/CoreGraphics.h>
#include "MultitouchSupport.h"
#include "settings.h"

#define try(...) \
    if ((__VA_ARGS__) == -1) { \
        fprintf(stderr, "`%s` failed", #__VA_ARGS__); \
        exit(1); \
    }

typedef struct {
    float lowx, lowy, upx, upy;
} Range;

typedef struct {
    int width, height;
} Screensize;

typedef struct {
    bool useArg;
    Range trackpadRange;
    Range screenRange;
    Screensize screensize;
} Settings;

Settings settings;
MTPoint fingerPosition = { 0, 0 };
MTPoint oldFingerPosition = { 0, 0 };
int32_t fingerID = 0;

double _rangeRatio(double n, double lower, double upper) {
   return (n - lower) / (upper - lower);
}

double _reverseRangeRatio(double n, double lower, double upper) {
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

    point.x *= settings.screensize.width;
    point.y *= settings.screensize.height;
    return point;
}

// moving cursor here increases sensitivity to finger
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
        // use settings.h if no command line arguments are given
        fingerPosition = (settings.useArg ? _map : map)(
                f->normalizedVector.position.x, 1 - f->normalizedVector.position.y);

        CGPoint point = {
            .x = fingerPosition.x,
            .y = fingerPosition.y,
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

Range parseRange(char* s) {
    char* token[4];
    int i = 0;
    for (;(token[i] = strsep(&s, ",")) != NULL; i++) { }
    if (i != 4) {
        fputs("Range format: lowx,lowy,upx,upy", stderr);
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

Screensize parseScreensize(char* s) {
    puts(s);
    char* token[2];
    int i = 0;
    for (;(token[i] = strsep(&s, "x")) != NULL; i++) { }
    if (i != 2) {
        fputs("Size format: widthxheight", stderr);
        exit(1);
    }
    int num[2];
    for (i = 0; i < 2; i++){
        char* endptr;
        num[i] = strtof(token[i], &endptr);
        if (*endptr) {
            fprintf(stderr, "Invalid number %s\n", token[i]);
            exit(1);
        }
    }
    return (Screensize) {
        num[0], num[1]
    };
}

Settings parseSettings(int argc, char** argv) {
    // Default settings
    Settings settings = { false, { 0, 0, 1, 1 }, { 0, 0, 1, 1, }, { 1440, 900 } };
    int opt;
    while ((opt = getopt(argc, argv, "i:o:s:")) != -1) {
        switch (opt) {
            case 'i':
                settings.trackpadRange = parseRange(optarg);
                settings.useArg = true;
                break;
            case 'o':
                settings.screenRange = parseRange(optarg);
                settings.useArg = true;
                break;
            case 's':
                settings.screensize = parseScreensize(optarg);
                settings.useArg = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-i lowx,lowy,upx,upy] [-o lowx,lowy,upx,upy] [-s widthxheight]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    return settings;
}

int main(int argc, char** argv) {
    // if (!check_privileges()) {
    //     printf("Requires accessbility privileges\n");
    //     return 1;
    // }
    settings = parseSettings(argc, argv);
    
    // start trackpad service
    MTDeviceRef dev = MTDeviceCreateDefault();
    MTRegisterContactFrameCallback(dev, (MTFrameCallbackFunction)trackpadCallback);
    MTDeviceStart(dev, 0);

    // simply an infinite loop waiting for app to quit
    CFRunLoopRun();
    return 0;
}
