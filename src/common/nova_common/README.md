# nova_common

**Status:** not started.

## What it is
A shared library, not a node. Anything two or more packages need lives here once and is imported, instead of every package keeping its own copy:
- **Shared data types:** a lane, an object, a trajectory
- **Route maths:** converting between map coordinates (x, y) and route coordinates (s = distance along the lane, d = sideways offset from the lane centre)
- **Geometry helpers:** distances, angles, the car's footprint
- **Message conversions** for messages several nodes use

If you find yourself copying a function from another package, it probably belongs here.

## Old code you can reuse
> **None of this code is in this repo.** These links go to old code from before the restructure, saved in git history at commit `86a6bde`. Most of it never ran as part of the driving stack: some of it was never launched, and some was launched but never got the inputs it needed. Read it for ideas, logic and tuned numbers, then write this package fresh in the new layout. Copied files won't work as they are: they use old topic names, old message types and old import paths.
>
> To read the code without a browser: `git show 86a6bde:<path>`.

- **Route coordinates:** [`frenet_utils.py`, lines 1–91](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/navigator_lane_change/frenet_utils.py#L1-L91). Pure Python with no ROS: path direction at a point, and projecting objects relative to the car. A start, but it only projects one way; we need a full x, y ↔ s, d round trip.
- **Data types:** [`types.py`, lines 7–87](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/navigator_lane_change/types.py#L7-L87). Simple dataclasses for the car, tracked objects and the scene. A starting point for the shared types.
- **Geometry helpers:** [`lateral_controller.py`, lines 246–302](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/lateral_controller.py#L246-L302). Point-to-segment distance and quaternion → yaw.
