#!/usr/bin/env bash
# Replace libOpenDRIVE/ with upstream libOpenDRIVE at one exact commit.
#
# Usage:  ./update.sh <40-character commit SHA>
#
# Find a commit at https://github.com/pageldev/libOpenDRIVE/commits/main.
# Then rebuild and run the tests (see README.md) before opening a PR.
set -euo pipefail

REPO="pageldev/libOpenDRIVE"
# Only these are copied. Upstream's tests/test.xodr is 1.6 MB, over the
# repo's 1 MB file limit, so its tests are left out; ours are in test/.
KEEP=(CMakeLists.txt OpenDriveConfig.cmake.in LICENSE README.md include src)

sha="${1:-}"
if [[ ! "$sha" =~ ^[0-9a-f]{40}$ ]]; then
  echo "Usage: $0 <40-character commit SHA>" >&2
  echo "A full SHA pins one exact version; a branch name would change under us." >&2
  exit 1
fi

here="$(cd "$(dirname "$0")" && pwd)"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

echo "Downloading $REPO at $sha ..."
curl -fsSL "https://codeload.github.com/$REPO/tar.gz/$sha" | tar -xz -C "$tmp" --strip-components=1

rm -rf "$here/libOpenDRIVE"
mkdir "$here/libOpenDRIVE"
for item in "${KEEP[@]}"; do
  cp -r "$tmp/$item" "$here/libOpenDRIVE/"
done

cat > "$here/UPSTREAM" <<EOF
repo:   https://github.com/$REPO
commit: $sha
EOF

echo "Done. libOpenDRIVE/ is now upstream $sha (recorded in UPSTREAM)."
echo "Next: colcon build --packages-select libopendrive && colcon test --packages-select libopendrive"
