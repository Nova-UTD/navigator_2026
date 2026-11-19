# ego_state

**Status:** not started. Small.

## What it does
Publishes our own position, heading, speed and acceleration in one message that every other node reads. It doesn't need the map.

## Topics
| | Topic | What's on it |
|---|---|---|
| Listens to | `/gnss/odometry` | Our position from GNSS (in CARLA, from `carla_interface`). On the GEM later: a filter that combines GNSS, IMU and wheel speed |
| Publishes | `/world/ego` | Position, heading, speed, acceleration, turn rate |

## How it works
- **Position and heading:** passed through from the input.
- **Speed:** the velocity along our heading (negative when reversing).
- **Acceleration:** the change in speed, smoothed (0.3 s smoothing).
- **Turn rate:** from the input's angular velocity.

## Done when
Position is within 0.2 m and speed within 0.1 m/s of CARLA's true values.

## Old code you can reuse
> **None of this code is in this repo.** These links go to old code from before the restructure, saved in git history at commit `86a6bde`. Most of it never ran as part of the driving stack: some of it was never launched, and some was launched but never got the inputs it needed. Read it for ideas, logic and tuned numbers, then write this package fresh in the new layout. Copied files won't work as they are: they use old topic names, old message types and old import paths.
>
> To read the code without a browser: `git show 86a6bde:<path>`.

- **Reading GNSS:** [`gnss_averaging_node.py`, lines 151–255](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/perception/state_estimation/state_estimation/gnss_averaging_node.py#L151-L255). How the old stack read and smoothed GNSS and published the car's position.
- **How the ACC read position and speed:** [`autonomous_cruise_node.py`, lines 300–324](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/autonomous_cruise_node.py#L300-L324). Which topics and message types `carla_interface` publishes today.
- **Publishing tf:** [`odom2tf.py`, lines 40–66](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/tools/recording/recording/odom2tf.py#L40-L66) (still in this repo's `tools/`).

## Old code to skip
- `mcl_node.py` (particle-filter localisation): not needed in CARLA, and the GEM will use a GNSS + IMU filter.
