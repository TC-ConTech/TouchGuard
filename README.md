# TouchGuard

Disables Mac touchpad for a user-specified amount of time each time a key is pressed on the keyboard. This prevents accidental touchpad input (e.g. palm of hand moving over the edge of the touchpad) from being detected as a tap and causing the cursor to jump to a different line while the user is typing.

**Download latest release from [here](https://github.com/thesyntaxinator/TouchGuard/releases)**

*NOTE: Must be run with administrative privileges.*

----------------
## Usage (non-tech savvy)
- Open Terminal, type "chmod +x ", drag the drop the downloaded file into the Terminal window, and press enter (this only needs to be done once after downloading the file).
- Then type "sudo", drag and drop the downloaded file into the Terminal window, type " -time 0.2" and press enter. You may be prompted for your password; if so, type it and presss enter. Note that you will not see the cursor move while typing your password -- this is normal and done for security reasons.
- Keep the terminal window open. If you close the window, the program will exit. You can hide the window by typing "command-h".
- To start TouchGuard automatically after every restart (no need to relaunch it by hand), use the installer described in the "Automatic startup (survives reboot)" section below.

------------------
## Sample command line usage (for the more tech-savvy)
```
# make the downloaded release file executable
chmod +x TouchGuard
# run it
sudo ./TouchGuard -time 0.2
```

The above launches TouchGuard with a time interval of 200 ms (disables the touchpad for 200 ms each time a key is pressed on the keyboard). I have found this to be effective for me -- if you are still having issues (e.g. you can't use the trackpad immediately after typing, or your cursor still jumps), you can adjust the time interval up or down as needed.

*Note: to run TouchGuard automatically at login (and have it survive reboot) instead of relaunching it by hand, use `./install.sh` - see the "Automatic startup (survives reboot)" section below.*

----------------
## Automatic startup (survives reboot)

`install.sh` installs TouchGuard and can register it as a per-user **LaunchAgent** so it starts automatically at every login and keeps running across reboots.

```
# from the repo root
./install.sh                 # interactive: asks whether to enable auto-start
./install.sh --auto-start    # install and enable auto-start at login
./install.sh --time 0.3      # set the disable interval in seconds (default 0.2)
./install.sh --uninstall     # remove the binary, the agent, and the grant
```

On macOS 10.14 (Mojave) and later, suppressing trackpad input requires **Accessibility permission** instead of running as root. After the installer loads the agent, enable **TouchGuard** under **System Settings > Privacy & Security > Accessibility**, then log out and back in (or run the `bootout`/`bootstrap` command the installer prints). From then on it starts on its own.

Notes:
- It installs as a **LaunchAgent**, not a LaunchDaemon, on purpose: an active event tap needs a logged-in GUI session, which a boot-time daemon does not have. That is why putting it in `/Library/LaunchDaemons` never worked on recent macOS - the tap could not be created and the process exited immediately.
- The binary is ad-hoc code-signed with a stable identifier so the Accessibility grant stays attached. If you reinstall or update the binary, you may need to re-enable it under Accessibility once.
- Log file: `~/Library/Logs/TouchGuard.log`.

----------------
## Support
Questions? Comments? Feedback? Issues? Open a new issue [here](https://github.com/thesyntaxinator/TouchGuard/issues) or email syntaxsoftsupport@icloud.com.

