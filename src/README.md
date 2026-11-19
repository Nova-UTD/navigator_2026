# Navigator source code

This is the code for Navigator's new architecture. This page explains how the pieces fit together, what's built and what isn't, and how every package should be written.

## Why a new architecture
The old stack drives well, but every module talked through 2D grids of numbers. A car, a curb and an overhanging branch all looked like "100", nothing remembered anything between frames, and every new behaviour became another grid or another if-branch.

The new stack is built in layers. Each layer has one job and talks to the next through clear messages:

| Layer | Job | Packages |
|---|---|---|
| **Map** | Knows the roads and the rules: lanes, the route from A to B, stop lines, lights, speed limits | `lane_map` |
| **Perception** | What the sensors see in this frame, with no memory | `lidar_ground_filter`, `lidar_object_detector`, `camera_object_detector`, `traffic_light_detector` |
| **World model** | What's around us, remembered over time: tracked objects with IDs and speeds, where they're going, things that don't move, our own position, our traffic light | `ego_state`, `static_obstacle_grid`, `object_tracker`, `prediction`, `light_state` |
| **Decide** | Turns the rules into limits, then picks the best safe trajectory | `behaviour_planner`, `trajectory_planner` |
| **Act** | Drives the trajectory, and brakes if anything goes wrong | `trajectory_follower`, `safety_monitor`, `health_monitor` |

**First goal:** drive from A to B on real lanes: follow cars, stop for pedestrians, pass parked cars when it's allowed, and obey lights and stop signs.

## How the pieces connect
![Architecture](architecture.png)

Arrows are ROS topics. Every node from the world model down also reads `/world/ego` (left out of the picture to keep it readable).

<details>
<summary>Same diagram as Mermaid (renders on GitHub)</summary>

```mermaid
flowchart TD
  subgraph IN["Inputs"]
    BR["CARLA bridge (carla_interface)<br/>LiDAR · cameras · GNSS · map"]
    GOAL["Goal B<br/>RViz click or launch setting"]
  end
  subgraph MAP["Map: the roads and the rules"]
    LM["lane_map"]
  end
  subgraph PER["Perception: what the sensors see right now"]
    GF["lidar_ground_filter"]
    LOD["lidar_object_detector"]
    COD["camera_object_detector"]
    TLD["traffic_light_detector"]
  end
  subgraph WM["World model: what's around us, with memory"]
    EGO["ego_state"]
    SG["static_obstacle_grid"]
    TR["object_tracker"]
    PR["prediction"]
    LS["light_state"]
  end
  subgraph DEC["Decide"]
    BH["behaviour_planner"]
    TP["trajectory_planner"]
  end
  subgraph ACT["Act"]
    TF["trajectory_follower"]
    SM["safety_monitor"]
    HM["health_monitor"]
  end
  CAR["The car (CARLA or the GEM)"]
  GOAL -->|/mission/goal| LM
  BR -->|map| LM
  BR --> GF & COD & TLD & EGO
  EGO -->|/world/ego| LM
  GF -->|/perception/lidar/nonground| LOD & SG
  GF -.->|/perception/lidar/nonground| SM
  LOD -->|/perception/lidar/objects| TR
  COD -->|/perception/camera/objects| TR
  TLD -->|/perception/camera/traffic_lights| LS
  LM -->|/map/road_ahead| LS & PR & BH & TP
  TR -->|/world/tracks| PR
  PR -->|/world/objects| BH & TP
  LS -->|/world/lights| BH
  SG -->|/world/static_grid| TP
  BH -->|/behaviour/constraints| TP
  TP -->|/planning/trajectory| TF & SM
  TF -->|/control/command| SM
  HM -->|/system/health| SM
  SM -->|/vehicle/control| CAR
```
</details>

### What happens every 100 ms
1. The LiDAR ground filter removes the road. The static grid updates, and the object detector puts boxes around what's left.
2. The tracker matches boxes to the objects it already knows, works out their speeds, and adds labels from the camera. Prediction adds where each one is going over the next 5 s.
3. `lane_map` publishes our lane and the rules ahead.
4. `light_state` works out which light is ours and its colour.
5. `behaviour_planner` applies the rules and publishes the limits: walls, speed limit, sideways range.
6. `trajectory_planner` tries about 135 trajectories, throws out the unsafe ones, and publishes the best.
7. `trajectory_follower` (50 times a second) steers and sets the speed to follow it.
8. `safety_monitor` checks the LiDAR along the trajectory and the health of every node, and can brake before the command reaches the car.

## What's in this folder

| Folder | Package | State |
|---|---|---|
| `common/` | `nova_common` | Not started |
| `map/` | `lane_map` | Not started |
| `perception/` | `lidar_ground_filter` | **Works** |
| | `camera_object_detector` | **Works** (needs its YOLO model) |
| | `traffic_light_detector` | **Works** |
| | `lidar_object_detector` | Not started |
| `world_model/` | `ego_state` | Not started |
| | `static_obstacle_grid` | **Works** (still the old 2D grid) |
| | `object_tracker` | **Works** (waits for `lidar_object_detector`) |
| | `prediction`, `light_state` | Not started |
| `behaviour/` | `behaviour_planner` | Not started |
| `planning/` | `trajectory_planner` | Not started |
| `control/` | `trajectory_follower` | Not started |
| `safety/` | `guardian` | **Works.** Becomes `health_monitor` |
| | `safety_monitor` | Not started |
| `msg/` | message packages | Unchanged |
| `tools/` | tools and bundled libraries | Unchanged |

**"Works"** packages came over from the old stack whole and build as before, under their new names and topics. Their messages and logic still change as each one is reworked.

**"Not started"** packages contain only a `README.md`. Each one explains what the package does, the topics it uses, how it should work, the starting settings, when it's done, and **exactly which old code to reuse** (file and line numbers). Read it before writing anything, so you don't start from scratch.

Everything else from the old stack was deleted. It's still in this repo's first commit (`86a6bde`) and in `Nova-UTD/navigator`.

## Running it
Same as before. Start CARLA and the `carla_interface` bridge, then in the Navigator container:

```bash
colcon build --symlink-install
source install/setup.bash
ros2 launch launches/launch.carla.py
```

For now this starts only `lidar_ground_filter` and RViz, so the car doesn't move. New nodes are added to `launches/launch.vehicle.py` as they're written.

## Topics
| Topic | Published by | Read by |
|---|---|---|
| `/mission/goal` | RViz click or launch setting | `lane_map` |
| `/map/road_ahead` | `lane_map` | `prediction`, `light_state`, `behaviour_planner`, `trajectory_planner` |
| `/perception/lidar/nonground` | `lidar_ground_filter` | `lidar_object_detector`, `static_obstacle_grid`, `safety_monitor` |
| `/perception/lidar/objects` | `lidar_object_detector` | `object_tracker` |
| `/perception/camera/objects` | `camera_object_detector` | `object_tracker` |
| `/perception/camera/traffic_lights` | `traffic_light_detector` | `light_state` |
| `/world/ego` | `ego_state` | everything downstream |
| `/world/static_grid` | `static_obstacle_grid` | `trajectory_planner` |
| `/world/tracks` | `object_tracker` | `prediction` |
| `/world/objects` | `prediction` | `behaviour_planner`, `trajectory_planner` |
| `/world/lights` | `light_state` | `behaviour_planner` |
| `/behaviour/constraints` | `behaviour_planner` | `trajectory_planner` |
| `/planning/trajectory` | `trajectory_planner` | `trajectory_follower`, `safety_monitor` |
| `/control/command` | `trajectory_follower` | `safety_monitor` |
| `/vehicle/control` | `safety_monitor` | the car |
| `/system/health` | `health_monitor` | `safety_monitor` |

Topic names follow `/<layer>/<thing>`.

---

## How to write a package

Every package, Python or C++, is laid out the same way. Each part has one job, so you always know where code goes, and the logic can be tested without ROS, CARLA or a GPU.

### The main idea: keep the logic away from ROS
Split every package into two parts:

- **`core/`: the logic.** Plain Python (or C++) that takes inputs and returns outputs. **No ROS imports**: no `rclpy`, no `rclcpp`, no `*_msgs`.
- **`ros/`: the ROS wrapper.** Reads parameters, subscribes, publishes. **No decisions.** It converts ROS messages into plain data, calls `core`, converts the result back, and publishes it.

Why bother:
- **Fast tests.** You can test `core/` with `pytest` in milliseconds, with no ROS, simulator or GPU.
- **Reuse.** Replay tools, test scripts and other packages can call `core/` directly.
- **Easy to read.** Anyone can open `ros/node.py` and understand the whole node in a minute, because it's only plumbing.
- **Easy to swap.** If ROS messages change, only `ros/convert.py` changes.

### Python layout
```text
behaviour_planner/
  package.xml · setup.py · setup.cfg · resource/behaviour_planner
  behaviour_planner/
    core/                      the logic. No ROS imports
      types.py                 plain dataclasses for this package's data
      config.py                a frozen Config dataclass, filled from params.yaml
      rules.py                 e.g. red light -> wall, speed zone -> limit
      stop_sign.py             e.g. the stop-sign state machine
      planner.py               ONE entry point: step(state, inputs, now, config) -> (state, outputs)
    ros/                       the ROS wrapper. No decisions
      node.py                  parameters, subscribers, publishers, one timer, heartbeat
      convert.py               ROS message <-> core dataclass
      viz.py                   RViz markers showing what this node is doing
  config/params.yaml           every number, with its unit and a comment
  launch/behaviour_planner.launch.py   runs this node on its own
  test/
    test_rules.py · test_stop_sign.py  unit tests on core/ (pytest)
    test_convert.py            messages convert both ways without losing anything
    fixtures/                  small example inputs for the tests
  README.md                    what it does, inputs, outputs, settings, how to run and test
```

### C++ layout
Same split:
```text
lane_map/
  CMakeLists.txt · package.xml
  include/lane_map/core/*.hpp   the logic. No rclcpp
  src/core/*.cpp
  src/node.cpp                  the ROS wrapper
  config/params.yaml
  launch/lane_map.launch.py
  test/test_*.cpp               gtest on core
  README.md
```

### What a node looks like
```python
# behaviour_planner/ros/node.py
class BehaviourPlannerNode(Node):
    def __init__(self):
        super().__init__('behaviour_planner')
        self.cfg = Config.from_params(self)        # read parameters once
        self.state = core.initial_state(self.cfg)
        self.latest = {}                            # topic -> newest message

        # Callbacks only store the newest message. Nothing else.
        self.create_subscription(RouteContext, '/map/road_ahead',
                                 lambda m: self.latest.__setitem__('road', m), 1)
        # ... the other subscriptions, same pattern ...

        self.pub = self.create_publisher(BehaviourConstraints, '/behaviour/constraints', 1)
        self.viz_pub = self.create_publisher(MarkerArray, '/behaviour/constraints/viz', 1)
        self.heartbeat_pub = self.create_publisher(DiagnosticStatus, '/system/heartbeat', 1)
        self.create_timer(1.0 / self.cfg.rate_hz, self.tick)
        self.create_timer(1.0, self.heartbeat)

    def tick(self):
        if not convert.inputs_ready(self.latest):
            return
        now = self.get_clock().now().nanoseconds * 1e-9
        inputs = convert.to_inputs(self.latest)                   # messages -> plain data
        self.state, out = core.step(self.state, inputs, now, self.cfg)  # all the logic
        self.pub.publish(convert.to_msg(out, self.get_clock().now()))   # plain data -> message
        self.viz_pub.publish(viz.markers(out))
```
`core.step` is a pure function: the same inputs always give the same outputs. Tests call it directly.

### The rules
| Rule | Why |
|---|---|
| `core/` never imports ROS | It can be tested in milliseconds and reused anywhere |
| `ros/node.py` makes no decisions | Callbacks only store the newest message. One timer converts, calls `core.step()`, converts back and publishes |
| Time is passed in | `core` gets `now` as an argument and never reads the clock (`time.time()`, `get_clock()`), so the simulator, replays and tests all behave the same |
| Every number lives in `config/params.yaml` | Read once into a `Config` dataclass and passed to `core`. No magic numbers in the code, and every value has its unit and a comment |
| Shared things live in `nova_common` | Lanes, objects, trajectories, route maths and shared message conversions are written once and imported, never copied |
| Every node publishes a heartbeat and RViz markers | `health_monitor` can tell it's alive, and you can see what it's doing |
| Every bug fix comes with a test | A unit test, or a CARLA scenario for a driving bug, so it can't come back |
| Every package has a README | What it does · what it listens to · what it publishes · settings · how to run it alone · how to test it |

### Before you open a PR
- `core/` has no ROS imports. Check with `grep -rE "rclpy|rclcpp|_msgs" <package>/<package>/core` (it should print nothing).
- Unit tests pass: `colcon test --packages-select <package>`.
- New numbers are in `params.yaml` with a unit and a comment.
- The README is up to date.
