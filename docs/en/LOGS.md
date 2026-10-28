# Saving and sending logs

[한국어](../ko/LOGS.md) | [English](../en/LOGS.md) | [日本語](../ja/LOGS.md) | [简体中文](../zh-CN/LOGS.md)

[Home](../../README.en.md)

When you report a problem, attaching the VR mod's diagnostic log helps a lot in finding the cause. Logging is off by default, so turn it on with the steps below, run into the problem again, and then send the file.

## 1. Turn on logging

1. Close the game.
2. Open the settings program: press **Open Settings** in the Installer, or run `vrmod/bin/GF2VR.Configurator/GF2VR.Configurator.exe` in the game folder.
3. On the **Diagnostics** tab, check **Automatically record logs** and press **Save**.
4. Start the game again. Saved settings take effect only after the game restarts.

## 2. Reproduce the problem

- Go to the screen where the problem happens, reproduce the same symptom once, then close the game.
- Each game run is appended to the same log file, and one run can exceed 100 MB. To capture only this problem, move the existing log file to another folder before starting the game.
- If the game closes suddenly, the last ~0.25 seconds may not be recorded.

## 3. Open the log folder

- **From 1.3.3:** on the **Diagnostics** tab of the settings program, press **Open log folder** to open it in File Explorer. The location is shown below the button. If no log has been created yet, a message explains what to do.
- **Open it yourself:** it is the `vrmod\logs\bootstrap` folder under the path shown in **Game folder** at the top of the settings program.
  - If `GF2_Exilium.exe` is directly in the game folder (as in the Steam version), it is `<game folder>\vrmod\logs\bootstrap`.
  - If `GF2_Exilium.exe` is inside a `GF2 Game` folder (as in the Haoplay version), it is `vrmod\logs\bootstrap` under the parent folder of `GF2 Game`.

## 4. Choose the file to send

- Pick the `.jsonl` file starting with `GF2VR-CLEAN-` or `GF2VR-DIRTY-` that has **the most recent Date modified**. If a `.inventory.jsonl` file with the same name exists, send it too.
- File names change between versions. For example, in 1.3.3 the Dirty log is `GF2VR-DIRTY-133-006.jsonl` and the Clean log is `GF2VR-CLEAN-130-002.jsonl`; in 1.3.1 the Dirty log was `GF2VR-DIRTY-130-004.jsonl`.
- Compress the selected files into a ZIP. On Windows 11: right-click → **Compress to** → **ZIP File**. On Windows 10: right-click → **Send to** → **Compressed (zipped) folder**. Logs are text and shrink a lot (for example, about 100 MB → about 5 MB).

## 5. Send it to the developer

- Create a new issue in [GitHub Issues](https://github.com/deadpixel134/GF2Exilium-VR/issues/new) and drag the ZIP file into the text box to attach it. GitHub accepts attachments up to 25 MB.
- Or post in the [Arca Live VR channel](https://arca.live/b/vrshits).
- If the file is too large, upload it to cloud storage such as Google Drive and include the share link.
- Please also include:
  - Variant (Clean/Dirty) and version, Haoplay or Steam
  - Headset and connection (for example, Quest 3 + Virtual Desktop)
  - The affected screen, reproduction steps and any error message shown
  - The approximate time when the problem happened

## 6. Before you share

- GitHub issues and Arca Live posts are visible to everyone.
- The log records how the VR mod ran (events, states and performance numbers). Send only the log files; do not send original game files, account or authentication information, or other files you were not asked for.

## 7. Turn it off afterwards

When you are done, uncheck **Automatically record logs** on the **Diagnostics** tab and save. If left on, the log file keeps growing. You can delete log files you have already sent.

[Settings](SETTINGS.md) · [Troubleshooting](TROUBLESHOOTING.md)
