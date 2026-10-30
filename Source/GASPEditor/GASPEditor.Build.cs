using UnrealBuildTool;

public class GASPEditor : ModuleRules
{
	public GASPEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		CppCompileWarningSettings.NonInlinedGenCppWarningLevel = WarningLevel.Warning;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine", "AnimGraphRuntime", "AnimationModifiers", "AnimationBlueprintLibrary",
			"GASP", "GameplayTags"
		});

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(new[]
			{
				"AnimGraph"
			});

			PrivateDependencyModuleNames.AddRange(new[]
			{
				"BlueprintGraph",
				"UnrealEd"
			});
		}
	}
}