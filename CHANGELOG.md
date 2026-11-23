# Changelog

## 2.4.0
- **Key Visibility When Toggled** checkbox in the AniVis Timeline pane, next to All items. It is the same setting as Edit > AniVis > Key Visibility When Toggled; the two stay in step.

## 2.3.2
- Much lighter scrubbing: hiding or showing an item takes Daz 50-200 ms (viewport and Scene pane updates), and dragging a time slider used to do that for every frame passed. While you drag, AniVis now updates visibility about 10 times a second and exactly where you stop. Playback, stepping frames, renders and scripts still update every frame.

## 2.3.1
- AniVis Timeline is frame-accurate: the current-frame line, key diamonds, ticks and frame numbers all sit on the middle of their frame (the line used to be half a frame off the keys).
- Each frame of the bars shows exactly the visibility AniVis applies on that frame; a key between two frames shows on the frame where it takes effect.
- The pane shows the play range, like Daz's Timeline, and follows changes to the play range and frame rate.

## 2.3.0
- New **AniVis Timeline** pane (Window > Panes (Tabs) > AniVis Timeline): a frame ruler and one row per item, showing when it is visible (green) or hidden (red), its keys and the current frame. Click or drag to move the timeline, click Now to toggle at the current frame. Dock it next to the Timeline.
- Fixed Daz Studio crashing on close, and then on start, after using the AniVis tab of 2.2 (that tab is replaced by the pane).

## 2.2.0
- Animate Visibility is added when, and only when, you toggle an item's visibility (eye icon); the toggle is keyed at the current frame.
- Edit > AniVis > Key Visibility When Toggled turns that on or off.

## 2.1.0
- Edit > AniVis menu: Add/Remove Property, Key Visible/Hidden Here, Clear Keys, Select Animated Items.
- Removed properties stay removed after saving and reopening.
- Scenes reopen correctly: Daz saves every value as one key, so only two or more keys count as animation.

## 2.0.0
- Port of AniVis to Daz Studio 6 (Qt 6, SDK 6).
- Each key holds until the next one (Daz rounds on/off keys down between keys).
- Fixed the original plugin never updating visibility (its signal connections pointed at renamed slots).
