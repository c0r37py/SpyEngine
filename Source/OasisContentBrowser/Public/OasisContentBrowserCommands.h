// Innerloop Interactive Production :: c0r37py

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "OasisContentBrowserStyle.h"

class FOasisContentBrowserCommands : public TCommands<FOasisContentBrowserCommands>
{
public:

	FOasisContentBrowserCommands()
		: TCommands<FOasisContentBrowserCommands>(TEXT("SpyEngine"), NSLOCTEXT("Contexts", "SpyEngine", "SpyEngine"), NAME_None, FOasisContentBrowserStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenSpyEngine;
	TSharedPtr< FUICommandInfo > ImportSelectedUAsset;
	TSharedPtr< FUICommandInfo > ToggleDependencyViewer;
};
