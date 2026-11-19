#!/bin/bash
# Find a working X display on the host and share it with the container, so RViz and other GUIs
# can open no matter how you got here (NoMachine, a desktop at the machine, ssh -X, VS Code).
#
# Writes .x11/env and .x11/xauth in the repo (mounted at /navigator). Container shells source
# .x11/env (see entrypoint.sh), so rerunning this fixes the display without recreating the container.
# Run on the host; docker/shell.sh and the Dev Container's initializeCommand call it for you.
#
# Usage: docker/x11.sh          (prints the display it picked)

REPO="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$REPO/.x11"

x_ok() {
    # Without xdpyinfo we can't test, so trust the candidate.
    command -v xdpyinfo >/dev/null || return 0
    DISPLAY="$1" XAUTHORITY="$2" timeout 2 xdpyinfo >/dev/null 2>&1
}

candidates() {
    # 1. Whatever this shell already has (desktop terminal, NoMachine terminal, ssh -X).
    [ -n "$DISPLAY" ] && echo "$DISPLAY"
    # 2. X sockets owned by this user, newest first (NoMachine virtual desktops, :1001 and up).
    for s in $(ls -t /tmp/.X11-unix/ 2>/dev/null); do
        [ -O "/tmp/.X11-unix/$s" ] && echo ":${s#X}"
    done
    # 3. Displays this user is logged in on at the machine, e.g. "(:0)" or "(:1)".
    who 2>/dev/null | awk -v u="$USER" '$1 == u' | grep -o '(:[0-9.]*)' | tr -d '()'
    # 4. Common local displays.
    echo :0
    echo :1
}

auth_files() {
    [ -n "$XAUTHORITY" ] && echo "$XAUTHORITY"
    echo "$HOME/.Xauthority"
    echo "/run/user/$(id -u)/gdm/Xauthority"
}

found=""
for d in $(candidates | awk '!seen[$0]++'); do
    for a in $(auth_files); do
        [ -f "$a" ] || continue
        if x_ok "$d" "$a"; then found="$d"; auth="$a"; break 2; fi
    done
done

mkdir -p "$OUT"
chmod 700 "$OUT"

if [ -z "$found" ]; then
    echo "[x11.sh] No working X display found; GUIs (RViz) won't open. Connect with NoMachine or log in at the machine, then rerun." >&2
    : > "$OUT/env"
    exit 0
fi

# Copy the cookie with the hostname wildcarded ("ffff") so it matches from inside the container.
rm -f "$OUT/xauth"
touch "$OUT/xauth"
chmod 600 "$OUT/xauth"
XAUTHORITY="$auth" xauth nlist "$found" 2>/dev/null | sed -e 's/^..../ffff/' | xauth -f "$OUT/xauth" nmerge - 2>/dev/null

# Backup in case the cookie doesn't match: let local root (the container) connect.
command -v xhost >/dev/null && DISPLAY="$found" XAUTHORITY="$auth" xhost +SI:localuser:root >/dev/null 2>&1

cat > "$OUT/env" <<EOF
export DISPLAY=$found
export XAUTHORITY=/navigator/.x11/xauth
EOF

echo "$found"
