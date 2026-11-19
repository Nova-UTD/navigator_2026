# safety_monitor

**Status:** not started. The core of it is the old ACC's LiDAR braking.

## What it does
The last check before a command reaches the car. If the LiDAR sees something in the path we're about to drive, or another node has stopped working, it brakes. **It can only make the car slower, never faster.** Even if every other node is wrong, this one should stop the car.

## Topics
| | Topic | What's on it |
|---|---|---|
| Listens to | `/control/command` | What `trajectory_follower` wants to do |
| Listens to | `/planning/trajectory` | The path we're about to drive |
| Listens to | `/perception/lidar/nonground` | LiDAR points with the road removed |
| Listens to | `/system/health` | Whether every node is still running |
| Listens to | `/world/ego` | Our speed |
| Publishes | `/vehicle/control` | The command that goes to the car |
| Publishes | `/safety/status` | Why it braked, for logs and tests |

## How it works
1. **Build the safety strip:** the area the car will sweep along the planned trajectory, a little wider than the car. If the trajectory is stale, use a straight strip ahead of the car.
2. **Check the LiDAR:** count points inside the strip, up to 20 m ahead. Measure their distance *along the path*, so on a turn an obstacle around the corner counts as far away as the drive to it. Something has to be seen in 2 frames in a row before it counts, so one stray point doesn't slam the brakes.
3. **Work out the brake:** brake gently to stop 4 m short of the obstacle. Brake hard only if it's too close for that.
4. **Combine with the follower's command:** `brake = max(follower brake, safety brake)`, `throttle = min(follower throttle, allowed throttle)`, steering unchanged.
5. **Faults → controlled stop** (3 m/s²): `/system/health` reports an error, `/control/command` is older than 0.2 s, or `/planning/trajectory` is older than 1 s.

## Settings to start with
Keep the ACC's tuned values: stop gap 4.0 m, comfortable braking 1.0 m/s², full-brake deceleration 6.0 m/s², brake rate 0.8 per second, urgent braking above 2.5 m/s², look-ahead 20 m, confirm frames 2.

## Done when
It brakes for anything in the planned path and on any fault, and never adds throttle. Add a test that feeds random commands and checks the output throttle is never higher than the input.

## Old code you can reuse
> **None of this code is in this repo.** These links go to old code from before the restructure, saved in git history at commit `86a6bde`. Most of it never ran as part of the driving stack: some of it was never launched, and some was launched but never got the inputs it needed. Read it for ideas, logic and tuned numbers, then write this package fresh in the new layout. Copied files won't work as they are: they use old topic names, old message types and old import paths.
>
> To read the code without a browser: `git show 86a6bde:<path>`.

- **Finding obstacles in the path:** [`autonomous_cruise_node.py`, lines 357–419](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/autonomous_cruise_node.py#L357-L419) (`lidar_callback`). Port it. The only change is the strip: it used the old path plus a 6 m extension; now it's the trajectory.
- **The braking profile:** [`autonomous_cruise_node.py`, lines 468–569](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/autonomous_cruise_node.py#L468-L569) (inside `control_loop`). Port the constants and the "stay stopped until the obstacle is clearly gone" logic.
- **Extending the path past its end:** [`autonomous_cruise_node.py`, lines 339–355](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/autonomous_cruise_node.py#L339-L355). Only needed for the straight-strip fallback.
- **Speed zones near obstacles:** [`airbag_node.py`, lines 65–102](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/safety/airbags/airbags/airbag_node.py#L65-L102) (old `airbags` package). An optional idea for capping speed near things.
- **Node health:** `src/safety/guardian` (in this repo). It becomes `health_monitor`, which publishes `/system/health`.
