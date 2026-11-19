# trajectory_follower

**Status:** not started. Most of it already exists in the old ACC.

## What it does
Drives the car along the path the planner picked. It turns the steering wheel to stay on the path and uses the throttle and brake to match the planned speed. It doesn't decide anything: where to go and how fast come from `trajectory_planner`.

## Topics
| | Topic | What's on it |
|---|---|---|
| Listens to | `/planning/trajectory` | The path to drive, with a speed for every point |
| Listens to | `/world/ego` | Where we are and how fast we're going |
| Publishes | `/control/command` | Steering, throttle and brake, 50 times a second. `safety_monitor` checks it before it reaches the car |

## How it works
1. **Steering:** pure pursuit. Pick a point on the path a little ahead of the car and steer towards it. The faster we go, the further ahead the point.
2. **Speed:** look up the planned speed 0.3 s ahead on the trajectory. A PID controller turns the speed error into an acceleration, which is limited and then turned into throttle or brake.
3. **No fresh trajectory?** If the newest trajectory is more than 0.3 s old, brake to a stop.

## Settings to start with
- Lookahead: 4.0 m + 0.3 × speed, at most 15 m. **Keep 4.0 m:** 6.0 m made the car cut right turns onto the sidewalk in Town02.
- Max steering: 1.0
- PID gains: kp 1.2, ki 0.15, kd 0.05
- Max acceleration 2.5 m/s², max braking 3.5 m/s²

## Done when
It follows a trajectory through the Town02 turns within 0.3 m sideways.

## Old code you can reuse
> **None of this code is in this repo.** These links go to old code from before the restructure, saved in git history at commit `86a6bde`. Most of it never ran as part of the driving stack: some of it was never launched, and some was launched but never got the inputs it needed. Read it for ideas, logic and tuned numbers, then write this package fresh in the new layout. Copied files won't work as they are: they use old topic names, old message types and old import paths.
>
> To read the code without a browser: `git show 86a6bde:<path>`.

- **Pure pursuit steering:** [`lateral_controller.py`, lines 19–302](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/lateral_controller.py#L19-L302). Port this almost unchanged. The only real change is the input: it reads a `nav_msgs/Path` ([lines 59–66](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/lateral_controller.py#L59-L66)), and the new version takes the trajectory's x, y points.
- **PID, limits and pedal mapping:** [`longitudinal_controller.py`, lines 235–346](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/longitudinal_controller.py#L235-L346). These are `_compute_pid_acceleration`, `_apply_jerk_limit`, `_apply_acceleration_limits` and `_acceleration_to_throttle_brake`. Port them.
- **The control loop:** [`autonomous_cruise_node.py`, lines 421–467](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/autonomous_cruise_node.py#L421-L467). Shows how the ACC checked for a stale path ([lines 454–457](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/autonomous_cruise_node.py#L454-L457)) and called the two controllers each tick. Use it as a pattern.
- **Publishing the command:** [`autonomous_cruise_node.py`, lines 570–613](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/autonomous_cruise_node.py#L570-L613). How `VehicleControl` was filled in, plus the stop and zero-control helpers.
- **Tests:** [`test_autonomous_cruise.py`](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/test/test_autonomous_cruise.py). A starting point. **4 of these already fail on the old `dev`** (lookahead, offset steering, lead detection, adaptive speed), so fix or drop them.
- **Parameters:** [`param/autonomous_cruise_params.yaml`](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/param/autonomous_cruise_params.yaml) (still in this repo's `param/` folder).

## Old code to skip
- **Following the car ahead:** [`longitudinal_controller.py`, lines 134–233](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/longitudinal_controller.py#L134-L233). This moves to `trajectory_planner`.
- **LiDAR braking** in the ACC node. This moves to `safety_monitor`.
- The old controllers `pure_pursuit_controller`, `unified_controller` and `pid_controller`. The ACC's versions above replaced them.
