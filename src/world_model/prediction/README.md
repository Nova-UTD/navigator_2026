# prediction

**Status:** not started. Almost all new.

## What it does
Guesses where every tracked car and person will be over the next 5 seconds, so the planner can avoid where they're going to be, not just where they are now. It uses simple physics, not machine learning.

## Topics
| | Topic | What's on it |
|---|---|---|
| Listens to | `/world/tracks` | Tracked objects with ID, type, position and velocity |
| Listens to | `/map/road_ahead` | Lanes near us |
| Publishes | `/world/objects` | The same objects, each with a predicted 5 s path |

## How it works
- **Cars** (and unknown objects longer than 3 m): find the lane they're in (heading within 45°, less than 2 m away) and move them along that lane at their current speed. If they're not in any lane, keep them going straight.
- **People and bikes:** keep walking or riding in a straight line at their current speed.
- **Anything stopped for more than 2 s:** stays where it is.
- One path per object: 50 points, 0.1 s apart.

## Done when
A car driving into a curve is predicted to follow the curve, not drive off the road.

## Old code you can reuse
> **None of this code is in this repo.** These links go to old code from before the restructure, saved in git history at commit `86a6bde`. Most of it never ran as part of the driving stack: some of it was never launched, and some was launched but never got the inputs it needed. Read it for ideas, logic and tuned numbers, then write this package fresh in the new layout. Copied files won't work as they are: they use old topic names, old message types and old import paths.
>
> To read the code without a browser: `git show 86a6bde:<path>`.

- **Straight-line prediction:** `state_prediction` in `src/world_model/object_tracker/object_tracker/trajectory.py` (in this repo). The tracker's Kalman prediction step; the same idea works for people.
- **Relating objects to the route:** [`frenet_utils.py`, lines 66–91](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/navigator_lane_change/frenet_utils.py#L66-L91). Use the `nova_common` version once it exists.

## Old code to skip
- `pedestrian_intent_to_enter_road`: guessed intent from body pose. Predicted paths replace it.
- `prednet_inference`: machine-learning predictions of future grids.
