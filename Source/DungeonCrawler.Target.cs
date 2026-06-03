using UnrealBuildTool;
using System.Collections.Generic;

public class DungeonCrawlerTarget : TargetRules
{
	public DungeonCrawlerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DungeonCrawler");
	}
}
