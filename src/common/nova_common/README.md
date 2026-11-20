# nova_common

**Status:** not started.

A shared library, not a node. Anything two or more packages need lives here once and is imported, instead of every package keeping its own copy. If you find yourself copying a function from another package, it probably belongs here.

What's planned so far, and which issue adds it:
- **Route maths** (`frenet.py`): converting between map x, y and route s, d, in both directions. Added by [#17](https://github.com/Nova-UTD/navigator_2026/issues/17); used by [#22](https://github.com/Nova-UTD/navigator_2026/issues/22), [#26](https://github.com/Nova-UTD/navigator_2026/issues/26), [#29](https://github.com/Nova-UTD/navigator_2026/issues/29).
- **Heartbeat helper:** adds the 1 Hz `/system/heartbeat` publisher to any node in one line, for Python and C++. Added by [#15](https://github.com/Nova-UTD/navigator_2026/issues/15); every node uses it.

Old code worth reading for these:
- **Route coordinates:** [`frenet_utils.py`, lines 1–91](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/navigator_lane_change/frenet_utils.py#L1-L91). Only projects one way; we need the full x, y ↔ s, d round trip.
- **Data types:** [`types.py`, lines 7–87](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/navigator_lane_change/types.py#L7-L87).
- **Geometry helpers:** [`lateral_controller.py`, lines 246–302](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/lateral_controller.py#L246-L302) (point-to-segment distance, quaternion → yaw).
