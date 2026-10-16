# Installation, removal and recovery

[한국어](../ko/INSTALLATION.md) · [English](../en/INSTALLATION.md) · [日本語](../ja/INSTALLATION.md) · [简体中文](../zh-CN/INSTALLATION.md)

Download **GF2VR-Native-SelfContained-1.0.2.zip** from [official version 1.0.2](https://github.com/deadpixel134/GF2Exilium-VR/releases/tag/v1.0.2). GitHub's **Source code** ZIP is not an installation package.

## Before installing

- A legitimate game installation and VR environment on Windows x64 are required. **2.0.3936** is the practically tested baseline. Original-game SHA-256, file-size or cache differences do not block installation; this does not guarantee VR compatibility with other game versions.
- Fully close the game.
- Extract the entire downloaded archive into a new folder. Do not run it directly from inside the ZIP.
- Keep the included files and folders together. Do not move the installer executable on its own.

The installation package includes the required .NET runtime. You must obtain the game, headset software and SteamVR separately.

## Installing

1. Run **GF2VR.Installer.exe** from the extracted folder.
2. Click **Browse…** beside **Game folder**. Select the **folder containing GF2_Exilium.exe** or an installation folder containing it. The installer finds the executable through different folder names and nested layouts. If it finds several matches, select a narrower folder for the intended game.
3. Installation files are found automatically beside the installer. There is no separate package path to select. If files are missing, fully extract the release ZIP into a new folder and run its included installer.
4. Check the displayed version and list of files to be processed. If the screen says it will **install only the configurator**, this is not a package for installing the complete VR mod.
5. Click **Install** and confirm that the displayed location is correct. The button may say **Update** or **Reinstall**, depending on the existing installation.
6. Wait for the completion message. Once the operation finishes, you can use **Open settings** to open the configurator.

Do not close the installer or move installation files while an operation is in progress. Start the game and SteamVR yourself after completion. See the [settings guide](SETTINGS.md) if you want to check your settings first.

Installation may be refused if another program's version.dll is present or product-package hash or ownership checks fail. An original-game hash difference alone does not cause refusal. Do not bypass file protection by overwriting or deleting existing files; see [Troubleshooting](TROUBLESHOOTING.md).

The standard `GF2 Game/GF2_Exilium.exe` layout keeps `vrmod` in its existing parent folder. Other layouts use `vrmod` in the located executable's folder. `version.dll` is always installed beside that executable.

## Buttons

| Button | What it does |
|---|---|
| Install / Update / Reinstall | Applies the selected installation package to the game folder. |
| Open settings | Opens the configurator matching the current installation. Disabled when unavailable. |
| Refresh | Checks the selected folders and installation status again. |
| Remove | Processes files installed by the installer. If a previous version is retained, you can return to it. |
| Recover interrupted operation | Restores the previous file state when installation or removal was interrupted. |

## Removing the mod or returning to a previous version

Close the game, confirm the game folder in the installer, then use the removal button. If a previous version is retained, the button is labeled **Restore previous version**. Read the confirmation message before continuing.

Removal deletes newly created files and restores preexisting files from backups. User settings and unrelated mods remain. If identical manually copied files were adopted, removal restores those files and can leave the manually installed mod present. Restoring a previous version also differs from completely removing the mod.

If modified files cause the operation to be refused, do not forcibly delete them. Refer to the [troubleshooting guide](TROUBLESHOOTING.md).

## If an operation was interrupted

1. Make sure the game is closed.
2. Open the installer again with the same game folder.
3. If **Recover interrupted operation** is available, run it.
4. Check the result, then try installation or removal again.

**Recover interrupted operation** cleans up an interrupted operation. **Restore previous version** returns to a version that was previously installed. If recovery fails, check the error message and operation log before repeatedly changing files.

This is not an original-game version or SHA-256 error. The existing `version.dll` differs from its installation record, or has no record after a manual copy or lost receipt, so it was preserved. In 1.0.2, a file without a receipt can be installed over only when it exactly matches the current package's final installed file. It is backed up first and restored on removal. Different DLLs and modified files remain protected. Keep installation records and backups, and follow the existing mod's removal instructions. Receiptless files copied from an older release are not automatically adopted when their contents differ from the current package.
