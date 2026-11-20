# lane_map

**Status:** not started. **Full spec: [#10](https://github.com/Nova-UTD/navigator_2026/issues/10).** Read it before you start.

The car's GPS and rulebook. Loads any OpenDRIVE map as a network of lanes, routes A → B over them, and publishes our lane, the lanes beside it and the rules ahead on `/map/road_ahead`. Never reads sensors, never decides. Built in steps: [#11](https://github.com/Nova-UTD/navigator_2026/issues/11), [#16](https://github.com/Nova-UTD/navigator_2026/issues/16), [#17](https://github.com/Nova-UTD/navigator_2026/issues/17), [#18](https://github.com/Nova-UTD/navigator_2026/issues/18), [#19](https://github.com/Nova-UTD/navigator_2026/issues/19).

When you build this package, replace this file with its real README: what it does, what it listens to, what it publishes, its settings, how to run it alone and how to test it. See ["How to write a package"](../../README.md#how-to-write-a-package).
