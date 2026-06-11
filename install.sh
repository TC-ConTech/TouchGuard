#!/bin/bash
#
# TouchGuard installer - installs the TouchGuard binary and, optionally, a
# LaunchAgent so it starts automatically at login and survives reboot.
#
# Usage:
#   ./install.sh                  interactive (asks about auto-start)
#   ./install.sh --auto-start     install and enable auto-start at login
#   ./install.sh --no-auto-start  install the binary only
#   ./install.sh --time 0.3       disable interval in seconds (default 0.2)
#   ./install.sh --system         install to /usr/local/bin + /Library/LaunchAgents (uses sudo)
#   ./install.sh --uninstall      remove everything (runs uninstall.sh)
#
# On macOS 10.14+ TouchGuard needs Accessibility permission (not root) to
# suppress trackpad taps. The installer loads the agent and points you at the
# Accessibility settings; enable TouchGuard there and it starts working.
#
set -euo pipefail

LABEL="com.syntaxsoft.touchguard"
HERE="$(cd "$(dirname "$0")" && pwd)"
TIME="0.2"
SYSTEM=0
AUTOSTART="ask"   # ask | yes | no

while [ $# -gt 0 ]; do
    case "$1" in
        --time) TIME="${2:?--time needs a value}"; shift 2 ;;
        --auto-start) AUTOSTART="yes"; shift ;;
        --no-auto-start) AUTOSTART="no"; shift ;;
        --system) SYSTEM=1; shift ;;
        --uninstall) exec "$HERE/uninstall.sh" ;;
        -h|--help) grep '^#' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 2 ;;
    esac
done

if [ "$SYSTEM" -eq 1 ]; then
    BIN_DIR="/usr/local/bin"
    AGENT_DIR="/Library/LaunchAgents"
    SUDO="sudo"
else
    BIN_DIR="$HOME/Library/Application Support/TouchGuard"
    AGENT_DIR="$HOME/Library/LaunchAgents"
    SUDO=""
fi
BIN="$BIN_DIR/TouchGuard"
PLIST="$AGENT_DIR/$LABEL.plist"
LOG="$HOME/Library/Logs/TouchGuard.log"
UID_NUM="$(id -u)"

echo "TouchGuard installer"
echo "  binary : $BIN"
echo "  agent  : $PLIST"
echo "  -time  : ${TIME}s"
echo

# 1) Obtain a binary: prefer a prebuilt ./TouchGuard file, else build from source.
SRC=""
if [ -f "$HERE/TouchGuard" ]; then
    SRC="$HERE/TouchGuard"
    echo "Using prebuilt binary: $SRC"
elif command -v clang >/dev/null 2>&1 && [ -f "$HERE/TouchGuard/main.c" ]; then
    echo "Building from source with clang..."
    clang -O2 "$HERE/TouchGuard/main.c" \
        -framework ApplicationServices -framework CoreFoundation \
        -o "/tmp/TouchGuard.$$"
    SRC="/tmp/TouchGuard.$$"
else
    echo "Error: no prebuilt 'TouchGuard' binary and no clang+source to build from." >&2
    echo "Install Command Line Tools (xcode-select --install) or download a release binary." >&2
    exit 1
fi

# 2) Install + ad-hoc sign with a stable identifier so the Accessibility grant
#    stays attached to the binary across reboots and rebuilds.
$SUDO mkdir -p "$BIN_DIR"
$SUDO cp "$SRC" "$BIN"
$SUDO chmod +x "$BIN"
$SUDO codesign --force --sign - --identifier "$LABEL" "$BIN" >/dev/null 2>&1 || true
[ "$SRC" = "/tmp/TouchGuard.$$" ] && rm -f "$SRC"
echo "Installed binary."

# 3) Decide auto-start.
if [ "$AUTOSTART" = "ask" ]; then
    if [ -t 0 ]; then
        printf "Enable automatic start at login (survives reboot)? [y/N] "
        read -r ans
        case "$ans" in [Yy]*) AUTOSTART="yes" ;; *) AUTOSTART="no" ;; esac
    else
        AUTOSTART="no"   # never enable silently in a non-interactive run
    fi
fi

if [ "$AUTOSTART" != "yes" ]; then
    echo
    echo "Done. Auto-start NOT enabled. Run manually with:"
    echo "  \"$BIN\" -time $TIME"
    exit 0
fi

# 4) Install + load the LaunchAgent.
mkdir -p "$HOME/Library/Logs"
$SUDO mkdir -p "$AGENT_DIR"
sed -e "s|@@BINARY@@|$BIN|g" -e "s|@@TIME@@|$TIME|g" -e "s|@@LOG@@|$LOG|g" \
    "$HERE/$LABEL.plist" | $SUDO tee "$PLIST" >/dev/null
echo "Installed LaunchAgent."

launchctl bootout "gui/$UID_NUM/$LABEL" 2>/dev/null || true
launchctl bootstrap "gui/$UID_NUM" "$PLIST"
echo "Loaded LaunchAgent."

# 5) Accessibility grant (required on macOS 10.14+). The agent is already
#    running and has registered TouchGuard in the Accessibility list.
echo
echo "ACTION REQUIRED - grant Accessibility permission:"
echo "  System Settings > Privacy & Security > Accessibility > enable 'TouchGuard'."
open "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility" 2>/dev/null || true
echo
echo "After enabling it, activate it with a full reload (a new Accessibility"
echo "grant is NOT picked up by 'kickstart' - it needs bootout+bootstrap), or"
echo "simply log out and back in / reboot:"
echo "  launchctl bootout gui/$UID_NUM/$LABEL 2>/dev/null; launchctl bootstrap gui/$UID_NUM \"$PLIST\""
echo
echo "TouchGuard then runs at every login. Log: $LOG"
