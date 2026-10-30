// Ninja Bear Studio Inc., all rights reserved.
using UnrealBuildTool;

public class GASPTests : ModuleRules
{
	public GASPTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Chooser",
			"Core",
			"CoreUObject",
			"CrucibleGameplay",
			"Engine",
			"FoundryActorFramework",
			"FoundryGameplayAbilities",
			"FoundryInventory",
			"FoundryInventoryEquipment",
			"FoundryInventoryPresence",
			"FoundryPersistence",
			"FoundryTests",
			"GASP",
			"GASPNBS",
			"GASPExtras",
			"FoundryInteraction",
			"PoseSearch",
			"GameplayAbilities",
			"GameplayCameras",
			"GameplayTags",
			"GameplayTasks",
			"Mover",
			"PhysicsControl",
			"UnrealEd"
		});
	}
}
