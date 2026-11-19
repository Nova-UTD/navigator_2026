# behaviour_planner

**Status:** not started. Several old pieces cover parts of it.

## What it does
Applies the rules of the road. It never steers or brakes. It tells `trajectory_planner` what it's not allowed to do:
- **Walls:** "don't drive past this point" (red light, stop sign, a pedestrian in the crosswalk, the goal)
- **Speed limit:** from the map, or lower near an unclear light
- **How far sideways we may go:** our lane normally; wider into the next lane while passing

## Topics
| | Topic | What's on it |
|---|---|---|
| Listens to | `/map/road_ahead` | Our lane, speed limits, and the rules ahead with their distances |
| Listens to | `/world/objects` | Cars and people around us, with where they're going |
| Listens to | `/world/lights` | The colour of the light that controls our lane |
| Listens to | `/world/ego` | Where we are and our speed |
| Publishes | `/behaviour/constraints` | Walls, speed limit and sideways limits, 10 times a second |

## How it works
Every tick it starts from the defaults (the map's speed limit, our own lane) and applies these rules.

**Simple rules (checked every tick):**
| Rule | When | What it adds |
|---|---|---|
| Speed limit | Always | The map's limit for where we are |
| Red light | Our light is red and the stop line is ahead | A wall at the stop line |
| Yellow light | Yellow, and we can still stop comfortably | A wall at the stop line (otherwise go through) |
| Unclear light | Can't tell the colour, stop line within 30 m | Slow to 3 m/s, and stop at the line if needed |
| Goal | The goal is ahead | A wall at the goal |
| Crosswalk | A person is in it, or will step in within 3 s | A wall just before the crosswalk |

**State machines (they remember what happened):**
- **Stop sign:** approach and stop at the line → wait 2 s → check nobody will cross our path in the next 4 s (and at an all-way stop, nobody who arrived first is still waiting) → go → done once past the junction.
- **Junction yield:** at a yield, or turning left on green, wait at the junction entry while anyone would cross our path before we clear it.
- **Passing a parked or stopped car:** following → blocked (the car ahead hasn't moved for 5 s, or is parked at the edge) → check we may cross into the next lane for the whole pass and no oncoming car will arrive in time → passing (sideways limit widened, speed 4 m/s) → return to our lane once 8 m past it → following. Cancel and stay in our lane if the check fails before we reach the car.

It also publishes its current state and the reason as plain text (for example `PASSING obj=12`), so you can see in RViz why it's doing something.

## Settings to start with
Stop margin 1.0 m, comfortable braking 2.0 m/s², unclear-light slow zone 30 m at 3.0 m/s, stop-sign hold 2.0 s, gap 4.0 s, crosswalk look-ahead 3.0 s, wait before passing 5.0 s, passing speed 4.0 m/s, edge margin 0.3 m, car width 1.9 m.

## Done when
Every rule and every state-machine step has a unit test with a small hand-made example, and the light, stop-sign, crosswalk, junction and passing scenarios pass in CARLA.

## Old code you can reuse
> **None of this code is in this repo.** These links go to old code from before the restructure, saved in git history at commit `86a6bde`. Most of it never ran as part of the driving stack: some of it was never launched, and some was launched but never got the inputs it needed. Read it for ideas, logic and tuned numbers, then write this package fresh in the new layout. Copied files won't work as they are: they use old topic names, old message types and old import paths.
>
> To read the code without a browser: `git show 86a6bde:<path>`.

**Intersections and stop signs**
- **ACC intersection state machine:** [`intersection_handler.py`, lines 20–324](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/intersection_handler.py#L20-L324). Stop/proceed states, the transitions ([`update_state`, lines 107–170](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/intersection_handler.py#L107-L170)) and stopping deceleration ([lines 193–230](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/intersection_handler.py#L193-L230)). Reuse the states and thresholds. It outputs a target speed; ours outputs walls.
- **Old intersection manager:** [`intersection_manager.py`, lines 108–366](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/intersection_manager/intersection_manager/intersection_manager.py#L108-L366). Traffic-light ([lines 131–161](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/intersection_manager/intersection_manager/intersection_manager.py#L131-L161)), stop-sign ([lines 164–283](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/intersection_manager/intersection_manager/intersection_manager.py#L164-L283)) and roundabout ([lines 286–340](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/intersection_manager/intersection_manager/intersection_manager.py#L286-L340)) logic. Take the thresholds, like braking distance `0.5 × 15 × t²`. **Skip** its trick of estimating distance from how wide the sign looks in the camera: distances now come from the map.
- **How the ACC used it:** [`autonomous_cruise_intersection_node.py`, lines 398–469](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/autonomous_cruise_intersection_node.py#L398-L469). For reference.

**Passing** (from the old `navigator_lane_change`, already written without ROS, with tests)
- **State machine:** [`lane_change_state_machine.py`, lines 14–226](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/navigator_lane_change/lane_change_state_machine.py#L14-L226). Prepare → check → find gap → wait → commit → execute, with abort and fault handling. The closest thing to the passing state machine; reuse its structure.
- **"Is something blocking us?":** [`need_detector.py`, lines 16–128](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/navigator_lane_change/need_detector.py#L16-L128). The following → blocked step.
- **"Is there a safe gap?":** [`gap_selector.py`, lines 17–167](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/navigator_lane_change/gap_selector.py#L17-L167). The check before passing.
- **Lane choice and final check:** [`target_lane_selector.py`](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/navigator_lane_change/target_lane_selector.py) and [`safety_checker.py`](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/navigator_lane_change/safety_checker.py).
- **Data types:** [`types.py`, lines 7–87](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/navigator_lane_change/types.py#L7-L87). A good pattern for `core/types.py`.
- **Tests:** [the 4 test files](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/test). Patterns for testing state machines.
- **Design notes:** [`README.md`](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/planning/navigator_lane_change/README.md).

**Parameters:** [`param/gem_e6_intersection_params.yaml`](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/param/gem_e6_intersection_params.yaml) and [`param/lane_change_params.yaml`](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/param/lane_change_params.yaml) (still in this repo's `param/` folder).

## Good to know
The old ACC's red-light rule ([`autonomous_cruise_node.py`, lines 326–331](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/autonomous_cruise/autonomous_cruise/autonomous_cruise_node.py#L326-L331)) listened to `/carla/traffic_light_state`, which nothing ever published, so the car never actually stopped for red lights. That rule is new work here.
