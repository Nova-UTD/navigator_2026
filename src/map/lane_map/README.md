# lane_map

The car's GPS and rulebook. It loads any OpenDRIVE (`.xodr`) map as a network of lanes and, in later steps, routes A → B over them and publishes the road ahead. It never reads sensors and never makes decisions. Full spec: [#10](https://github.com/Nova-UTD/navigator_2026/issues/10).

**Status:** step 1 of 4 done: [#11](https://github.com/Nova-UTD/navigator_2026/issues/11), load the map and build the lane network. Next: [#16](https://github.com/Nova-UTD/navigator_2026/issues/16) route A → B, [#17](https://github.com/Nova-UTD/navigator_2026/issues/17) `/map/road_ahead`, [#18](https://github.com/Nova-UTD/navigator_2026/issues/18) rules ahead.

## What it builds
At startup, and again whenever CARLA loads a different town, every driving lane in the map becomes a `Lane` (`include/lane_map/core/types.hpp`):

| Field | What it is |
|---|---|
| `id` | `"road/section/lane"`, e.g. `"15/0/-1"`: OpenDRIVE road 15, its first lane section, lane -1 |
| `points` | The centre line every 0.5 m, **in the direction cars drive**. Each point has x, y, z, heading, distance along the lane, width, speed limit (m/s), and the painted line on its left and right edge |
| `left`, `right` | The lane beside it (looking along the lane), and whether it's oncoming |
| `successors`, `predecessors` | The lanes it leads into, and the ones that lead into it |
| `junction_id` | Set if the lane is inside a junction |

Plus `LaneIndex::lane_at(x, y, heading)`: which lane a point is in (an R-tree of lane outlines; on two-way roads and in junctions, the lane pointing your way).

### How it works on any map
Nothing in the code names a map, road or coordinate. Maps differ in how carefully they're made, so each step relies on what's dependable:
- **Direction:** lanes right of the reference line drive along it, lanes left of it drive against it (flipped for left-hand-traffic roads). Points are reversed for lanes that drive against it, so nothing downstream needs to know.
- **Successors come from the geometry, not the links written in the file.** A lane's successors are the lanes it runs into where it ends: within 0.5 m along the road, and sideways inside the next lane's width (plus 0.5 m), pointing the same way (within 60°), at the same height (within 1 m). Landing inside the width, not on the centre, covers lanes that end by narrowing to 0 m and merging into the lane beside them (motorway on-ramps, 2+1 roads) and lanes shifted sideways where a new lane section starts (roadworks). The file's links are unreliable: CARLA's Town07 leaves out links between lanes that really join, and Town03 and Town06 list links to lanes that start metres away.
- **Left-hand traffic** (`rule="LHT"` on a road) flips which side drives which way.
- **Speed limits** are read per point, since a road's limit can change partway along it. Roads with none get `default_speed_limit_mps`.
- **Lanes that are never wider than 5 cm** are dropped, with a warning.

### Speed
Measured in the Navigator container:

| Map | Load (libOpenDRIVE) | Build | Lookup |
|---|---|---|---|
| Town02 | 9 ms | 7 ms | 1.7 µs |
| Town05 | 23 ms | 31 ms | 1.3 µs |
| campus | 13 ms | 23 ms | 1.3 µs |
| Town12 (huge) | 370 ms | 643 ms | 2.4 µs |

- **Points come from `Road::get_xyz`, not `get_surface_pt`.** `get_surface_pt` adds crossfall and lane height (a few cm), but it copies lane objects internally and was ~85% of the build time.
- **The lookup indexes lanes in 10 m pieces,** not whole lanes. A long, curving lane's bounding box matches many queries, and checking a point against it walks hundreds of points (50 µs per lookup on Town05, before the change).

## Topics
| | Topic | Type | Notes |
|---|---|---|---|
| Listens to | `/carla/world_info` | `carla_msgs/CarlaWorldInfo` | Only when `map_source: carla`. Subscribed twice: transient local, to get the copy `carla_ros_bridge` latches the moment we start, and volatile, for `carla_interface`'s re-send every 5 s (which a transient-local subscription can't receive) |
| Publishes | `/map/lanes/viz` | `visualization_msgs/MarkerArray` | Once per map, latched (transient local) |
| Publishes | `/system/heartbeat` | `diagnostic_msgs/DiagnosticStatus` | 1 Hz. OK once loaded, WARN while waiting, ERROR if the map failed to load |

### What the drawing shows
Colours are chosen for RViz's light grey background:
- **Blue:** centre lines, with a `>` every 10 m pointing the way the lane drives
- **Dark grey:** painted white lines, solid or dashed
- **Amber:** painted lines next to oncoming traffic, solid or dashed
- **Brown:** curbs
- **Faint grey:** lane edges with no painted line

## Settings
All in `config/params.yaml`, each with its unit: `map_source` (`carla` | `file`), `map_file`, `frame_id`, `check_period_s`, `sample_spacing_m`, `default_speed_limit_mps`, `min_lane_width_m`, `connect_tolerance_m`, `connect_heading_tolerance_rad`, `connect_height_tolerance_m`, `viz_point_step`.

## Run it on its own
In the Navigator container, after `colcon build --packages-select libopendrive lane_map` and `source install/setup.bash`:
```bash
ros2 launch lane_map lane_map.launch.py                                                  # map from CARLA
ros2 launch lane_map lane_map.launch.py town:=Town03 rviz:=true                          # any CARLA town, no CARLA needed
ros2 launch lane_map lane_map.launch.py map_file:=/navigator/data/maps/campus.xodr rviz:=true   # any .xodr file
```
`town:=` reads the town's map file from CARLA's install (mounted at `/carla`), so you can look over any town's lanes without running CARLA. With `rviz:=true`, RViz opens looking at the `map` frame.

In the full stack (`launches/launch.carla.py`), `lane_map` takes the map from CARLA, and rebuilds by itself when CARLA loads a different town. RViz's frame there is `base_link`, which follows the car.

## Test it
```bash
colcon test --packages-select lane_map
colcon test-result --verbose --test-result-base build/lane_map
```
- `test/test_build.cpp`, `test/test_lookup.cpp`: on `test/fixtures/test_map.xodr`, a small hand-written map (described at the top of the file) where every expected value can be worked out by hand.
- `test/test_real_maps.cpp`: every CARLA town and campus, found on disk at the paths the Navigator container mounts. These maps are over the repo's 1 MB file limit, so the test skips itself when they aren't there (as on GitHub's CI). Choose the maps with `LANE_MAP_REAL_MAPS=/a.xodr:/b.xodr`. Run `build/lane_map/test_real_maps` to see its table: lanes, lanes with no successor or predecessor, and how many lane midpoints the lookup finds correctly.

## Layout
```text
include/lane_map/core/   types.hpp, config.hpp, build.hpp, lookup.hpp   the logic, no ROS
src/core/                build.cpp, lookup.cpp
src/node.cpp             the ROS wrapper: parameters, loading, publishing
src/viz.cpp              lanes → RViz markers
config/params.yaml       every setting
launch/lane_map.launch.py
test/
```
`core/` builds as its own library, `lane_map_core`, and the tests link only that library, so the build fails if anything in `core/` needs ROS.
