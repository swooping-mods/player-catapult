# Player Catapult: behavior and implementation

Describes the retained payload **0.2.0**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.2.0** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.2.0** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

Buy the $750 catapult and select its Equipment slot. P deploys it, E launches from the rear bucket and O packs it. Deployment requires four level support samples and clear capsule space. Launch additionally requires bucket proximity, a clear initial corridor and an unseated, unclimbing, non-flying host. Native Rigidbody.AddForce with VelocityChange applies the launch; the arm animates and resets.

## State and lifecycle

Ownership persists in the shared equipment-provider sidecar; native quickslot IDs persist normally. The deployed structure/arm are session-local and remain until packed or world teardown, even when another tool is selected. Primitive colliders are disabled; bucket interaction uses proximity. Focus, host and world gates guard actions.

## Integration and configuration

Runtime dependencies: [shop-framework](https://github.com/swooping-mods/shop-framework), [equipment-framework](https://github.com/swooping-mods/equipment-framework), [hints-framework](https://github.com/swooping-mods/hints-framework).

The [module metadata](../source/mods/player-catapult/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Requires `shop-framework.shop` | 1 | at least 56 bytes |
| Requires `equipment-framework.equipment` | 1 | at least 32 bytes |
| Requires `hints-framework.hints` | 1 | at least 24 bytes |

Packaged defaults in [settings.ini](../source/mods/player-catapult/settings.ini); supported bounds and interpretation remain in the linked parser.

| Section | Setting | Default |
|---|---|---|
| `(root)` | `enabled` | `1` |
| `(root)` | `equipment_key` | `121` |
| `(root)` | `action_key` | `101` |
| `(root)` | `speed` | `38` |

## Implementation sources

- [mod.c](../source/mods/player-catapult/mod.c)
- [settings.h](../source/mods/player-catapult/settings.h)
- [tool_runtime.h](../source/mods/player-catapult/tool_runtime.h)

## Verification boundary

No player teleport, replacement gravity, saved structure or remote launch protocol. Support clearance, flight/landing and collision safety need game-level qualification.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
