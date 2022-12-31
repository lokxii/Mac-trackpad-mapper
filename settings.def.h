#pragma once

// Do not remove this. This variable is initialized in main program
extern CGSize screenSize;
// Emit mouse event or wrap cursor. Default is warp cursor
bool emitMouseEvent = false;

// Helper function: lower and upper has to be without 0 to 1 inclusive
static inline double rangeRatio(double n, double lower, double upper) {
    if (n < lower || n > upper) {
        return -1;
    }
    return (n - lower) / (upper - lower);
}

// Compulsory: Modify this function to change how relative position of trackpad is mapped to normalized screen coordinates. Return negative number for invalid finger position
static inline MTPoint map(double normx, double normy) {
    // whole trackpad to whole screen
    MTPoint point = {
        .x = normx,
        .y = normy,
    };
    //scaling the points up to the screen size
    point.x *= screenSize.width;
    point.y *= screenSize.height;
    return point;
}
