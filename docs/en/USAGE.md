# Starting the game and controller controls

[한국어](../ko/USAGE.md) · [English](../en/USAGE.md) · [日本語](../ja/USAGE.md) · [简体中文](../zh-CN/USAGE.md)

> **Required before using VR: Turn OFF anti-aliasing in the game's settings.**

1.1.3 delivers the default-wrapper 3DMigoto interoperability changes from PR #1 in both Clean and Dirty. Game wrappers remain intact; VR copies and OpenXR use the native D3D11 device/context. Matching Configurator and Installer metadata ships with the DLLs.

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


## Startup order

1. Make sure your headset and controllers are connected, then start SteamVR.
2. If needed, save your options in the configurator and close it.
3. Start the game as you normally would. The installer does not start it for you.
4. When using controllers to operate the game, make sure the game window is selected on the PC.

To apply new settings, close and restart the game. If the display or input behaves unexpectedly, see the [troubleshooting guide](TROUBLESHOOTING.md).

## Front panel and auxiliary panel

Normal menus and 2D screens appear on a front panel. In supported 3D scenes, stereoscopic VR is the main view, and you can open the original game screen as an auxiliary panel when needed.

With the default settings, **left Grip** opens and closes the auxiliary panel in 3D scenes. It does not hide the front panel on normal 2D screens. If you have changed hand roles, use Grip on the hand assigned to the auxiliary panel.

## Basic controls

The following assumes that you have not changed hand roles.

| Control | Action |
|---|---|
| Left stick | Move the VR viewpoint |
| Right stick | Turn the view. The default is snap turning in 30° steps. |
| Right-hand pointer | Point at items on the game screen. |
| Right Trigger or A | Click the pointed item |
| Right B | Back / cancel |
| Left Grip | Open or close the auxiliary panel in 3D scenes |

To snap turn again, return the stick to the center before tilting it again. To drag, hold the click while moving the pointer.

These controls change if you change hand roles or swap the click and Back buttons in settings. The hands used for movement and turning sticks are configured separately from the panel and pointer hands.

## Game control mode: WASD, F and Escape

**Hold Trigger on the auxiliary-panel hand** to enter game control mode. With the default settings, this is left Trigger.

- The movement stick sends **WASD** input to the game instead of moving the VR viewpoint.
- **X or A** sends **F**, and **Y or B** sends **Escape**.
- Release Trigger to return to normal VR movement and UI controls.

You can use this mode without opening the auxiliary panel. This Trigger is separate from the pointer-hand Trigger used for normal UI clicks. If input does not work, first make sure the game window is selected.

## Moving your viewpoint with world grip

World grip is off by default. To use it, close the game, enable it in the configurator's **World grip** tab, save and restart the game.

With the default selection of right Grip and right-hand tracking, hold right Grip and move your hand to adjust your viewpoint as if grabbing and moving the world. Release the button to stop, and press it again to continue.

You can change the activation buttons and tracked hands. However, the auxiliary panel's Grip, the game control mode's Trigger and pointer UI controls can take priority. See the [settings guide](SETTINGS.md) as well.

## Stick clicks: return viewpoint and camera following

The following controls work in the 3D VR view. They mean **pressing the stick down as a button**, rather than tilting it.

| Control | Action |
|---|---|
| Physical left stick click | Returns a viewpoint changed by world grip or stick movement to the current original-camera reference. |
| Physical right stick click | Toggles whether to follow the original camera's movement. |

The left and right click roles do not change when you change the movement-hand setting. Camera following controls whether to follow the original camera's movement; it is separate from looking around with your head or moving manually.

To use world grip again after returning your viewpoint, release any Grip you were holding and press it again. Camera following toggled with right stick click lasts only for the current game session. Save its startup value separately in the configurator for the next launch.

See the [settings guide](SETTINGS.md) for world scale, linked stereo depth and movement speed, and startup camera following.
