# Changelog

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
