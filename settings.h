#pragma once

// Modify this according to your screen resolution
const MTPoint screenSize = { .x = 1440, .y = 900 };

// lower and upper has to be without 0 to 1 inclusive
static inline double rangeRatio(double n, double lower, double upper) {
    if (n >= lower && n <= upper) {
        return (n - lower) / (upper - lower);
    } else if (n < lower) {
        return 0;
    } else {
        return 1;
    }
}

// Modify this function to change how relative position of trackpad is mapped to screen coordinates
static inline MTPoint map(double normx, double normy) {
    // whole trackpad to whole screen
    MTPoint point = {
        .x = normx,
        .y = normy,
    };
    //scaling the points up to the screen size
    point.x *= screenSize.x;
    point.y *= screenSize.y;
    return point;
}
