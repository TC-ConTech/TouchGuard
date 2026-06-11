#!/bin/bash
#
# TouchGuard uninstaller - unloads the LaunchAgent and removes the binary,
# plist, and Accessibility entry from both user and system locations.
#
set -uo pipefail

LABEL="com.syntaxsoft.touchguard"
UID_NUM="$(id -u)"

echo "Uninstalling TouchGuard..."

# Unload the agent if loaded.
launchctl bootout "gui/$UID_NUM/$LABEL" 2>/dev/null || true

# Remove user-level install.
rm -f  "$HOME/Library/LaunchAgents/$LABEL.plist"
rm -rf "$HOME/Library/Application Support/TouchGuard"

# Remove system-level install (only sudo if present, to avoid a needless prompt).
[ -f "/Library/LaunchAgents/$LABEL.plist" ] && sudo rm -f "/Library/LaunchAgents/$LABEL.plist"
[ -f "/usr/local/bin/TouchGuard" ]          && sudo rm -f "/usr/local/bin/TouchGuard"

# Forget the Accessibility grant.
tccutil reset Accessibility "$LABEL" 2>/dev/null || true

echo "Done. If 'TouchGuard' still shows under System Settings > Privacy & Security"
echo "> Accessibility, remove it there with the '-' button."
