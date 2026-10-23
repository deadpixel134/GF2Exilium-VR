# Install, switch and remove

[한국어](../ko/INSTALLATION.md) | [English](../en/INSTALLATION.md) | [日本語](../ja/INSTALLATION.md) | [简体中文](../zh-CN/INSTALLATION.md)

[Home](../../README.en.md)

## Before you start

- A working **Haoplay or Steam installation** of the game. The game is not included.
- A VR headset and controllers with a working **SteamVR/OpenXR connection**.
- In-game **anti-aliasing OFF**. It was identified as the cause of reported eye misalignment on Quest 3 and Galaxy XR.

## Choose a package

Choose a full Clean or Dirty ZIP under **Assets** on the [latest release](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest). General VR, controller input and white hand markers are shared. Character deformation and contact haptics are Dirty-only. Install one variant at a time. Do not use GitHub’s `Source code` ZIP.

## Installation steps

1. Download **one Clean or Dirty ZIP** from the [latest release](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest) and extract the entire archive to a fresh folder. The `Source code` ZIP is not an installer package.
2. Close the game and any existing configurator/installer. Run the extracted `GF2VR.Installer.exe`, select `GF2_Exilium.exe` or the game folder, and install.
3. Open settings and save your options. Prepare SteamVR and your headset, then launch the game through your usual method.

## Installed files

The installer places `version.dll` beside the game executable and shared support files, including the OpenXR loader and configurator, under `vrmod`. Original `GameAssembly.dll`, `UnityPlayer.dll` and game assets are not replaced. Keep the full package together; do not move individual executables/DLLs or mix files from different packages.

## Switching Clean and Dirty

Extract the full latest ZIP of the desired variant into a fresh folder and use its installer on the same game location. Do not stack the two variants. Compatible user settings are preserved; Clean does not read Dirty’s optional `touch_interaction.json`.

## Removal and previous-install restore

Close the game, select the same game location in the installer and review the removal action. A recorded previous installation is restored when available; an initial installation removes only owned files. User settings and modified or unowned files are preserved. Keep the installation records and backups. Use interrupted-operation recovery if offered.

[Updating](UPDATE.md) · [Troubleshooting](TROUBLESHOOTING.md)
