// Innerloop Interactive Production :: c0r37py

#include "OasisDependencyViewerSchema.h"
#include "OasisContentBrowser.h"
#include "OasisContentBrowserStyle.h"
#include "OasisContentBrowserSettings.h"
#include "OasisDependencyViewerCommands.h"

#include "Textures/SlateIcon.h"
#include "Misc/Attribute.h"
#include "ToolMenus.h"
#include "EdGraph/EdGraph.h"
#include "EditorStyleSet.h"
#include "CollectionManagerTypes.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
#include "ConnectionDrawingPolicy.h"


static const FLinearColor RiceFlower = FLinearColor(FColor(236, 252, 227));
static const FLinearColor CannonPink = FLinearColor(FColor(145, 66, 117));
static const FLinearColor CannonPinkRed = FLinearColor(FColor(245, 66, 11));

// Overridden connection drawing policy to use less curvy lines between nodes
class FExtDependencyViewerConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
public:
	FExtDependencyViewerConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements)
		: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements)
	{
	}

	virtual FVector2D ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const override
	{
		const UOasisContentBrowserSettings* OasisContentBrowserSetting = GetDefault<UOasisContentBrowserSettings>();
		const bool bSpline = !OasisContentBrowserSetting->UseStraightLineInDependencyViewer;

		if (bSpline)
		{
			const int32 Tension = FMath::Abs<int32>(Start.X - End.X);
			return Tension * FVector2D(1.0f, 0);
		}
		else
		{
			const FVector2D Delta = End - Start;
			const FVector2D NormDelta = Delta.GetSafeNormal();
			return NormDelta;
		}
	}

	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params) override
	{
		const bool bHardRefernce = OutputPin->PinType.PinCategory == TEXT("hard") || InputPin->PinType.PinCategory == TEXT("hard");
		const bool bInvalid = OutputPin->PinType.PinSubCategory == TEXT("invalid") || InputPin->PinType.PinSubCategory == TEXT("invalid");

		if (bHardRefernce)
		{
			if (bInvalid)
			{
				Params.WireColor = FOasisContentBrowserStyle::Get().GetColor("ErrorReporting.HardReferenceColor");
			}
			else
			{
				Params.WireColor = RiceFlower;
			}
		}
		else
		{
			if (bInvalid)
			{
				Params.WireColor = FOasisContentBrowserStyle::Get().GetColor("ErrorReporting.SoftReferenceColor");
			}
			else
			{
				Params.WireColor = CannonPink;
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// UOasisDependencyViewerSchema

UOasisDependencyViewerSchema::UOasisDependencyViewerSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UOasisDependencyViewerSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	{
		FToolMenuSection& Section = Menu->AddSection(TEXT("Fit"), NSLOCTEXT("ReferenceViewerSchema", "FitSectionLabel", "Fit"));
		Section.AddMenuEntry(FOasisDependencyViewerCommands::Get().ZoomToFitSelected);
	}

#if ECB_LEGACY
	{
		FToolMenuSection& Section = Menu->AddSection(TEXT("Asset"), NSLOCTEXT("ReferenceViewerSchema", "AssetSectionLabel", "Asset"));
		Section.AddMenuEntry(FGlobalEditorCommonCommands::Get().FindInContentBrowser);
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().OpenSelectedInAssetEditor);
	}

	{
		FToolMenuSection& Section = Menu->AddSection(TEXT("Misc"), NSLOCTEXT("ReferenceViewerSchema", "MiscSectionLabel", "Misc"));
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().ZoomToFit);
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().ReCenterGraph);
		Section.AddSubMenu(
			"MakeCollectionWith",
			NSLOCTEXT("ReferenceViewerSchema", "MakeCollectionWithTitle", "Make Collection with"),
			NSLOCTEXT("ReferenceViewerSchema", "MakeCollectionWithTooltip", "Makes a collection with either the referencers or dependencies of the selected nodes."),
			FNewToolMenuDelegate::CreateUObject(const_cast<UReferenceViewerSchema*>(this), &UReferenceViewerSchema::GetMakeCollectionWithSubMenu)
		);
	}

	{
		FToolMenuSection& Section = Menu->AddSection(TEXT("References"), NSLOCTEXT("ReferenceViewerSchema", "ReferencesSectionLabel", "References"));
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().CopyReferencedObjects);
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().CopyReferencingObjects);
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().ShowReferencedObjects);
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().ShowReferencingObjects);
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().ShowReferenceTree);
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().ViewSizeMap);

		FToolMenuEntry ViewAssetAuditEntry = FToolMenuEntry::InitMenuEntry(FAssetManagerEditorCommands::Get().ViewAssetAudit);
		ViewAssetAuditEntry.Name = TEXT("ContextMenu");
		Section.AddEntry(ViewAssetAuditEntry);
	}
#endif
}

FLinearColor UOasisDependencyViewerSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	if (PinType.PinCategory == TEXT("hard"))
	{
		return RiceFlower;
	}
	else
	{
		return CannonPink;
	}
}

void UOasisDependencyViewerSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	// Don't allow breaking any links
}

void UOasisDependencyViewerSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	// Don't allow breaking any links
}

FPinConnectionResponse UOasisDependencyViewerSchema::MovePinLinks(UEdGraphPin& MoveFromPin, UEdGraphPin& MoveToPin, bool bIsIntermediateMove, bool bNotifyLinkedNodes) const
{
	// Don't allow moving any links
	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, FString());
}

FPinConnectionResponse UOasisDependencyViewerSchema::CopyPinLinks(UEdGraphPin& CopyFromPin, UEdGraphPin& CopyToPin, bool bIsIntermediateCopy) const
{
	// Don't allow copying any links
	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, FString());
}

FConnectionDrawingPolicy* UOasisDependencyViewerSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	return new FExtDependencyViewerConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements);
}

void UOasisDependencyViewerSchema::GetAssetsGraphHoverMessage(const TArray<FAssetData>& Assets, const UEdGraph* HoverGraph, FString& OutTooltipText, bool& OutOkIcon) const
{
	OutOkIcon = true;
}

void UOasisDependencyViewerSchema::GetMakeCollectionWithSubMenu(UToolMenu* Menu)
{
#if ECB_LEGACY
	FToolMenuSection& Section = Menu->AddSection("Section");

	Section.AddSubMenu(
		"MakeCollectionWithReferencers",
		NSLOCTEXT("ReferenceViewerSchema", "MakeCollectionWithReferencersTitle", "Referencers <-"),
		NSLOCTEXT("ReferenceViewerSchema", "MakeCollectionWithReferencersTooltip", "Makes a collection with assets one connection to the left of selected nodes."),
		FNewToolMenuDelegate::CreateUObject(this, &UReferenceViewerSchema::GetMakeCollectionWithReferencersOrDependenciesSubMenu, true)
		);

	Section.AddSubMenu(
		"MakeCollectionWithDependencies",
		NSLOCTEXT("ReferenceViewerSchema", "MakeCollectionWithDependenciesTitle", "Dependencies ->"),
		NSLOCTEXT("ReferenceViewerSchema", "MakeCollectionWithDependenciesTooltip", "Makes a collection with assets one connection to the right of selected nodes."),
		FNewToolMenuDelegate::CreateUObject(this, &UReferenceViewerSchema::GetMakeCollectionWithReferencersOrDependenciesSubMenu, false)
		);
#endif
}

void UOasisDependencyViewerSchema::GetMakeCollectionWithReferencersOrDependenciesSubMenu(UToolMenu* Menu, bool bReferencers)
{
#if ECB_LEGACY
	FToolMenuSection& Section = Menu->AddSection("Section");

	if (bReferencers)
	{
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().MakeLocalCollectionWithReferencers, 
			TAttribute<FText>(),
			ECollectionShareType::GetDescription(ECollectionShareType::CST_Local), 
			FSlateIcon(FAppStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ECollectionShareType::CST_Local))
			);
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().MakePrivateCollectionWithReferencers,
			TAttribute<FText>(),
			ECollectionShareType::GetDescription(ECollectionShareType::CST_Private), 
			FSlateIcon(FAppStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ECollectionShareType::CST_Private))
			);
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().MakeSharedCollectionWithReferencers,
			TAttribute<FText>(),
			ECollectionShareType::GetDescription(ECollectionShareType::CST_Shared), 
			FSlateIcon(FAppStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ECollectionShareType::CST_Shared))
			);
	}
	else
	{
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().MakeLocalCollectionWithDependencies, 
			TAttribute<FText>(),
			ECollectionShareType::GetDescription(ECollectionShareType::CST_Local), 
			FSlateIcon(FAppStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ECollectionShareType::CST_Local))
			);
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().MakePrivateCollectionWithDependencies,
			TAttribute<FText>(),
			ECollectionShareType::GetDescription(ECollectionShareType::CST_Private), 
			FSlateIcon(FAppStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ECollectionShareType::CST_Private))
			);
		Section.AddMenuEntry(FAssetManagerEditorCommands::Get().MakeSharedCollectionWithDependencies,
			TAttribute<FText>(),
			ECollectionShareType::GetDescription(ECollectionShareType::CST_Shared), 
			FSlateIcon(FAppStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ECollectionShareType::CST_Shared))
			);
	}
#endif
}
