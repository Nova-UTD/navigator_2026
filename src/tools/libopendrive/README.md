# libopendrive

[libOpenDRIVE](https://github.com/pageldev/libOpenDRIVE) reads OpenDRIVE (`.xodr`) maps: roads, lanes, road marks, speed limits, signals, junctions, and which lane leads into which. `lane_map` is built on it.

## What's here
| Path | What it is |
|---|---|
| `libOpenDRIVE/` | Upstream libOpenDRIVE at one exact commit. **Never edit it by hand.** `update.sh` overwrites it |
| `UPSTREAM` | The upstream repo and the commit `libOpenDRIVE/` came from |
| `update.sh` | Replaces `libOpenDRIVE/` with any upstream commit |
| `CMakeLists.txt`, `package.xml` | Our wrapper, so colcon builds it like any other package (in Release, with the system pugixml) |
| `test/` | Our tests, on a small hand-written map (`test/fixtures/test_map.xodr`) |

Upstream's own tests aren't copied: their test map is 1.6 MB, over the repo's 1 MB file limit.

## Using it from another package
```cmake
# CMakeLists.txt
find_package(OpenDrive REQUIRED)
target_link_libraries(<your target> OpenDrive::OpenDrive)
```
```xml
<!-- package.xml -->
<depend>libopendrive</depend>
```
```cpp
#include "OpenDriveMap.h"

odr::OpenDriveMap map("/path/to/map.xodr");
if (map.id_to_road.empty()) { /* the file is missing or isn't valid OpenDRIVE */ }
```
Things to know:
- **It only loads from a file path**, not from a string. To load CARLA's map from `/carla/world_info`, write the string to a temporary file first.
- **A bad file doesn't throw.** It logs an error and gives an empty map, so check `id_to_road.empty()`.
- **Read its maps by reference** (`for (const auto& [id, road] : map.id_to_road)`). `get_roads()`, `get_lanesections()` and `get_lanes()` return copies.
- It logs to stderr. Town12 prints thousands of `Lateral Profile Shape not supported` errors: that's road surface shape (OpenDRIVE 1.6), which doesn't affect lanes. Set `odr::log::set_level(...)` (in `Log.hpp`) to quiet it.

## Build and test
In the Navigator container:
```bash
colcon build --packages-select libopendrive
colcon test --packages-select libopendrive
colcon test-result --verbose --test-result-base build/libopendrive
```
To see each test by name: `build/libopendrive/test_libopendrive`.

## Updating to a newer upstream
1. Pick a commit on [upstream `main`](https://github.com/pageldev/libOpenDRIVE/commits/main) and copy its full 40-character SHA.
2. `./update.sh <sha>`. This replaces `libOpenDRIVE/` and records the commit in `UPSTREAM`.
3. Build and test (above). If upstream renamed something, fix our tests and every package that uses it.
4. Open a PR. Mention what changed upstream that matters to us.
