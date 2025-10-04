// Innerloop Interactive Production :: c0r37py

#include "OasisDependencyViewerCommands.h"

//////////////////////////////////////////////////////////////////////////
// FOasisDependencyViewerCommands

#define LOCTEXT_NAMESPACE "OasisDependencyViewerCommands"

FOasisDependencyViewerCommands::FOasisDependencyViewerCommands() : TCommands<FOasisDependencyViewerCommands>(
	"OasisDependencyViewerCommands",
	NSLOCTEXT("Contexts", "OasisDependencyViewerCommands", "Dependency Viewer Commands"),
	NAME_None, 
	FAppStyle::GetAppStyleSetName())
{
}

void FOasisDependencyViewerCommands::RegisterCommands()
{
	UI_COMMAND(ZoomToFitSelected, "Zoom to Fit", "Zoom in and center the view on the selected item", EUserInterfaceActionType::Button, FInputChord(EKeys::Home));
}

#undef LOCTEXT_NAMESPACE
