# Troubleshooting

[한국어](../ko/TROUBLESHOOTING.md) · [English](../en/TROUBLESHOOTING.md) · [日本語](../ja/TROUBLESHOOTING.md) · [简体中文](../zh-CN/TROUBLESHOOTING.md)

> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

The VR DLLs and shared Configurator payload are identical to 1.1.1. This version updates Installer guidance only.

## 1.1.1 — Clean / Dirty

| Feature | Clean | Dirty |
|---|---|---|
| Ordinary VR, stereo, controller input and settings | Included | Included |
| White spheres at both hand positions | Included | Included |
| Character breast contact and deformation | Excluded | Included |

Both variants share ordinary VR source, versions and updates. Only Dirty includes contact deformation code; Clean excludes that implementation rather than merely turning it off.

Installation, switching and removal preserve user settings. Optional touch_interaction.json is used only by Dirty. Remove restores a recorded previous installation when available; otherwise it removes only owned files. Modified or unowned DLLs are preserved and refused. Do not copy version.dll alone or mix files from different packages.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

The remaining detail is retained from earlier versions. Use the 1.1.1 download and verification scope above as current authority.


First identify where the problem occurs. Settings saving, installation or updates, and the actual VR display or input each require different checks.

## The game folder cannot be found

Select the **folder containing GF2_Exilium.exe** or an installation folder containing it. The installer finds the executable through different folder names and nested layouts. If it finds several matches, select a narrower folder for the intended game. If the selected tree is too broad to search completely, choose a folder closer to the executable. Select only the game location in the installer. The extracted installation files are located automatically.

## Package files are missing or installation is unavailable

- Check that you have extracted the entire ZIP first.
- Make sure you have not moved only the executable elsewhere.
- Check that you are not using GitHub's source code ZIP as the installation package.
- Use the installation ZIP from the [1.0.2 release](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2). Original-game SHA-256, size or cache differences are nonblocking; corruption or hash mismatches in the product package still block installation.

If files are missing or damaged, download the release ZIP again and fully extract it into a new folder. Do not modify files rejected by the installer to try to bypass its checks.

## A file was modified or has no ownership record

This is not an original-game version or SHA-256 error. The existing `version.dll` differs from its installation record, or has no record after a manual copy or lost receipt, so it was preserved. In 1.0.2, a file without a receipt can be installed over only when it exactly matches the current package's final installed file. It is backed up first and restored on removal. Different DLLs and modified files remain protected. Keep installation records and backups, and follow the existing mod's removal instructions. Receiptless files copied from an older release are not automatically adopted when their contents differ from the current package.

## The headset view is missing or stutters periodically

Check SteamVR/OpenXR and your headset connection, then fully close the game and restart it in your usual way. Checking whether the PC game view freezes at the same time can help narrow down the cause.

If this began after increasing the quality scale, return to the last value that worked. Version 1.0.0 includes a fix for repeated camera-warning calls that caused periodic stuttering in Activity scenes. If it continues, report the scene, the interval between stutters, and whether the PC view, HMD or both are affected. A fixed frame rate is not guaranteed across devices.

## The Open settings button is disabled

A configurator matching the current game installation could not be found. Check the game folder and click **Refresh**. Copying only the configurator or installer separately may leave required companion files missing.

If a launch path is provided, try opening it from that location. If the issue persists, check the error guidance rather than mixing program and package files.

## Settings cannot be saved, or installation fails

First make sure the game has fully closed. It may still be running even after its window has closed.

For a numeric error, check that you have not entered units or other characters in the field. Use a period for decimals. If a value is too large or cannot be handled by the device, return to the previous value or default.

For a file-access error, check that the displayed folder is correct and that another program is not using the file. If the operation stopped to protect modified files, back them up first and identify what changed.

## Saved settings do not change the game

Make sure you closed and restarted the game after saving. You must also click **Save** after importing a profile or selecting defaults.

Camera following toggled with right stick click applies only to the current game session. Save it in the configurator to change the startup state for the next launch.

## Auxiliary panel, clicking or stick controls behave unexpectedly

1. Check that your controllers are connected correctly.
2. Select the game window on your PC.
3. Check whether you changed the movement/turning hands or panel/pointer hands in settings.
4. If you are holding Trigger on the auxiliary-panel hand, release it. Holding it activates WASD game control mode.
5. For snap turning, return the stick to the center before tilting it again.

Use Grip on the panel hand to open and close the auxiliary panel in 3D scenes. This does not hide the front panel on normal 2D screens. See [Controls](USAGE.md).

## World grip does not move the view

The feature is off by default. Check that you enabled it, selected at least one activation button and at least one tracked hand, saved and restarted the game.

If your combination gives priority to another panel or UI action, try the default right Grip and right-hand tracking. Release any button you were holding and press it again.

## Updates cannot be checked

Check your internet connection, GitHub CLI installation and login state, and whether a new official version actually exists. You can also download the ZIP directly from the public [releases page](https://github.com/deadpixel134/GF2Exilium-VR/releases) to update. The existing installation is retained if update checking fails. See [Updating](UPDATE.md).

To upgrade from 1.0.0, download the 1.0.2 ZIP and use its included installer. If the old configurator rejects the package, this manual route preserves settings and installation records.

## Installation was interrupted, or the new version has problems

Close the game and reopen the installer with the same folder. Use **Recover interrupted operation** if an operation is pending. If problems appear after an update completes, check the instructions for restoring the previous version. See [Removal and recovery](INSTALLATION.md).

## Reporting a problem

Include the button you pressed, the sequence that caused the problem, the error message, your headset and connection method. For installation problems, the last entries in the operation log can help.

Before sharing screenshots or files, check that they contain no account information or credentials. You do not need to send complete original game files.
