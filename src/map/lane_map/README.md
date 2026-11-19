# lane_map

**Status:** not started. The old `map_management` already loads maps and finds routes over lanes; that routing was written but never switched on.

## What it does
The car's GPS and rulebook:
1. **Loads the map:** the OpenDRIVE file (from CARLA, or `campus.xodr`) as a network of lanes: which lane leads into which, and which lanes are side by side.
2. **Plans the route:** A to B over those lanes, like a GPS app.
3. **Describes our lane:** width, centre line, how far it continues, whether the lane beside us is oncoming, and whether we're allowed to pass into it.
4. **Describes the rules ahead,** with their distances: stop lines, lights, stop signs, crosswalks, junctions and speed limits.

It never looks at sensors and never makes decisions. It uses libOpenDRIVE (`src/tools/libopendrive`).

## Topics
| | Topic | What's on it |
|---|---|---|
| Listens to | the map | `/carla/world_info` from CARLA, or a map file set in the parameters |
| Listens to | `/world/ego` | Where we are |
| Listens to | `/mission/goal` | Where to go (an RViz click or a launch setting) |
| Publishes | `/map/road_ahead` | Our lane, the lanes beside it, and everything ahead, 10 times a second |
| Publishes | `/map/lanes/viz`, `/map/road_ahead/viz` | RViz markers |

## How it works
- **At startup:** load the map. Build the lane network. Merge in extra rules from `data/maps/rules/<map>.yaml` (the campus map is missing its lights and stop signs).
- **When a goal arrives:** find the lane we're in and the lane the goal is in, and search the network for the best route. Changing lanes costs extra, so the route doesn't hop between lanes.
- **Every 100 ms:** find where we are on the route and publish the next 150 m (and 20 m behind): centre line, lane edges, speed limits, the lanes beside us, and every rule ahead with its distance.
- **May we pass?** From the lane line: broken → yes; solid, double solid or curb → no; no line → a setting (default no on campus).

## Settings to start with
Look ahead 150 m, behind 20 m, sample every 0.5 m, nearby lanes within 60 m, lane change cost 50 m, reset the route after 2 s off it, crossing unmarked lines off.

## Done when
It plans A → B on Town02 and on campus, and every lane, stop line, light and passing zone shows up in RViz in the right place while driving. Tests: the map loads; a known A → B gives the expected lanes; Town02 stop lines are within 1 m of CARLA's; x, y ↔ s, d round-trips.

## Old code you can reuse
> **None of this code is in this repo.** These links go to old code from before the restructure, saved in git history at commit `86a6bde`. Most of it never ran as part of the driving stack: some of it was never launched, and some was launched but never got the inputs it needed. Read it for ideas, logic and tuned numbers, then write this package fresh in the new layout. Copied files won't work as they are: they use old topic names, old message types and old import paths.
>
> To read the code without a browser: `git show 86a6bde:<path>`.

- **Loading the map from CARLA:** [`MapManager.cpp`, lines 1371–1390](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/src/MapManager.cpp#L1371-L1390) (`worldInfoCb`). Port it.
- **Loading from a file, and setup:** [`MapManager.cpp`, lines 38–147](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/src/MapManager.cpp#L38-L147) (the constructor). Loads `campus.xodr` at [line 92](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/src/MapManager.cpp#L92) and builds the lane network at [line 103](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/src/MapManager.cpp#L103).
- **The lane network and routing:** [`MapManager.cpp`, lines 932–1038](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/src/MapManager.cpp#L932-L1038) (`buildTrueRoutingGraph`, `recursiveSearch`, `calculateRoute`). Port it. **Read** [`getTrueSuccessors`, lines 865–931](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/src/MapManager.cpp#L865-L931) first: it works around bugs in libOpenDRIVE's lane connections.
- **Lane centre lines:** [`MapManager.cpp`, lines 1146–1205](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/src/MapManager.cpp#L1146-L1205) (`getLaneCenterline`, `getReorientedRoute`). Port it.
- **Goals from an RViz click:** [`MapManager.cpp`, lines 242–430](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/src/MapManager.cpp#L242-L430). How a clicked point became a route.
- **Start and goal lanes, junctions:** [`getLaneStart`, lines 829–864](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/src/MapManager.cpp#L829-L864) and [lines 1322–1370](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/src/MapManager.cpp#L1322-L1370) (`updateRouteGivenDestination`, `getJunctionMap`).
- **Headers and build setup:** [`MapManager.hpp`](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/include/map_management/MapManager.hpp) for which libOpenDRIVE types it uses, and [`CMakeLists.txt`](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/CMakeLists.txt) for how it links libOpenDRIVE, lemon and Boost.
- **Fallback route while routing is built:** [`routing_hardcoded.py`, lines 33–79](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/tools/helpers/helpers/routing_hardcoded.py#L33-L79). Replays `Route.csv` (still in this repo's `tools/`).

## Old code to skip
- Painting the drivable-area grid: [`publishGrids`, lines 566–828](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/src/MapManager.cpp#L566-L828) and [lines 1075–1145](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/mapping/map_management/src/MapManager.cpp#L1075-L1145). Lane edges replace it.
- The old `route_costmap` grid. The lane route replaces it.
