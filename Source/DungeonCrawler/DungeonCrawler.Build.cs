using System.IO;
using UnrealBuildTool;

public class DungeonCrawler : ModuleRules
{
	public DungeonCrawler(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Source is organized into subfolders for navigation; add each as an include root so existing
		// bare includes (e.g. #include "HealthComponent.h") keep resolving without per-file path changes.
		foreach (string Dir in new string[]
		{
			"Core", "Player", "Components", "Enemies", "Dungeon", "Items", "Town", "UI", "VFX"
		})
		{
			PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, Dir));
		}

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
			"GameplayTags",
			"ImageWrapper",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
