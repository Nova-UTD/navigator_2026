# trajectory_planner

**Status:** not started. **Full spec: [#29](https://github.com/Nova-UTD/navigator_2026/issues/29).** Read it before you start.

Ten times a second, tries about 135 possible trajectories for the next 5 seconds, throws out any that hit something or break a rule, and publishes the best on `/planning/trajectory`.

When you build this package, replace this file with its real README: what it does, what it listens to, what it publishes, its settings, how to run it alone and how to test it. See ["How to write a package"](../../README.md#how-to-write-a-package).
