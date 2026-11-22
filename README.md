# AniVis for Daz Studio 6
Plugin to animate node visibility in Daz Studio 6.

This is a port of [fjdiazt/AniVis](https://github.com/fjdiazt/AniVis) (DAZ Studio 4.5+, Qt 4) to Daz Studio 6 (Qt 6.10, SDK 6). It also fixes a bug in the original code where the watcher's signal connections pointed at slot names that no longer existed, so the Visible property was never updated.

Current version is "beta" code not "production".

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
Select one or more nodes in the scene.

Click the menu item Edit / Add AnaVis Property.

Select a node and move in the Timeline to the frame where you want it to disappear.

Locate the Animate Visibility property under Parameters/Visibility and toggle it to Off.

Move the prior frame and toggle it to On.

Note that you can also toggle back and forth in multiple frames e.g. to blink.

Also note that nodes do not need to be top-level, e.g. you can make a character's head disappear... 

## Trademarks
DAZ Studio is a trademark of DAZ 3D, Inc.

Visual Studio and Windows are registered trademarks of Microsoft Corp.
