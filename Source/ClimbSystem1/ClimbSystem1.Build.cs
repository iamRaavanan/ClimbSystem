// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ClimbSystem1 : ModuleRules
{
	public ClimbSystem1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
