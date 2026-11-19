# lidar_object_detector

**Status:** not started. New C++ code using PCL.

## What it does
Takes the LiDAR points that aren't road and groups points that are close together into objects, then puts a 3D box around each one. It doesn't know *what* each object is (car, person); the tracker adds that from the camera.

## Topics
| | Topic | What's on it |
|---|---|---|
| Listens to | `/perception/lidar/nonground` | LiDAR points with the road removed |
| Publishes | `/perception/lidar/objects` | A 3D box for every object |

## How it works
1. Keep points within 40 m of the car, between 0.15 m and 2.5 m above the ground.
2. Thin them out to one point per 10 cm cube, so the next step is fast.
3. Group points that are within 0.5 m of each other (a group needs 10 to 5,000 points).
4. Fit a box around each group to get its size and which way it faces.
5. Drop anything longer than 12 m or bigger than 30 m²: that's a wall or a building, and the static grid handles those.
6. Convert the boxes to map coordinates.

## Done when
It boxes every car and person within 30 m in the Town02 scenarios (at least 95% of them, compared with CARLA's ground truth).

## Old code you can reuse
> **None of this code is in this repo.** These links go to old code from before the restructure, saved in git history at commit `86a6bde`. Most of it never ran as part of the driving stack: some of it was never launched, and some was launched but never got the inputs it needed. Read it for ideas, logic and tuned numbers, then write this package fresh in the new layout. Copied files won't work as they are: they use old topic names, old message types and old import paths.
>
> To read the code without a browser: `git show 86a6bde:<path>`.

- **A C++ PCL node to copy the setup from:** `src/perception/lidar_ground_filter` (in this repo). It produces this node's input; copy its package layout, point-cloud conversion and CMake setup.
- **Filling in the box message:** [`lidar_detection_node.py`, lines 99–123](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/perception/lidar_detection/lidar_detection/lidar_detection_node.py#L99-L123) (old machine-learning detector). Only for how it filled in `Object3DArray`.
- **Drawing boxes in RViz:** [`object_visualizer_node.py`, lines 34–91](https://github.com/Nova-UTD/navigator_2026/blob/86a6bde1a7b2f442c597d56593a7c792a78d1f86/src/perception/visualizer/visualizer/object_visualizer_node.py#L34-L91). A start for `viz`.
