# Updating and switching variants

[한국어](../ko/UPDATE.md) | [English](../en/UPDATE.md) | [日本語](../ja/UPDATE.md) | [简体中文](../zh-CN/UPDATE.md)

[Home](../../README.en.md)

## Update through the configurator

A normal **managed installation of 1.1.6 or later** checks public GitHub releases when the configurator opens. Approve **Update** in the notification to download, verify and install. The current update path does not require GitHub CLI or login.

Close the game before installation and follow the application’s exit instructions. A failed update check alone leaves the current installation intact.

## If the notification is missing or updating fails

1. Download the **full ZIP** for your current Clean or Dirty variant from the [latest release](https://github.com/deadpixel134/GF2Exilium-VR/releases/latest).
2. Extract everything to a fresh folder; do not mix package files.
3. Close the game and existing configurator/installer.
4. Run the new ZIP’s `GF2VR.Installer.exe`, confirm the existing game location and variant, then install.

Use this one-time route for 1.1.4 or earlier, or when an older updater cannot check releases. Publishing a new file cannot itself notify old clients that cannot reach the release source.

## Settings and recovery

Same-variant updates preserve compatible user settings and installation records. To switch Clean↔Dirty, use the full ZIP and installer for the desired variant. If integrity, ownership or running-game checks reject the operation, follow [troubleshooting](TROUBLESHOOTING.md); do not bypass them. See [installation](INSTALLATION.md) for interrupted-operation recovery and previous-install restore.

[Release history](RELEASE_NOTES.md) · [GitHub Releases](https://github.com/deadpixel134/GF2Exilium-VR/releases)
