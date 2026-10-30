using UnrealBuildTool;

public class GASPExtras : ModuleRules
{
	public GASPExtras(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GASP",
				"CrucibleGameplay",
				"FoundryActorFramework",
				"FoundryPersistence",
				"InputCore",
				"EnhancedInput",
				"Mover",
				"GameplayTags",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"GameplayCameras"
			}
		);
	}
}