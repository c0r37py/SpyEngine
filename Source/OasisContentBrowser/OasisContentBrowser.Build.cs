// Innerloop Interactive Production :: c0r37py

using System.IO;
using UnrealBuildTool;

public class OasisContentBrowser : ModuleRules
{
	public OasisContentBrowser(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.AddRange(
             new string[] {
                    Path.Combine(ModuleDirectory, "Private"),
                    Path.Combine(ModuleDirectory, "Private/Adapters"),
                    Path.Combine(ModuleDirectory, "Private/DependencyViewer"),
                    Path.Combine(ModuleDirectory, "Private/Widgets"),
                    Path.Combine(ModuleDirectory, "Private/Documentation"),
             }
            );


        PublicIncludePaths.AddRange(
            new string[]
            {
            }
            );

        PrivateIncludePathModuleNames.AddRange(
			new string[] {
                "AssetRegistry",
                "AssetTools",
                "AssetManagerEditor",
                "CollectionManager",
                "GameProjectGeneration",
                "MainFrame",
                "PackagesDialog",
                "SourceControl",
                "SourceControlWindows"
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "AppFramework",
                "ApplicationCore",
                "AssetRegistry",
                "AssetTagsEditor",
                "BlueprintGraph",
                "CoreUObject",
                "CollectionManager",
                "ContentBrowserData",
                "DesktopPlatform",
                "DesktopWidgets",
                "Engine",
                "EditorStyle",
                "EditorWidgets",
                "FileUtilities",
                "GraphEditor",
                "Projects",
                "InputCore",
                "LevelEditor",                
                "RenderCore",
                "RHI",
                "Slate",
                "SlateCore",
                "TreeMap",
                "ToolMenus",
                "UnrealEd",
                "WorkspaceMenuStructure",

                // For Documentation
                "Analytics",
                "Documentation",
                "SourceCodeAccess",
                "SourceControl",
            }
            );

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "AssetTools",
                "Core",
			}
			);

		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
                //"AssetTools",
                "PropertyEditor",
                "PackagesDialog",
                "GameProjectGeneration",
                "MainFrame"
            }
			);
    }
}
