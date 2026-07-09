using UnrealBuildTool;

public class GASP : ModuleRules
{
	public GASP(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		CppCompileWarningSettings.NonInlinedGenCppWarningLevel = WarningLevel.Warning;

		PublicDependencyModuleNames.AddRange(new []
		{
			"AnimationWarpingRuntime",
			"Core",
			"CoreUObject",
			"Engine",
			"NetCore",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new []
		{
			"MotionTrajectory",
			"PoseSearch",
			"Chooser",
			"AnimationWarpingRuntime",
			"BlendStack",
			"PhysicsCore",
			"AnimGraphRuntime",
			"Niagara",
			"MotionWarping",
		});

		if (Target.Type == TargetRules.TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new []
			{
				"GameplayDebugger",
			});
		}
	}
}