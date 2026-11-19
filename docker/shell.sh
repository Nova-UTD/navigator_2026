#!/bin/bash
# Open a shell in the shared navigator_carla container (the same one VS Code attaches to).
# Starts it if needed. Use this instead of `docker compose run navigator_carla`, which leaves
# behind exited one-off containers that make the VS Code Dev Container connection hang.
#
# Usage: docker/shell.sh [command...]   (no command = interactive shell with ROS sourced)

set -e

# docker-compose.yml bind-mounts $PWD, so it must run from the repo root
cd "$(dirname "$0")/.."

# Find a working X display for RViz and write it to .x11/ (read by entrypoint.sh)
export DISPLAY="$(docker/x11.sh)"

# --no-recreate: never replace a running container just because env vars (DISPLAY, etc.) differ
docker compose up -d --no-recreate navigator_carla

# Use the repo's entrypoint.sh, not the image's copy, so changes apply without rebuilding
exec docker exec -it "navigator_2026_carla_${USER}" bash /navigator/docker/entrypoint.sh "$@"
