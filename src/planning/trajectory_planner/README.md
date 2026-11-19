# trajectory_planner

**Status:** not started. Mostly new. This replaces the old grid-and-Dijkstra planning.

## What it does
Ten times a second, it decides where the car should be and how fast for the next 5 seconds. It tries about 135 possible trajectories, throws out any that would hit something or break a rule, and picks the best of what's left. Following a car ahead comes out of this naturally: trajectories that get too close are thrown out.

## Topics
| | Topic | What's on it |
|---|---|---|
| Listens to | `/map/road_ahead` | Our lane's centre line and edges |
| Listens to | `/world/static_grid` | Things that don't move (curbs, walls, parked cars) |
| Listens to | `/world/objects` | Moving things, with where they'll be |
| Listens to | `/behaviour/constraints` | Walls, speed limit and how far sideways we may go |
| Listens to | `/world/ego` | Where we are and our speed |
| Publishes | `/planning/trajectory` | The chosen trajectory: points 0.1 s apart, each with position and speed |
| Publishes | `/planning/trajectory/viz` | Every candidate in RViz: green = chosen, grey = allowed, red = rejected (with the reason) |

## How it works
1. **Make candidates.** Combine 9 sideways positions (around the lane centre) × 2 ways of getting there × about 15 speed profiles (keep a speed, or stop at a wall or behind the car ahead). That's about 135 candidates, each 5 s long. Always add one emergency option: stay in place sideways and brake to a stop.
2. **Throw out the bad ones.** A candidate is rejected if, at any moment, it:
   - goes outside the allowed sideways range
   - touches an occupied cell in the static grid (the car is checked as 3 circles along its body)
   - comes within 0.5 m of a car (1.0 m of a person) where that object is *predicted* to be at that moment
   - drives past a wall
   - breaks a limit: speed limit, acceleration, braking, how sharply the car can turn, sideways acceleration
3. **Score what's left.** Lower is better: losing speed, being off-centre, jerky motion, being close to objects, and changing plan from last time (so it doesn't flip-flop).
4. **Pick the best.** If nothing is allowed, use the emergency stop.

## Settings to start with
Max acceleration 2.5 m/s², max braking 3.5 m/s² (6.0 for the emergency stop only), max sideways acceleration 2.0 m/s², wheelbase 2.875 m, object margin 0.5 m (people 1.0 m), stop gap behind a car 4.0 m. Today's following gaps are 9.1–10.7 m centre to centre; don't get worse. Aim for 30 ms per cycle; if NumPy is too slow, move the collision check to C++.

## Done when
In CARLA with perfect perception (the ground-truth helper), it drives A → B, follows the car ahead at least as well as today, stops for a person stepping out, and passes a parked car when allowed.

## Old code you can reuse
> **None of this code is in this repo.** These links go to old code from before the restructure, saved in git history at commit `86a6bde`. Most of it never ran as part of the driving stack: some of it was never launched, and some was launched but never got the inputs it needed. Read it for ideas, logic and tuned numbers, then write this package fresh in the new layout. Copied files won't work as they are: they use old topic names, old message types and old import paths.
>
> To read the code without a browser: `git show 86a6bde:<path>`.

- **The ACC's following gap:** [`longitudinal_controller.py`, lines 134–233](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/longitudinal_controller.py#L134-L233). Lead-car detection and the speed it chose (1.5 s time gap, at least 5 m). Use these numbers when rejecting candidates that get too close.
- **Stopping smoothly before an obstacle:** [`autonomous_cruise_node.py`, lines 468–521](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/autonomous_cruise_node.py#L468-L521). The "fastest speed that still stops us in time" calculation; the same maths works for walls and the car ahead.
- **Generating and scoring candidates:** [`rtp_node.py`, lines 206–341](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/rtp/rtp/rtp_node.py#L206-L341) (old tree planner). Only an example of the general idea.

## Old code to skip
- `costs` (route costmap, grid summation) and `path_planners` (Dijkstra, A*, and the rest): planning on summed grids is what this package replaces.
