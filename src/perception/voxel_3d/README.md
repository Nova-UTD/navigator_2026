# voxel_3d

**Status:** not started. **Full spec: [#12](https://github.com/Nova-UTD/navigator_2026/issues/12).** Read it before you start.

Builds a 3D voxel volume around the car from the LiDAR every frame, and provides ray tracing through it to convert between camera pixels, 3D points and the top-down grid, stopping at obstacles instead of assuming a flat, empty ground. Also publishes the thinned cloud (one point per occupied voxel) on `/perception/lidar/voxels` for `lidar_cluster` and `static_obstacle_grid`. Used as a library by `static_obstacle_grid`, `object_tracker` and `light_state`.

When you build this package, replace this file with its real README: what it does, what it listens to, what it publishes, its settings, how to run it alone and how to test it. See ["How to write a package"](../../README.md#how-to-write-a-package).
