# AniVis for Daz Studio 6
Plugin to animate node visibility in Daz Studio 6.

This is a port of [fjdiazt/AniVis](https://github.com/fjdiazt/AniVis) (DAZ Studio 4.5+, Qt 4) to Daz Studio 6 (Qt 6.10, SDK 6). It also fixes a bug in the original code where the watcher's signal connections pointed at slot names that no longer existed, so the Visible property was never updated.

Version 2.1. Tested on Daz Studio 6.26 (Windows) with Genesis 9.

## Terms
Licensed under Mozilla Public License Version 2.0 (MPL).

Free for personal and commercial use. 

Any work products you create using AniVis are entirely your own so far as AniVis Contributors are concerned.

There is NO WARRANTY, USE AT YOUR OWN RISK.


## Supported Platform(s)
Currently available for the Windows 64-bit platform.

## To Install
Copy `dsp_anivis.dll` into the Daz Studio 6 plugins folder (e.g. `C:\Program Files\DAZ 3D\DAZStudio6\plugins`).

Restart Daz Studio if it was already running.

Open Help > About Installed Plugins and check that the list includes AniVis with "green" status.

## To Build (Optional)
Requirements:

- The Daz Studio 6 SDK.
- Qt 6.10 (`msvc2022_64`), the Qt version Daz Studio 6 uses.
- CMake 3.21+ and Visual Studio 2022 (toolset v143) or later.

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DDAZ_SDK_DIR="C:/path/to/Daz Studio 6 SDK" -DCMAKE_PREFIX_PATH="C:/Qt/6.10.3/msvc2022_64"
cmake --build build --config Release
```

The result is `build/Release/dsp_anivis.dll`. If MSBuild reports `FTK1011` errors, the build folder's path is too long: use a shorter `-B` folder.

Notes for the Daz Studio 6 SDK: Windows plugins must be named `dsp_*.dll`, `anivis.def` exports `getSDKVersion` and `getPluginDefinition` by their plain names, and `DzBase::makePersistent()` is obsolete.

## To Use
Every figure, piece of clothing, hair, prop and light you add to the scene automatically gets an **Animate Visibility** property (Parameters pane, under Display). Bones and cameras do not, so a Genesis figure gets one property per item, not one per bone.

To make something disappear and reappear:

1. Select it and move the timeline to the frame where it should disappear.
2. Choose **Edit > AniVis > Key Hidden Here** (or set Animate Visibility to Off in the Parameters pane).
3. Move to the frame where it should come back and choose **Key Visible Here**.

Each key holds until the next key: Off at frame 10 and On at frame 20 means hidden for frames 10-19. The first key also keeps what the item looked like before it, so keying "hidden" at frame 10 does not hide frames 0-9 as well.

Until Animate Visibility is keyed on two or more frames it does nothing, and the normal eye icon in the Scene pane works as usual. (Daz saves every animatable value as one key, so a single key counts as a plain value.)

### Menu: Edit > AniVis
The same items are in the Scene pane's right-click menu under Edit > AniVis.

| Item | What it does |
|---|---|
| Add Property | Adds Animate Visibility to the selected items, or brings back one you removed. |
| Remove Property | Deletes the keys, makes the item visible and hides the property. Auto-add will not add it back, also after saving and reopening the scene. |
| Key Visible Here / Key Hidden Here | Keys the selected items at the current frame. |
| Clear Keys | Deletes all AniVis keys on the selected items and makes them visible. |
| Select Animated Items | Selects every item whose visibility is animated. |
| Auto-Add to New Items | Turns auto-add on or off. The setting is remembered. Turning it on also adds the property to the items already in the scene. |

### Saving and loading
The property, its keys and a removed property's "removed" state are saved with the scene. They survive opening the scene, merging it into another scene, and loading the items as content. Scenes made with the original AniVis (same property name) keep working.

Items can be nested: you can also animate a single piece of clothing, or a prop parented to a figure.

### Limitations
- Add, Remove, Clear Keys and the key items cannot be undone with Edit > Undo yet.
- Hiding a figure does not hide its clothing; key the clothing too, or select the figure and its clothing together before keying.

## Trademarks
DAZ Studio is a trademark of DAZ 3D, Inc.

Visual Studio and Windows are registered trademarks of Microsoft Corp.
