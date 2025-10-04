// Innerloop Interactive Production :: c0r37py

#pragma once

#include "CoreMinimal.h"
#include "EditorStyleSet.h"
#include "Framework/Commands/Commands.h"

// Actions that can be invoked in the dependency viewer
class FOasisDependencyViewerCommands : public TCommands<FOasisDependencyViewerCommands>
{
public:
	FOasisDependencyViewerCommands();

	// TCommands<> interface
	virtual void RegisterCommands() override;
	// End of TCommands<> interface

	/** Zoom in to fit the selected objects in the window */
	TSharedPtr<FUICommandInfo> ZoomToFitSelected;
};
