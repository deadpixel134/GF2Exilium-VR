# Using GF2VR with 3DMigoto

This change keeps the game's D3D11 wrapper while using the underlying native
device and immediate context for private VR copies and OpenXR. Device checks
use canonical COM `IUnknown` identity instead of comparing interface addresses.

## Configuration

Use 3DMigoto's default wrapper mode. An omitted or commented `[System]` `hook`
entry already has the required behavior; no additional edit is needed in that
case. Additional function hooks such as `hook=recommended` or `hook=all` are
outside this change's supported configuration.

`calls` and `crash` control logging and dump handling; enabling them is not a
compatibility requirement. Skin-specific INI syntax and duplicate-rule warnings
remain separate issues.

## Implementation

- Obtain the native device using the swapchain's public `GetDevice`, falling
  back to its backbuffer's `GetDevice` when necessary.
- Validate COM identity against Unity's device, then retain the native device
  and immediate context for VR. Preserve the game's device/context wrappers.
- Apply the same identity rule to panel captures, destination textures, and
  controller cursor textures.
- Reject known wrapper interfaces on the supposed native device/context;
  installation failure reasons 11/12 indicate remaining function hooks.
- The `native_graphics_isolated` diagnostic value is 1 when the Unity and native
  device addresses differ, and 0 when they were already the same address.

The desktop tools pin the native DLL's hash. Copying a changed DLL alone can
cause Configurator `runtimeError` and disabled controls. Follow
[the metadata build procedure](BUILD_VARIANT_METADATA.md) to produce matching
runtime contracts, compiled variant profiles, and package manifests from the
actual DLL outputs. Preserve hash checks and installation transactions; do not
distribute a user's installed contract or receipt.

## Evidence and limits

Before preparing this PR, the user confirmed simultaneous VR and skin rendering
with the local Dirty 1.1.2 compatibility build. Earlier standalone D3D11 checks
covered wrapper command execution and native copy/query/readback. Configurator's
real installation check passed after matching metadata was installed. These
are historical local observations, not a claim that every injector fork, skin,
or headset was exercised. Clean has not been confirmed in the game with an HMD.

This contribution contains GF2VR source and documentation only. It includes no
3DMigoto program or configuration files, skins, game files, user settings,
logs, or installation receipts.
