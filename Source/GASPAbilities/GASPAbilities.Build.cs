// Ninja Bear Studio Inc., all rights reserved.
using UnrealBuildTool;

public class GASPAbilities : ModuleRules
{
    public GASPAbilities(ReadOnlyTargetRules target) : base(target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "Core",
                "GameplayAbilities",
                "GameplayTags",
                "GameplayTasks",
                "GASP"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "CoreUObject",
                "Engine",
                "ModularGameplay"
            }
        );
    }
}