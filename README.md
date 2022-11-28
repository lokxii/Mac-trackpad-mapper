# Trackpad mapper for Mac

This utility maps finger position on trackpad to curosr coordinate on screen.

[x] Lightweight
[x] Does not create window
[x] Highly configurable

## Modifying rule
There is a function `map` in main.c that converts normalized finger position on
trackpad to absolute coordinate of cursor on Screen.

```C
MTPoint map(double normx, double normy) {
    // whole trackpad to whole screen
    MTPoint point = {
        normx * screenSize.x, normy * screenSize.y
    };
    return point;
}
```

Example code to map to top right quarter of trackpad to whole screen

```C
MTPoint map(double normx, double normy) {
    // top right quarter of the area of trackpad to whole screen
    MTPoint point = {
        .x = normx >= 0.5 ? ((normx - 0.5) / 0.5) : 0,
        .y = normy <= 0.5 ? (normy / 0.5) : 1,
    };
    point.x *= screenSize.x;
    point.y *= screenSize.y;
    return point;
}
```

## Screen Size
This code does not detect screen size. Instead there is a global constant that keeps
track of screen size

```C
const MTPoint screenSize = { .x = 1440, .y = 900 };
```

Modifying this constant changes screen size.
