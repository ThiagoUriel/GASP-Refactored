# Game Animation Sample Refactored (GASP)

Reworked and improved C++ architecture based on Epic's Game Animation Sample, powered by Mover 2.0 and NetworkPrediction.

## Architecture & Modules

The plugin is structured into six dedicated modules:

- **`GASP`** (Runtime): Core gameplay module containing the custom Mover component (`UGASPMoverComponent`), locomotion modes, obstacle traversal system (`UGASPTraversalComponent`), ragdoll simulation and tasks, multi-character interactions, foley audio subsystem (`UGASPFoleyWorldSubsystem`), and animation nodes.
- **`GASPCamera`** (Runtime): Gameplay camera interface (`IGASPGameplayCameraInterface`) and camera rig integration.
- **`GASPExtras`** (Runtime): Sample implementations including `AGASPCharacterExample`, player controller, input mappings, moving platforms, and test interactions (takedown, shove).
- **`GASPEditor`** (UncookedOnly): Editor tools, animation graph nodes (`UAnimGraphNode_GameplayTagsBlend`), and rig unit customizations.
- **`GASPNBS`** (Runtime): Foundry interaction definitions, handlers and the sample Pawn, plus overlay Presence modifiers and batched Presence reconciliation.
- **`GASPTests`** (Editor): Automation coverage for actions, multiplayer playback, overlays, Presence integration and the Crucible pawn bridge.

## Key Features

- **Mover 2.0 Locomotion**: Built natively on Epic's Mover framework with `NetworkPrediction` support for client-side prediction, rollback, and replication.
- **Dual Locomotion Pipelines**: Runtime-switchable between Motion Matching (Pose Search + BlendStack + Trajectory Prediction) and State Machine pipelines via console variable or configuration.
- **Obstacle Traversal System**: Multi-stage trace detection for obstacle height, depth, front/back ledges, and floor clearance. Executes Chooser-selected animations with dynamic Motion Warping targets (vault, hurdle, climb).
- **Physics-Driven Ragdoll & Get-Up**: Full ragdoll integration with slope auto-roll torque, impact direction prediction, Physics Control curves, and directional rolling or standing get-up montages evaluated through Chooser tables.
- **Synchronized Actions**: `UGASPActionComponent` selects and plays paired Mover animations for an explicit target. Foundry owns interaction discovery, focus, preparation and Commit through the existing GASPNBS module. See [Foundry interactions in GASP](Source/GASPNBS/README.md) for setup and the complete flow.
- **Foley & Surface Audio**: World subsystem (`UGASPFoleyWorldSubsystem`) driving surface-dependent MetaSounds, decals, and Niagara effects triggered by animation notifies.
- **Overlay & Layering System**: GameplayTag-driven stance, gait, and overlay management with linked animation layers.
- **Procedural IK & Rig Units**: Custom Control Rig units for slope alignment, toe ground alignment, hyperextension prevention, and pelvis clamping.

## Supported Engine Versions & Platforms

| Plugin Version | Unreal Engine Version |
|----------------|-----------------------|
| [1.3](https://github.com/Anaylan/GASP-Refactored/releases/tag/1.3) | 5.5 |
| [1.11](https://github.com/Anaylan/GASP-Refactored/releases/tag/1.11) | 5.6 |
| [1.12](https://github.com/Anaylan/GASP-Refactored/releases/tag/1.12) | 5.7 |
| [2.0](https://github.com/Anaylan/GASP-Refactored/releases/tag/2.0) | 5.8 |

Target platform is **Windows (Win64)**.

## Prerequisites & Required Plugins

This project version depends on **Foundry** and **Crucible**. It also relies on the following built-in Unreal Engine plugins enabled in `GASP.uplugin`:
- `Mover`, `NetworkPrediction`, `GameplayTasks`
- `PoseSearch`, `MotionTrajectory`, `MotionWarping`, `AnimationWarping`
- `Chooser`, `BlendStack`
- `GameplayCameras`, `EnhancedInput`
- `PhysicsControl`, `GameplayInteractions`
- `AnimationLocomotionLibrary`, `DeformerGraph`, `RigLogic`, `CurveExpression`

## Quick Start

1. Clone or extract the repository into your project's `Plugins` directory.
2. Ensure the required engine plugins listed above are enabled in your project.
3. Configure `Config/DefaultNetworkPrediction.ini` as described below.
4. Compile and launch your project editor.

## Configuration

GASP requires independent ticking policy settings in `Config/DefaultNetworkPrediction.ini` to avoid movement jittering under NetworkPrediction:

```ini
[/Script/NetworkPrediction.NetworkPredictionSettingsObject]
Settings=(PreferredTickingPolicy=Independent,ReplicatedManagerClassOverride=None,FixedTickFrameRate=60,bForceEngineFixTickForcePhysics=True,SimulatedProxyNetworkLOD=Interpolated,FixedTickInterpolationBufferedMS=100,IndependentTickInterpolationBufferedMS=100,IndependentTickInterpolationMaxBufferedMS=250,FixedTickInputSendCount=6,IndependentTickInputSendCount=6,MaximumRemoteInputFaultLimit=6)
```

## Documentation & References

- [CONSOLE_COMMANDS.md](CONSOLE_COMMANDS.md) - Runtime console variables and debug commands reference.\

## Coding Conventions

This fork follows [Foundry Coding Conventions](../../NinjaBearStudio/Foundry/CODE_GUIDELINES.md)
and [Foundry Test Code Guidelines](../../NinjaBearStudio/Foundry/Source/Tests/TEST_CODE_GUIDELINES.md).
Use `GASP` for gameplay type and file prefixes. `GASPNBS` identifies the fork's integration module;
keep it in module infrastructure and the `GASPNBS_API` export macro.

## License

Licensed under the MIT License. See [LICENSE.md](LICENSE.md) for details.

## Crucible pawn and equipment overlays

`AGASPCharacter` derives from `ACruciblePawn`, which derives from Foundry's lazy ability pawn.
Foundry owns ASC creation, profiles, PlayerState adoption, and avatar cleanup. Crucible supplies
combo, damage, faction, perception, and a pawn-local Gameplay Tasks Component. GASP retains Mover
and Physics Control, suppresses Crucible's optional Physical Animation Component, and stops Mover
locomotion intent when the Foundry death flow starts.

For equipment presentation:

1. Add **GASP Inventory Presence Component** to the visual pawn instead of the plain Foundry
   Presence component. It preserves Foundry's transport, batches each local reconciliation, and
   retries persistent Inventory association when a late PlayerState ASC becomes available.
2. On an item's Equipment fragment, add **GASP Overlay Presence Modifier** to its Equipped or
   Stored Presence as appropriate. Select the `GASP.Overlay.*` tags that the Overlay Chooser expects.
3. Configure the character's existing Overlay and Pose Choosers. They still evaluate the character
   as their context, with `GetOverlayMode()` exposing the combined manual and contributed tags.
4. Avoid configuring Foundry's direct Animation Layer modifier to control the same animation
   layers: GASP's Overlay Component coordinates their final linking and unlinking.

`UGASPOverlayComponent` replaces the old override manager implementation at the stable
`OverrideModeManager` subobject name. Existing character setters, getters, events, and authored
properties remain available. Native subclasses can use
`ObjectInitializer.SetDefaultSubobjectClass<UMyOverlayComponent>(OverlayComponentName)` to extend it.

`SetOverlayContribution(Source, Id, Tags)` is an idempotent source registration, with a matching
`RemoveOverlayContribution(Source, Id)`. The GASPNBS modifier uses its own identity plus the
Inventory and Item handles, so shared modifiers and repeated item definitions remain independent.
An empty tag set removes a registration. These local contributions never call the character's
manual-overlay RPCs; Presence already transports the equipment state to observers.

Chooser resolution runs against the complete effective tag set. Desired classes are deduplicated
across overlays and pose. Unchanged class stacks make no link/unlink calls. When a stack changes,
remaining classes are reasserted in deterministic tag order, with pose last, to restore layers hidden
by a removed override. This is class precedence, not automatic blending of incompatible graphs.
Null Chooser results are allowed. Desired intent survives missing/reinitialized animation instances;
mesh changes release old instances. Call `RefreshOverlay()` when other Chooser inputs change or
when pruning expired sources. `BeginOverlayUpdate()` / `EndOverlayUpdate()` batch multi-source edits.

The editor-only `GASPTests` module registers `NinjaBearStudio.GASP.Overlay` automation tests for
source ownership, Chooser results, pose sharing, atomic Presence updates, animation lifecycle,
PlayerState ASC handoff, and Mover death handling.
