# What's new in navigator_2026

`navigator_2026` is where Navigator's new architecture is being built. This page lists everything that's different from the old `navigator` repo. For how the new architecture works and how to write a package, see [`src/README.md`](../src/README.md).

## Running it
**Nothing changed.** The steps in the [Navigator docs](https://nova-utd.github.io/navigator) still work: start CARLA, start the `carla_interface` bridge, run the Navigator container, build, and launch with `ros2 launch launches/launch.carla.py`.

**The car doesn't drive yet.** Only the packages that work today are launched (`lidar_ground_filter` and RViz). New nodes are added to `launches/launch.carla.py` as they're written.

### New shortcuts (optional)
| Shortcut | What it does |
|---|---|
| `docker/shell.sh` | Run from the repo root on the host. Starts the Navigator container if it isn't running and opens a shell in it with ROS and Navigator sourced. Run it again from any terminal to get another shell in the same container. Use it instead of `docker compose run navigator_carla`, which leaves stopped containers behind that can make the VS Code Dev Container hang. `docker/shell.sh <command>` runs a single command instead. |
| `docker/x11.sh` | Finds a working X display on the host (NoMachine, a desktop at the machine, `ssh -X`, VS Code) and shares it with the container, so RViz opens. `docker/shell.sh` and the Dev Container run it for you. If RViz stops opening, run it again on the host; no need to restart the container. |
| `navigator launch carla` | Inside the container: shortcut for `ros2 launch launches/launch.carla.py`. |

### Containers
The Docker Compose project and container names now include `2026` (for example `navigator_2026_carla_<user>`), so this repo's container runs alongside the old repo's without replacing it. In VS Code, the Dev Container is called **Navigator 2026**.

## The `src/` folder is organised by layer
```text
src/
  common/       nova_common
  map/          lane_map
  perception/   lidar_ground_filter, lidar_object_detector, camera_object_detector, traffic_light_detector
  world_model/  ego_state, static_obstacle_grid, object_tracker, prediction, light_state
  behaviour/    behaviour_planner
  planning/     trajectory_planner
  control/      trajectory_follower
  safety/       guardian, safety_monitor
  msg/          message packages (unchanged)
  tools/        tools and bundled libraries
```
Packages that aren't built yet contain only a `README.md`: what the package does, its topics, how it should work, when it's done, and which old code to reuse (with file and line links). Read it before you start one.

## Renamed packages, nodes and topics
The packages kept from the old stack were renamed to match the new architecture. Their logic and message types are unchanged.

| Old package | New package | Executable / node | Topic changes |
|---|---|---|---|
| `ground_seg` | `lidar_ground_filter` | `lidar_ground_filter` | `/lidar/filtered` → `/perception/lidar/nonground` |
| `occupancy_cpp` | `static_obstacle_grid` | `static_obstacle_grid` | `/grid/occupancy/current` → `/world/static_grid`, `/grid/masses` → `/world/static_grid/masses` |
| `road_user_detection` | `camera_object_detector` | `camera_object_detector` | `/road_users/detections` → `/perception/camera/objects` |
| `traffic_light_detector` | (same) | `traffic_light_detector` (was `traffic_light_node`) | `/traffic_lights/detections` → `/perception/camera/traffic_lights` |
| `multi_object_tracker_3d` | `object_tracker` | `object_tracker` | `/detected/objects3d` → `/perception/lidar/objects`, `/tracked/objects3d` → `/world/tracks` |

`guardian` keeps its name for now. It becomes `health_monitor` when it's rebuilt.

## Removed
The rest of the old stack was deleted: `map_management`, the segmentation and grid nodes, `route_costmap`, `grid_summation`, `path_planner`, `intersection_manager`, `autonomous_cruise`, the other planners and controllers, and the SLAM tools. Also removed: settings files for deleted packages, old model configs, `vt_viz`, `voltron_test_utils`, the old routing scripts, and other one-off files.

None of it is lost. It's all in this repo's history at commit `86a6bde`, and in the old `Nova-UTD/navigator` repo:
```bash
git show 86a6bde:<path>          # read an old file
git checkout 86a6bde -- <path>   # bring one back
```

Real-car and localization settings were kept: the `robot_localization` EKF configs, the GEM's drive-by-wire calibration, the LiDAR drivers and fusion, LIO-SAM, and the GEM's tuning and geometry.

## RViz
`data/navigator_default.rviz` keeps the same view, tools and layout. Displays for topics nothing publishes any more were removed: the old grids (drivable area, costs, junctions, predictions), routes and paths, Nav2, pure pursuit, 3D object boxes, RADAR and the planning markers.

On by default: the grid, filtered LiDAR (now coloured by height), cameras 0–3, and the static grid (`/world/static_grid`). Still there but off: raw LiDAR, the CARLA rough route, the bird's-eye camera, the car model and TF.

**The road surface is gone for now.** The grey road in the old view was the drivable-area and cost grids, whose nodes were deleted. Roads come back when `lane_map` publishes lanes.

When you add a node, add a display for its output and save the config in the same PR.
