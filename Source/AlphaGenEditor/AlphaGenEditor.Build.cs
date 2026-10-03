// Copyright K-Studio. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class AlphaGenEditor : ModuleRules
{
	public AlphaGenEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
		);
				
		PrivateIncludePaths.AddRange(
			new string[] {
				System.IO.Path.Combine(ModuleDirectory),
			}
		);
			
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"RenderCore",
				"RHI",
				"Renderer",
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"EditorStyle",
				"UnrealEd",
				"ToolMenus",
				"Projects",
				"EditorFramework",
				"WorkspaceMenuStructure",
				"PropertyEditor",
				"DesktopPlatform",
				"ImageWrapper",
			}
		);
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
		
		// Register shader directory for GPU compute shaders
		string PluginPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));
		string ShaderPath = Path.Combine(PluginPath, "Shaders");
		if (Directory.Exists(ShaderPath))
		{
			// Shader directory will be registered by module startup
		}
	}
}
