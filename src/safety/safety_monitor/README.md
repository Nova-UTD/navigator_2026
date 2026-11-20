# safety_monitor

**Status:** not started. **Full spec: [#31](https://github.com/Nova-UTD/navigator_2026/issues/31).** Read it before you start.

The last check before a command reaches the car. Brakes if the LiDAR sees something in the path we're about to drive or if a node has died. Can only make the car slower, never faster. Owns `/vehicle/control`.

When you build this package, replace this file with its real README: what it does, what it listens to, what it publishes, its settings, how to run it alone and how to test it. See ["How to write a package"](../../README.md#how-to-write-a-package).
