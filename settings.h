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

// Modify this function to change how relative position of tracpad is mapped to screen coordinates
static inline MTPoint map(double normx, double normy) {
    // whole trackpad to whole screen
    MTPoint point = {
        normx * screenSize.x, normy * screenSize.y
    };
    return point;
}
