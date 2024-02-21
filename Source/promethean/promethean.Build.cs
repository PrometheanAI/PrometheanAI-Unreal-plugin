// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class promethean : ModuleRules
{
	public promethean(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "Json", "JsonUtilities"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "MaterialEditor",
                "RHI", // holds definitions of shit like this: GMaxRHIFeatureLevel
                "UnrealEd",  // holds ThumbnailTools
                "InputCore",  // tcp stuff
                "Sockets", // tcp stuff
                "Networking", // tcp stuff
                "ProceduralMeshComponent", // Mesh generation
                "MeshDescription", // Generated meshes -> static meshes
				"ViewportInteraction",
				"VREditor",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
