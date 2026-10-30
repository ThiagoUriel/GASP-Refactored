// Ninja Bear Studio Inc., all rights reserved.
// ReSharper disable InconsistentNaming
using UnrealBuildTool;

public class GASPNBS : ModuleRules
{
	public GASPNBS(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"CrucibleGameplay",
			"FoundryActorFramework",
			"FoundryInput",
			"FoundryInputHandlers",
			"FoundryPersistence",
			"Engine",
			"FoundryInteraction",
			"FoundryInventory",
			"FoundryInventoryEquipment",
			"FoundryInventoryPresence",
			"GASP",
			"GASPExtras",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"FoundryGameplayAbilities",
			"EnhancedInput",
			"GameplayAbilities"
		});
	}
}
