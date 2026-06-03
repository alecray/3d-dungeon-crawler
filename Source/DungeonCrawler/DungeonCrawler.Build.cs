using UnrealBuildTool;

public class DungeonCrawler : ModuleRules
{
	public DungeonCrawler(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
