# Foundry interactions in GASP

Foundry chooses the interaction and owns its execution. GASP chooses and plays the paired animation
for the exact participants Foundry selected. The integration lives in `GASPNBS`; core GASP playback
remains in `GASP`.

## Pawn setup

Use **GASP Foundry Pawn** (`AGASPFoundryPawn`) as the native base of the sample character Blueprint.
It keeps the camera and input behavior of **GASP Character Example**, and supplies three components:

| Component | Responsibility |
| --- | --- |
| Foundry Interaction Component | Discovers, focuses and performs interactions. Inherited from Crucible at the optional `InteractionComponent` subobject. |
| GASP Interaction Target Component | Offers the named `Shove` and `Takedown` slots, including their definitions and GASP handlers. |
| GASP Action Component | Checks whether the pair can act, selects the performance, reserves both participants and plays their roles through Mover. |

The Pawn also supplies a tagged scan sphere and target overlap sphere. Both participants need an
Action Component, a compatible animated mesh and GASP movement settings. The default feasibility
policy requires grounded participants, a reachable target in front of the source, and a source that
is not aiming. Override `CanSelectTarget` for different gameplay rules, or `SelectAction` for different
animation selection. Keep ownership and playback orchestration in the base component.

Define the **InteractionScanner** and **InteractionTarget** collision presets in the project. They
should use separate query-only object channels and overlap each other. The Pawn uses those presets
so Foundry's interaction collision controls can disable and restore the target shape. Lamplighter
includes this configuration in `Config/DefaultEngine.ini`.

Player possession enables scanning automatically. Target-only NPCs keep offering their slots without
running a scanner. Enable **Scan Without Player** on an AI Pawn that should also initiate interactions;
its ability profile must grant the same Scan and Execute abilities.

Register a **Foundry Ability Setup Data Asset** with profile tag `Ability.Profile.GASP` under
**Project Settings > Foundry: Gameplay Abilities > Ability System Profiles**. Grant **Interaction
Ability Scan** and **GASP Interaction Ability Execute**. Lamplighter's migrated setup is
`/Game/Data/Abilities/Setups/AS_GASPInteraction`. The Pawn uses this profile when it creates its ASC;
projects supplying a PlayerState ASC must grant the same abilities through that owner's setup.

## Foundry Input Handlers

The integration supplies **GASP: Move**, **GASP: Look** and **GASP: Jump** under
`GameFramework/Input/Handlers`. The sample and Lamplighter Pawn Blueprints provide
`/GASP/GASPNBS/Input/IS_GASPLocomotion` through **Input Setup Provider Interface**. This setup owns
the existing `IMC_Default` at priority 1 and uses the configured `IH_GASPMove`, `IH_GASPLook` and
`IH_GASPJump` assets. Foundry registers it on possession and removes its routes and context on
unpossession. The remaining sample actions keep their controller Blueprint bindings in that same
context. Move, Look and Jump no longer have controller Blueprint routes or native sample action methods.

| Handler | Input contract and behavior |
| --- | --- |
| GASP: Move | Axis2D on Triggered/Ongoing. Uses the sample's X-forward/Y-right axes and preserves magnitude. GASP applies controller yaw when consuming the input. Forward-reference and gait-scaling settings inherited from the generic handler are not used; mapping modifiers supply the dead zone. |
| GASP: Look | Axis2D on Triggered/Ongoing. Uses Foundry's pitch/yaw blocking and optional frame-time scaling, inverts pitch to match the sample, and suppresses camera input during twin-stick aiming. Do not add another pitch inversion to the existing sample mappings. |
| GASP: Jump | Boolean on Triggered, matching the sample's Pressed + Released action. True offers traversal first, then requests recovery, standing or a Mover jump. Normal release leaves the short pulse intact; Canceled clears it. Conventional actions can instead subscribe to the inherited Started/Completed hooks for immediate release. |

The Pawn implements Foundry's **Traversal Movement Input Interface**. Its existing traversal entry
point retains the movement-dependent trace settings and forwards execution to the Traversal Component.
Successful traversal suppresses the ordinary jump. Ragdoll recovery and standing are handled by the
non-traversal fallback. The Pawn owns the 0.1-second jump reset and clears it on cancellation, unpossession,
death and EndPlay. Setup removal also releases jump intent on the current Pawn; unpossession cleans
the previous Pawn independently.

Use separate Look actions/handlers when mouse deltas and gamepad rates require different frame-time
settings. The default handlers leave dead zones to the mappings, preserving small mapped values.
The migrated controllers leave their optional **Default Input Mapping** empty, so Foundry owns the
main context. The controller's fallback mapping code only removes contexts it installed itself; it
never clears all mappings. The separately installed `IMC_UI` and its menu bindings remain in the
controller until the UI input migration.

After building the plugin, run `Scripts/MigrateFoundryInput.py` with Unreal's Python commandlet to
apply this migration in another checkout. It backs up affected assets under `Saved/GASPInputMigration`,
creates the configured handler Blueprints and setup, and removes only the three legacy input chains.
Pass `-GASPInputVerifyOnly` to reopen the saved assets, compile the Blueprint families and confirm
that the remaining bindings and original input mapping/action files are intact.

## From input to completion

1. Foundry's Scan Ability discovers target slots and asks their handlers whether they are available.
   These focus checks are read-only: they do not choose an animation or reserve either Pawn.
2. Input calls `RequestAction` with `Shove` or `Takedown`. A remote player sends only that action name;
   authority resolves the target through Foundry's current interaction offers.
3. Foundry accepts the request, claims the target slot and runs optional preparation. The GASP handler
   asks the Action Component to select a Chooser/PoseSearch performance for that exact partner and
   reserve both participants. No montage starts during preparation.
4. Foundry runs Commit after preparation succeeds. The handler consumes the prepared action and waits
   asynchronously while each participant plays its selected montage through Mover.
5. The initiator replicates the selected performance and start time to observers. Each machine rebuilds
   its own animation contexts. Animation Blueprints and motion-warping notifies read the participant's
   local result through `GetInteractionResult`; animation instances are never sent over the network.
6. Once every role succeeds, the handler completes Foundry's interaction. Cancellation, participant
   removal or an unexpected montage interruption stops the action and releases its reservations.

Preparation and Commit have independent Foundry timeouts. A prepared action that never reaches Commit
is also released when its Foundry execution ends. Repeating an identical performance creates a fresh
execution identity, so an old callback cannot cancel the next attempt.

## Planned animation handoffs

An animation can intentionally hand its role to another gameplay behavior before its montage ends.
Use **GASP Action Handoff** as the native base of that completion notify. It calls
`CompleteRoleForHandoff` before the notify's Blueprint event runs. The existing
`BP_AnimNotify_TriggerRagdoll` uses this base, retaining its ragdoll behavior. The other participant
must still finish before Foundry reports success. An unrelated interruption remains a failure.

## Migrating the sample assets

After building the plugin, run `Scripts/MigrateFoundryInteractions.py` with Unreal's Python commandlet.
The script backs up touched assets under the project's `Saved/GASPFoundry/AssetBackup`, records their
original defaults, reparents the sample and preserves its input event signatures. It replaces the
legacy Blueprint candidate scan with Foundry requests, restores named native component references,
creates the ability setup and compiles the affected Blueprints. Controller references remain compatible.

Pass `-GASPVerifyOnly` to reopen and check the saved assets without migrating them again. Review the
script's output and keep the backups until the game has been validated. Project configuration still
needs the ability setup registration described above.

## Verification

`NinjaBearStudio.GASP.Actions` covers reservation, cancellation, stale requests, participant removal,
Mover completion, interruption and authored handoffs through the public APIs. Playback-focused fixtures
control animation selection while retaining the production playback and ownership path. The network
scenarios use the migrated sample Blueprint, real Chooser/PoseSearch selection and remote input, then
observe owning players and other clients through completion, repeat and cancellation.

The [Foundry interaction guide](../../../../NinjaBearStudio/Foundry/Source/Interaction/FoundryInteraction/README.md)
explains target definitions, preparation and interaction flows. Existing equipment overlay integration
is described in the [plugin README](../../README.md#crucible-pawn-and-equipment-overlays).
