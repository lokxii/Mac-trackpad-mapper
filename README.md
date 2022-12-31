# Trackpad mapper for Mac

This utility maps finger position on trackpad to curosr coordinate on screen.

- [x] Lightweight
- [x] Does not create window
- [x] Highly configurable
- [x] Status bar app for easy toggling absolute tracking

I personally use this to play Osu! (& Osu!lazer) on Mac with trackpad.

## Usage

Open the app, there is a trackpad icon shown on the status bar. By default,
mapping was disabled. To toggle mapping, click the icon and choose 'Start
absolute tracking' or 'Stop absolute tracking'

## Settings

In the preference window, there are three items:

1. - [ ] Use settings in header file (settings.h)
2. Trackpad region: [Region]
3. Screen region: [Region]
4. - [ ] Emit mouse events
5. Tapping keys; [Keys]

If you want to use your custom code to map coordinates, enable the first item.
(See Modifying rule in header file)

The syntax of [Region] is `lowx,lowy,upx,upy`, where all numbers are floats in
range 0 to 1 inclusively. These numbers are passed as command line arguments to
`trackpad_mapper_util`. Click `Apply` to update the settings and remember to
restart absolute tracking.

Emit mouse events option can also be set in `settings.h`. See
[Emitting Mouse Events](#emitting-mouse-events)

Tapping keys can only be set here. [Keys] expects a 2 characters string. The
first key press is simulated when only one finger is down. The second key press
is simulated when the second finger is down while the first finger already touches
the trackpad. An empty string in [Keys] means disabling the option.

## Modifying rule in header file

The default settings are stored in `settings.def.h`. You may want to make a
copy of it and rename it to `settings.h`. This is where all local settings are
stored.

### Mapping

There is a function `map` in `settings.h` that converts normalized finger
position on trackpad to absolute coordinate of cursor on Screen.
`MTPoint map(double, double)` must be provided in `settings.h` as it will be
called in `trackpad_mapper_util.c`.

```C
MTPoint map(double normx, double normy) {
    // whole trackpad to whole screen
    MTPoint point = {
        normx * screenSize.width, normy * screenSize.height
    };
    return point;
}
```

Use the function `rangeRatio()` to map custom arrangments.

```C
.x = rangeRatio(normx, lowx, highx),
.y = rangeRatio(normy, lowy, highy),
```

Set the `lowx` & `highx` to a number between 0 & 1, much like a percentage. To map the middle of the trackpad to the whole screen (for x dimension), set `lowx` to `.25` & `highx` to `.75`.  

Example code to map to top right quarter of trackpad to whole screen

```C
MTPoint map(double normx, double normy) {
    // top right quarter of the area of trackpad to whole screen
    MTPoint point = {
            //the right half (.5 to 1) of the trackpad
            .x = rangeRatio(normx, .5, 1),
            //the top half (0 to 0.5) of the trackpad
            .y = rangeRatio(normy, 0, .5),
        };
    point.x *= screenSize.width;
    point.y *= screenSize.height;
    return point;
}
```

Remember to rebuild the util everytime you changed `map`. See [Rebuilding Util](#rebuilding-util)

### Screen Size

`extern CGSize screenSize` declared in `settings.def.h` will be initialized in
the main program. Do not remove the declaration.

The main program makes sure the mapped cursor coordinate is within `screenSize`
to make the dock appear.

### Emitting mouse events

It is possible choose between emitting mouse event or wrapping cursor coordinate
(without mouse event). Some programs requires emitting mouse events but enabling
it will cause the cursor not to magnify. Set `bool emitMouseEvent = true` to
emit mouse event. The default value is `false`.

## Building

Building the app requires Xcode version >= 11.

To compile, run
```sh
make release
```

The app is located in `build/`.

You may want to copy the app to the `/Applications/` folder or run
```sh
make install
```
to do it for you.

## Rebuilding Util

After you modified `map`, run
```sh
make util_release install_util_update
```
This will compile and update the util binary in the app bundle inside `/Applications/`

## TODO List

- [x] Create an app that can toggle absolute tracking
- [ ] Better way to change mapping rule

## Reference
Where I got the MultitouchSupport.h header
https://gist.github.com/rmhsilva/61cc45587ed34707da34818a76476e11
