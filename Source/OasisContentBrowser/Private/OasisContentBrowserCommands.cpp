// Innerloop Interactive Production :: c0r37py

#include "OasisContentBrowserCommands.h"

#define LOCTEXT_NAMESPACE "FOasisContentBrowserModule"

void FOasisContentBrowserCommands::RegisterCommands()
{
	UI_COMMAND(OpenSpyEngine, "UAsset", "Open SpyEngine", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ImportSelectedUAsset, "Import Selected UAsset", "Import Selected UAsset File", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ToggleDependencyViewer, "Toggle Dependency Viewer", "Toggle the display of Dependency Viewer", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
