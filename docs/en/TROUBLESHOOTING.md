# Troubleshooting

[한국어](../ko/TROUBLESHOOTING.md) | [English](../en/TROUBLESHOOTING.md) | [日本語](../ja/TROUBLESHOOTING.md) | [简体中文](../zh-CN/TROUBLESHOOTING.md)

[Home](../../README.en.md)

## The eyes are misaligned or VR looks wrong

First turn in-game **anti-aliasing OFF**. It caused reported eye misalignment on Quest 3 and Galaxy XR. Check SteamVR/OpenXR and the headset connection, fully close the game and launch it again.

## The game folder or installation package cannot be found

Select `GF2_Exilium.exe` or its folder. Haoplay and Steam layouts are supported. Extract the full latest Clean/Dirty ZIP to a fresh folder and run its installer. A `Source code` ZIP or an installer copied on its own is not a complete package.

## A file is modified or has no ownership record

The existing `version.dll` may differ from its installation record or belong to another mod. This is separate from a game-original hash error. An exact match to the incoming package can be adopted with a backup even without an old receipt, but different or unverified files remain protected. Keep files, records and backups; do not bypass integrity checks. Check the other mod’s removal instructions.

## Input, the auxiliary panel or world grip does not work

Focus the game window on the PC and check controller connections. Default left Grip toggles the 3D auxiliary panel; holding left Trigger uses WASD game control mode. Grip does not hide the normal 2D front panel. World grip defaults to OFF: enable it, choose an activation button and tracked hand, save and restart. See [full controls](USAGE.md).

## Saved settings, hands or contact do not take effect

Save and restart the game. Importing a profile or restoring defaults also requires Save. Check hand visibility; contact requires Dirty and supported character bones. Toggling camera following with right-stick click changes the current session, not the saved startup value. See [settings](SETTINGS.md).

## The image stutters or frame rate is low

Check whether the PC view stalls too and which scenes trigger it. If it started after changing VR render scale, return to the last working value. Enable logs only for investigation and record the game, headset, connection and symptoms. A fixed frame rate is not guaranteed on every device.

## Updating, Open settings or installation fails

The current updater needs no CLI or login. For network or older-client problems, use the full-ZIP route in [updating](UPDATE.md). If Open settings is disabled, recheck the game location and complete package. Follow running-game/file-access messages. See [installation](INSTALLATION.md) for interrupted-operation recovery and previous-install restore.

## Information to include in a report

Include variant/version, Haoplay or Steam, headset/connection, affected screen, reproduction steps and exact error text. If needed, enable diagnostic logs and restart to collect them. Remove account/authentication information before sharing; do not send the entire game installation.
