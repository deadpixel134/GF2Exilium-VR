# Updating

[한국어](../ko/UPDATE.md) · [English](../en/UPDATE.md) · [日本語](../ja/UPDATE.md) · [简体中文](../zh-CN/UPDATE.md)

The current official version is **1.0.2**. It improves installation when folder layouts or original-game fingerprints differ. Download the release ZIP and update with its installer. Changes to source code or documentation do not automatically update installed programs.

**To upgrade from 1.0.0, download the 1.0.2 ZIP and run the installer from its freshly extracted folder.** Select the existing game folder to update while preserving settings and installation records. The old configurator may reject the new package profile; use this manual route if it does. Automatic replacement by the old configurator for this release has not been verified.

## Updating with a release ZIP

1. Download **GF2VR-Native-SelfContained-VERSION.zip** for the official version you want from the [releases page](https://github.com/deadpixel134/GF2Exilium-VR/releases).
2. Close the game and fully extract the new ZIP into a separate folder.
3. In the new installer, confirm only your existing game folder. Installation files are selected automatically.
4. Check the displayed version and **Update** prompt, then continue.
5. Once finished, use **Open settings** to check your existing settings, then start the game yourself.

See [Installation](INSTALLATION.md) for a detailed walkthrough. User settings are preserved. Avoid copying only some files over an older installation and mixing versions.

## Checking for updates in the configurator

This feature uses the **GitHub CLI (gh)** installed on your PC and its login state. If you do not use the CLI, update through the public ZIP download described above. You do not need to put tokens or passwords in GF2VR settings files.

## If an update notification appears

1. Check for a new-version notification when you open the configurator.
2. Close the game if it is running.
3. Click the update button and confirm the offered version.
4. If you agree to proceed, wait while the program downloads and processes the files.
5. When the configurator closes and reopens, check the result. Restart the game yourself.

The update button appears when an available new version has been confirmed. If the window does not reopen or an error appears, open the configurator or installer from its existing location and check its status.

## If no notification appears

There may be no new version, your current installation method may not support automatic updates, or repository access or the internet connection may be unavailable. The absence of an update button alone does not mean your installation is broken.

Version 1.0.0 can update to 1.0.2. If you already have 1.0.2 and no newer official release exists, no notification is expected. Drafts and prereleases are not selected as official updates.

If update checking or file verification fails, the existing installation is retained. If the problem continues, check the messages using the [troubleshooting guide](TROUBLESHOOTING.md).

## Interrupted updates and returning to a previous version

If an update was interrupted, use [interrupted-operation recovery](INSTALLATION.md). If problems appear after an update completes, check the installer's instructions for returning to the previous version. Close the game before recovery or removal as well.
