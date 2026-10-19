# Using the configurator

[한국어](../ko/SETTINGS.md) · [English](../en/SETTINGS.md) · [日本語](../ja/SETTINGS.md) · [简体中文](../zh-CN/SETTINGS.md)

## 1.1.6 — Automatic update fix

The settings GUI checks public GitHub releases at startup. Approve **Update** in the notification to download, verify and install. Starting with 1.1.6, GitHub CLI and sign-in are no longer required.

Fixed rejection of new native builds by the old clients frozen allowlist. The client verifies release/file hashes, build flavor and current ownership; the verified incoming Installer owns final native validation, backups and installation. Clean/Dirty flavor and settings are preserved.

For 1.1.4 or earlier, extract the complete desired Clean/Dirty ZIP into a new folder and run its new GF2VR.Installer.exe once. Existing 1.1.5 installations with working update checks can use the GUI updater. Users whose old version lacks its required GitHub CLI also need this one-time ZIP installation. After installing 1.1.6, GUI updates need neither CLI nor sign-in.

This release retains the exact 1.1.5 VR DLLs. Logging remains OFF by default; explicit ON and its permanent initial-cleanup exemption, panel and contact settings are preserved. Close the game and settings window before installation.

## 1.1.5 — Logging controls

Added **Diagnostics → Automatically record logs** to the settings GUI in both Clean and Dirty. The default is **OFF**, including when older settings omit this option. Save and restart the game to apply.

On the first update installation, previous automatic VR logs are cleaned once for users with logging OFF. **Users with logging ON keep their existing logs and are excluded from this initial cleanup.** Switching OFF later does not trigger a deferred deletion. Settings, other mods and manually collected evidence are preserved.

Enable logging when troubleshooting. With logging ON, the same build log is appended and may grow over time.

For 1.1.5, extract the entire desired Clean or Dirty ZIP into a new folder and use its new GF2VR.Installer.exe. Older settings programs may reject the new native profile. Existing settings are preserved.

## 1.1.4

### Auxiliary panel — common

Choose Face the viewer and Apply manual angles while facing viewer independently. With manual-angle application off, yaw/pitch/roll and screen tilt apply only when Face the viewer is off; inactive values are retained. Position and maximum width/height always apply. The panel preserves its aspect ratio within both size limits.

Defaults: Face the viewer ON, apply manual angles while facing OFF. Position X/Y/Z = 0 / -0.10 / 0.01m; pitch/yaw/roll = -67 / 3 / 0 degrees; screen tilt 45 degrees; maximum width and height 0.66m each.

Position follows the controller local axes. In the observed held pose, +X is right / +Y forward / +Z up; the axes rotate with your hand. Screen tilt uses the final screen horizontal axis: positive upward, negative downward. Existing settings without an auxiliaryPanel object retain the previous layout until panel settings or defaults are applied.

### Contact feedback — Dirty

The Hands tab controls contact-vibration strength and duration. It pulses once at initial contact and rearms after full separation. Defaults are 15% / 30ms; zero disables vibration.

The Contact jiggle tab provides separate breast/buttock controls. Values below are strength / duration / range / tilt / stretch.

| Region | Defaults |
|---|---|
| Breasts | 40% / 0.4x / 75% / 50% / 45% |
| Buttocks | 120% / 0.5x / 120% / 120% / 140% |

Explicit saved values are preserved; regional values override legacy common values. Arbitrary upper limits are removed while finite-number, positive-duration and positive-compression-scale checks remain. Restart the game after saving.

## 1.1.1 — Clean / Dirty

| Feature | Clean | Dirty |
|---|---|---|
| Ordinary VR, stereo, controller input and settings | Included | Included |
| White spheres at both hand positions | Included | Included |
| Character breast contact and deformation | Excluded | Included |

Both variants share ordinary VR source, versions and updates. Only Dirty includes contact deformation code; Clean excludes that implementation rather than merely turning it off.

Installation, switching and removal preserve user settings. Optional touch_interaction.json is used only by Dirty. Remove restores a recorded previous installation when available; otherwise it removes only owned files. Modified or unowned DLLs are preserved and refused. Do not copy version.dll alone or mix files from different packages.

The reported binocular misalignment on Quest 3 and Galaxy XR was confirmed to be caused by anti-aliasing. Turn OFF anti-aliasing in the game's settings before using VR.

The remaining detail is retained from earlier versions. Use the 1.1.4 download and verification scope above as current authority.


Use **Open settings** in the installer to open the configurator. You can also run `vrmod/bin/GF2VR.Configurator/GF2VR.Configurator.exe` in the installed game folder directly. Keep the files installed alongside the executable in the same folder. Change the display language with the **한국어 / English / 日本語** buttons at the bottom; your selection is remembered.

## Applying changes

1. Close the game.
2. Adjust the settings you want. Read the descriptions beneath the settings as well.
3. Click **Save** and check the completion message.
4. Start the game again yourself.

Changing a number in the settings window does not immediately apply it to a running game. Enter numbers without units, using a period for the decimal separator, for example 0.5.

## Quality and stereo depth

**VR render resolution scale** controls the resolution used to draw the VR view. Higher values can make it sharper, but increase performance and memory demands. Lower values can make it blurrier. This setting does not change the resolution of the PC screen or auxiliary panel.

The default is 1.0. If a problem occurs, return to a value that previously worked. The same value may not work on every device. Visual effects use the game's defaults; individual effect on/off controls are not provided.

## Movement and turning

| Setting | Description |
|---|---|
| Enable stick movement | Whether controller sticks can move the VR viewpoint. |
| Movement hand | Left by default. Changing it swaps the movement and turning stick roles together. |
| Enable stick turning | Whether the opposite hand's stick can turn the view. |
| Turn mode | Snap turns in fixed steps; smooth turning continues while the stick is tilted. |
| Movement speed | Base stick movement speed. The default is 1.0. |
| Manual movement speed multiplier | Adjusts the base movement speed. 1.0 leaves it unchanged, 2 doubles it and 0.5 halves it. |
| Snap turn angle | Angle of each snap turn. The default is 30°. |
| Smooth turn speed | Speed of continuous turning. Not used in snap mode. |
| Stick deadzone | Range in which small stick movements are ignored. Check this if movement continues after you release the stick. |
| Follow camera movement at startup | Whether to follow the original camera's movement when the game starts. On by default. |

Changing the movement hand is separate from swapping the panel and pointer hands in the **Buttons** tab.

Toggling camera following during play with **right stick click** does not change the saved startup setting. Save it in this window if you want the same state at the next launch.

## World grip

World grip moves the VR viewpoint by moving your hand while holding a button. It is off by default, so enable it first if you want to use it.

- **Enable world grip**: Turns the feature on or off.
- **Activation buttons**: Select which left or right Grip or Trigger buttons to use.
- **Tracked hands**: Select which hand movements to use.

Select at least one activation button and at least one tracked hand. The defaults are right Grip and right-hand tracking.

On the auxiliary-panel hand, Grip gives priority to opening and closing the panel, and Trigger is used for game control mode. On the pointer hand, Trigger gives priority to UI controls. The default right Grip combination is an easier starting point for learning how the feature works.

## Activity UI

Turning off **Show Activity UI in the main view** hides the Activity UI overlaid on the main VR view. The 3D view and auxiliary panel showing the original game screen remain available.

Even when enabled, the main-view UI is temporarily hidden while the auxiliary panel is open, and reappears when the panel is closed.

## Buttons

| Setting | Off: default | On |
|---|---|---|
| Swap click and Back buttons | A or X on the pointer hand clicks; B or Y goes Back | Swaps the two actions. |
| Swap panel and pointer hands | Left hand controls the auxiliary panel; right hand is the pointer | Right hand controls the auxiliary panel; left hand is the pointer. |

Changing the panel hand also moves its panel Grip and game-control-mode Trigger to that hand. It does not change the stick movement and turning hands.

The **X/A → F, Y/B → Escape** inputs in game control mode are separate from the normal click/Back swap.

## World scale

**World scale multiplier** adjusts how large the world around you feels. **1 means 100%, 2 means 200%, and 0.5 means 50%.** Enter 2 for 200%.

| Setting | Description |
|---|---|
| World scale multiplier | Adjusts the size of the world. Start with small changes from the default of 1. |
| Link Stereo Depth to world scale | Automatically adjusts the in-game separation of the two eyes to match world scale. On by default. |
| Stereo Depth / eye separation multiplier | Manually adjusts in-game eye separation, which controls stereo depth. While automatic linking is enabled, it only displays the current value and cannot be edited. |
| Link movement speed to world scale | Makes movement proportionally faster when the world is larger and slower when it is smaller. On by default. |

Turning off automatic eye-separation linking lets you adjust it manually, starting from the value calculated at that point. Turning it back on calculates it automatically from the current world scale. This setting does not replace your headset's physical eye-spacing adjustment.

Turning off movement-speed linking preserves the base speed and manual multiplier in the **Movement and turning** tab. Only the automatic adjustment for world scale is removed.

The **Head movement** readout at the bottom shows head movement compensated for world scale. It is not stick movement speed and is not a separate input field.

## Save and profile buttons

| Button | How to use it |
|---|---|
| Save | Saves your current settings. Restart the game to apply them. |
| Reload | Loads the last saved settings. Unsaved changes are discarded. |
| Defaults | Resets the settings to their defaults. Save afterward to apply them. |
| Export | Stores the current settings in a separate profile file. Use it to switch between combinations or keep a backup. |
| Import | Loads a saved profile. Check its contents and save before the game settings are updated. |

The recommended ranges beside numeric fields are guidance for choosing a starting point. Values outside them are not always blocked, but values a device or feature cannot handle may not be applied. If an error appears, read the message and return to the previous value or default.
