# light_state

**Status:** not started.

## What it does
Works out which traffic light controls **our** lane, and what colour it is. The camera detector sees every light in the image; this node picks ours using the map, and smooths out flicker.

## Topics
| | Topic | What's on it |
|---|---|---|
| Listens to | `/perception/camera/traffic_lights` | Every light the camera sees, with its colour |
| Listens to | `/map/road_ahead` | Where the lights that control our lane are, and their stop lines |
| Listens to | camera info and tf | To work out where a light should appear in the image |
| Publishes | `/world/lights` | Our light's colour, how long it's been that colour, and the stop line distance |

## How it works
1. For each light the map says controls our lane, work out where it should appear in the camera image.
2. Take the camera detections within 60 pixels of that spot.
3. Only change colour after 3 frames in a row agree, so one bad frame doesn't flip it.
4. If the light hasn't been seen for 1 s, report "unknown".

## Done when
It matches CARLA's true light colour in more than 99% of frames in the traffic-light scenarios.

## Old code you can reuse
> **None of this code is in this repo.** These links go to old code from before the restructure, saved in git history at commit `86a6bde`. Most of it never ran as part of the driving stack: some of it was never launched, and some was launched but never got the inputs it needed. Read it for ideas, logic and tuned numbers, then write this package fresh in the new layout. Copied files won't work as they are: they use old topic names, old message types and old import paths.
>
> To read the code without a browser: `git show 86a6bde:<path>`.

- **The detector it reads from:** `src/perception/traffic_light_detector/traffic_light_detector/traffic_light_detector.py` (in this repo), especially `image_callback` (lines 37–83). Shows what a detection contains.
- **How the old stack used light detections:** [`intersection_manager.py`, lines 77–79](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/intersection_manager/intersection_manager/intersection_manager.py#L77-L79) and [lines 131–161](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/intersection_manager/intersection_manager/intersection_manager.py#L131-L161).

Matching a light to a lane is new work: nothing in the old stack did it.
