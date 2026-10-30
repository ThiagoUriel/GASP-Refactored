# Ninja Bear Studio Inc., all rights reserved.
"""Migrate the sample's interaction graphs to Foundry, with backups before any asset is saved."""

import json
import shutil
from pathlib import Path

import unreal

SAMPLE = '/GASP/GASPExtras/Blueprints/CBP_SandboxCharacter'
PLAYER = '/Game/Blueprints/Characters/CPB_Player'
CONTROLLERS = ('/GASP/GASPExtras/Blueprints/BP_GASP_PlayerController', '/Game/Blueprints/Player/PC_Player')
PROFILE = '/Game/Data/Abilities/Setups/AS_GASPInteraction'
HANDOFF_NOTIFY = '/GASP/GASP/Blueprints/AnimNotifies/BP_AnimNotify_TriggerRagdoll'
NATIVE_PAWN = '/Script/GASPNBS.GASPFoundryPawn'
PROJECT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
REPORT = PROJECT / 'Saved/GASPFoundry'
BACKUP = REPORT / 'AssetBackup'


def asset_file(package):
    """Resolve only the game and GASP mount points touched by this migration."""
    mount, relative = package[1:].split('/', 1)
    base = PROJECT / 'Content' if mount == 'Game' else PROJECT / 'Plugins/Game/GASP/Content'
    assert mount in ('Game', 'GASP'), package
    return base / (relative + '.uasset')


def backup_asset(package):
    """Retain the original file once so rerunning validation cannot replace the backup."""
    source = asset_file(package)
    destination = BACKUP / package[1:]
    destination = destination.with_suffix('.uasset')
    if source.exists() and not destination.exists():
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)


def snapshot(blueprint):
    """Capture the sample's mesh, animation, settings and camera defaults before changing its parent."""
    default = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(blueprint))
    mesh = default.get_editor_property('mesh')
    asset = mesh.get_skinned_asset()
    animation = mesh.get_editor_property('anim_class')
    settings = default.get_editor_property('settings')
    return {
        'mesh': asset.get_path_name() if asset else None,
        'animation': animation.get_path_name() if animation else None,
        'settings': settings.get_path_name() if settings else None,
        'mesh_transform': mesh.get_relative_transform().export_text(),
        'camera_name': default.get_editor_property('gameplay_camera').get_name(),
    }


def connect(output, input_pin):
    """Fail before saving if a graph connection does not match the expected signature."""
    assert output.is_valid() and input_pin.is_valid()
    assert output.try_create_connection(input_pin)


def rewrite_input(blueprint):
    """Keep the public sample event while replacing its candidate-array execution path."""
    editor = unreal.BlueprintGraphEditor.get_graph_editor_by_name(blueprint, 'EventGraph')
    nodes = {node.get_name(): node for node in editor.list_all_nodes()}
    event = next(node for node in nodes.values() if node.get_node_title() == 'TryInteract' and node.get_class().get_name() == 'K2Node_CustomEvent')
    event.find_then_pin().break_pin_links()
    event.find_output_pin('InteractionType').break_pin_links()
    legacy_nodes = (
        'K2Node_VariableGet_7', 'K2Node_PromotableOperator_3', 'K2Node_BreakStruct_2',
        'K2Node_InstancedStruct_1', 'K2Node_VariableGet_8', 'K2Node_IfThenElse_2',
        'K2Node_VariableGet_6', 'K2Node_CallFunction_13',
    )
    assert all(name in nodes for name in legacy_nodes), 'The sample input graph has changed; review its new layout before migrating.'
    editor.remove_nodes([nodes[name] for name in legacy_nodes])
    request = editor.add_call_function_node(NATIVE_PAWN + '.RequestAction')
    connect(event.find_then_pin(), request.find_execute_pin())
    connect(event.find_output_pin('InteractionType'), request.find_input_pin('ActionType'))
    position = event.get_node_pos()
    request.set_node_pos(unreal.IntPoint(position.x + 320, position.y))

    # The old scan ran each tick ahead of unrelated ragdoll recovery. Preserve that continuation.
    scan = nodes['K2Node_CallFunction_4']
    incoming = scan.find_execute_pin().list_connected_pins()
    outgoing = scan.find_then_pin().list_connected_pins()
    editor.remove_nodes([scan])
    for previous in incoming:
        for following in outgoing:
            connect(previous, following)
    for name in ('CheckForInteractableCharacters', 'CanDoCharacterInteraction', 'CanBeInteractedWith'):
        unreal.BlueprintEditorLibrary.remove_function_graph(blueprint, name)
    editor.remove_member_variable('InteractableActors')
    editor.remove_member_variable('Actor')


def rewrite_result(blueprint):
    """Keep the animation interface while reading the component's local role result."""
    editor = unreal.BlueprintGraphEditor.get_graph_editor_by_name(blueprint, 'GetInteractionResult')
    nodes = editor.list_all_nodes()
    entry = next(node for node in nodes if node.get_class().get_name() == 'K2Node_FunctionEntry')
    result = next(node for node in nodes if node.get_class().get_name() == 'K2Node_FunctionResult')
    entry.find_then_pin().break_pin_links()
    result.find_input_pin('ReturnValue').break_pin_links()
    result.find_execute_pin().break_pin_links()
    editor.remove_nodes([node for node in nodes if node != entry and node != result])
    component = editor.add_call_function_node('/Script/GASPExtras.GASPCharacterExample.GetActionComponent')
    role = editor.add_call_function_node('/Script/GASP.GASPActionComponent.GetActionResult')
    connect(component.find_result_pin(), role.find_self_pin())
    connect(role.find_result_pin(), result.find_input_pin('ReturnValue'))
    connect(entry.find_then_pin(), result.find_execute_pin())
    component.set_node_pos(unreal.IntPoint(160, 160))
    role.set_node_pos(unreal.IntPoint(480, 160))
    result.set_node_pos(unreal.IntPoint(800, 0))


def restore_components(blueprint):
    """Reconnect native component properties after the old interaction field changes ownership."""
    for property_name, component_name in (('ActionComponent', 'CharacterInteractionComponent'), ('InteractionComponent', 'InteractionComponent')):
        assert unreal.GASPBlueprintMigrationLibrary.restore_default_component_reference(blueprint, property_name, component_name)


def create_profile():
    """Grant the sample's Scan and Execute abilities through Foundry's normal profile asset."""
    asset = unreal.load_asset(PROFILE)
    if not asset:
        unreal.EditorAssetLibrary.make_directory('/Game/Data/Abilities/Setups')
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', unreal.FoundryAbilitySetupDataAsset)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset('AS_GASPInteraction', '/Game/Data/Abilities/Setups', unreal.FoundryAbilitySetupDataAsset, factory)
    assert asset
    default = unreal.get_default_object(unreal.load_class(None, NATIVE_PAWN))
    asset.set_editor_property('profile_tag', default.get_editor_property('ability_system_profile_tag'))
    abilities = []
    for path in ('/Script/FoundryInteraction.InteractionAbility_Scan', '/Script/GASPNBS.GASPInteractionAbility_Execute'):
        entry = unreal.FoundryDefaultGameplayAbility()
        entry.import_text('(GameplayAbilityClass=\"' + path + '\",Level=1,Input=-1)')
        abilities.append(entry)
    asset.set_editor_property('default_gameplay_abilities', abilities)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)


def verify(expected=None):
    """Compile the affected Blueprint family and confirm the inherited components and defaults."""
    result = {'blueprints': {}, 'status': 'passed'}
    for package in (SAMPLE, PLAYER) + CONTROLLERS:
        blueprint = unreal.load_asset(package)
        assert blueprint, package
        assert unreal.BlueprintEditorLibrary.compile_blueprint(blueprint), 'Blueprint compilation failed: ' + package
        actual = snapshot(blueprint) if package in (SAMPLE, PLAYER) else None
        if expected and actual:
            assert actual == expected[package], (package, expected[package], actual)
        if actual:
            default = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(blueprint))
            assert default.get_editor_property('interaction_component').get_name() == 'InteractionComponent'
            assert default.get_editor_property('action_component').get_name() == 'CharacterInteractionComponent'
            shapes = {component.get_name(): component for component in default.get_components_by_class(unreal.SphereComponent)}
            assert str(shapes['InteractionScan'].get_collision_profile_name()) == 'InteractionScanner'
            assert str(shapes['InteractionTargetCollision'].get_collision_profile_name()) == 'InteractionTarget'
            target = default.get_editor_property('interaction_target_component')
            slots = list(target.get_editor_property('slots'))
            assert len(slots) == 2
            for slot in slots:
                definition = slot.get_editor_property('definitions')[0]
                assert definition.get_editor_property('interaction_preparation_handler')
                assert definition.get_editor_property('interaction_committed_handler')
        result['blueprints'][package] = {'parent': unreal.BlueprintEditorLibrary.get_blueprint_parent_class(blueprint).get_path_name(), 'defaults': actual}
    sample = unreal.load_asset(SAMPLE)
    assert unreal.BlueprintEditorLibrary.get_blueprint_parent_class(sample).get_path_name() == NATIVE_PAWN
    names = [graph.get_name() for graph in unreal.BlueprintEditorLibrary.list_graphs(sample)]
    assert 'CheckForInteractableCharacters' not in names
    assert 'CanDoCharacterInteraction' not in names
    assert unreal.load_asset(PROFILE)
    notify = unreal.load_asset(HANDOFF_NOTIFY)
    assert unreal.BlueprintEditorLibrary.compile_blueprint(notify)
    assert unreal.BlueprintEditorLibrary.get_blueprint_parent_class(notify).get_path_name() == '/Script/GASP.AnimNotify_GASPActionHandoff'
    (REPORT / 'assets_verified.json').write_text(json.dumps(result, indent=2))
    return result


def migrate():
    """Apply the reviewed migration, preserving authored defaults and existing controller callers."""
    REPORT.mkdir(parents=True, exist_ok=True)
    for package in (SAMPLE, PLAYER) + CONTROLLERS + (PROFILE, HANDOFF_NOTIFY):
        backup_asset(package)
    sample = unreal.load_asset(SAMPLE)
    player = unreal.load_asset(PLAYER)
    baseline = REPORT / 'defaults_before.json'
    expected = json.loads(baseline.read_text()) if baseline.exists() else {SAMPLE: snapshot(sample), PLAYER: snapshot(player)}
    if not baseline.exists():
        baseline.write_text(json.dumps(expected, indent=2))
    graph_names = [graph.get_name() for graph in unreal.BlueprintEditorLibrary.list_graphs(sample)]
    if 'CheckForInteractableCharacters' in graph_names:
        rewrite_input(sample)
        rewrite_result(sample)
    native = unreal.load_class(None, NATIVE_PAWN)
    assert native
    if unreal.BlueprintEditorLibrary.get_blueprint_parent_class(sample) != native:
        unreal.BlueprintEditorLibrary.reparent_blueprint(sample, native)
    assert unreal.BlueprintEditorLibrary.compile_blueprint(sample)
    for blueprint in (sample, player):
        restore_components(blueprint)
        default = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(blueprint))
        profile = default.get_editor_property('ability_system_profile_tag')
        if not profile.export_text() or 'TagName=""' in profile.export_text() or 'None' in profile.export_text():
            default.set_editor_property('ability_system_profile_tag', unreal.get_default_object(native).get_editor_property('ability_system_profile_tag'))
        assert unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        restore_components(blueprint)
        assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    notify = unreal.load_asset(HANDOFF_NOTIFY)
    parent = unreal.load_class(None, '/Script/GASP.AnimNotify_GASPActionHandoff')
    if unreal.BlueprintEditorLibrary.get_blueprint_parent_class(notify) != parent:
        unreal.BlueprintEditorLibrary.reparent_blueprint(notify, parent)
    assert unreal.BlueprintEditorLibrary.compile_blueprint(notify)
    assert unreal.EditorAssetLibrary.save_loaded_asset(notify, only_if_is_dirty=False)
    create_profile()
    verify(expected)
    unreal.log('GASP_FOUNDRY_MIGRATION_PASSED')


if '-GASPVerifyOnly' in unreal.SystemLibrary.get_command_line():
    expected = json.loads((REPORT / 'defaults_before.json').read_text())
    verify(expected)
    unreal.log('GASP_FOUNDRY_REOPEN_VERIFIED')
else:
    migrate()
