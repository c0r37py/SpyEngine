// Innerloop Interactive Production :: c0r37py

#include "EdGraphNode_OasisDependency.h"
#include "OasisAssetData.h"
#include "OasisPackageUtils.h"
#include "OasisContentBrowser.h"

#include "GenericPlatform/GenericPlatformFile.h"
#include "EdGraph/EdGraphPin.h"
#include "HAL/PlatformFilemanager.h"

#define LOCTEXT_NAMESPACE "ExtDependencyViewer"

//////////////////////////////////////////////////////////////////////////
// UEdGraphNode_OasisDependency

UEdGraphNode_OasisDependency::UEdGraphNode_OasisDependency(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DependencyPin = NULL;
	ReferencerPin = NULL;
	bIsCollapsed = false;
	bIsPackage = false;
	bIsPrimaryAsset = false;
	bUsesThumbnail = false;
	bIsMissingOrInvalid = false;
}

void UEdGraphNode_OasisDependency::SetupReferenceNode(const FIntPoint& NodeLoc, const TArray<FExtAssetIdentifier>& NewIdentifiers, const FOasisAssetData& InAssetData)
{
	check(NewIdentifiers.Num() > 0);

	NodePosX = NodeLoc.X;
	NodePosY = NodeLoc.Y;

	Identifiers = NewIdentifiers;
	const FExtAssetIdentifier& First = NewIdentifiers[0];
	FString ShortPackageName = FPackageName::GetLongPackageAssetName(First.PackageName.ToString());

	bIsCollapsed = false;
	bIsPackage = true;
	
	FPrimaryAssetId PrimaryAssetID = NewIdentifiers[0].GetPrimaryAssetId();
	if (PrimaryAssetID.IsValid())
	{
		ShortPackageName = PrimaryAssetID.ToString();
		bIsPackage = false;
		bIsPrimaryAsset = true;
	}
	else if (NewIdentifiers[0].IsValue())
	{
		ShortPackageName = FString::Printf(TEXT("%s::%s"), *First.ObjectName.ToString(), *First.ValueName.ToString());
		bIsPackage = false;
	}

	if (NewIdentifiers.Num() == 1 )
	{
		if (bIsPackage)
		{
			NodeComment = First.PackageName.ToString();
		}
		NodeTitle = FText::FromString(ShortPackageName);
	}
	else
	{
		NodeComment = FText::Format(LOCTEXT("ReferenceNodeMultiplePackagesTitle", "{0} nodes"), FText::AsNumber(NewIdentifiers.Num())).ToString();
		NodeTitle = FText::Format(LOCTEXT("ReferenceNodeMultiplePackagesComment", "{0} and {1} others"), FText::FromString(ShortPackageName), FText::AsNumber(NewIdentifiers.Num() - 1));
	}
	
	CacheAssetData(InAssetData);
	AllocateDefaultPins();
}

void UEdGraphNode_OasisDependency::SetReferenceNodeCollapsed(const FIntPoint& NodeLoc, int32 InNumReferencesExceedingMax)
{
	NodePosX = NodeLoc.X;
	NodePosY = NodeLoc.Y;

	Identifiers.Empty();
	bIsCollapsed = true;
	bUsesThumbnail = false;
	NodeComment = FText::Format(LOCTEXT("ReferenceNodeCollapsedMessage", "{0} other nodes"), FText::AsNumber(InNumReferencesExceedingMax)).ToString();

	NodeTitle = LOCTEXT("ReferenceNodeCollapsedTitle", "Collapsed nodes");
	CacheAssetData(FOasisAssetData());
	AllocateDefaultPins();
}

void UEdGraphNode_OasisDependency::AddReferencer(UEdGraphNode_OasisDependency* ReferencerNode)
{
	UEdGraphPin* ReferencerDependencyPin = ReferencerNode->GetDependencyPin();

	if ( ensure(ReferencerDependencyPin) )
	{
		ReferencerDependencyPin->bHidden = false;
		ReferencerPin->bHidden = false;
		ReferencerPin->MakeLinkTo(ReferencerDependencyPin);
	}
}

FExtAssetIdentifier UEdGraphNode_OasisDependency::GetIdentifier() const
{
	if (Identifiers.Num() > 0)
	{
		return Identifiers[0];
	}

	return FExtAssetIdentifier();
}

void UEdGraphNode_OasisDependency::GetAllIdentifiers(TArray<FExtAssetIdentifier>& OutIdentifiers) const
{
	OutIdentifiers.Append(Identifiers);
}

void UEdGraphNode_OasisDependency::GetAllPackageNames(TArray<FName>& OutPackageNames) const
{
	for (const FExtAssetIdentifier& AssetId : Identifiers)
	{
		if (AssetId.IsPackage())
		{
			OutPackageNames.AddUnique(AssetId.PackageName);
		}
	}
}

UEdGraph_OasisDependencyViewer* UEdGraphNode_OasisDependency::GetDependencyViewerGraph() const
{
	return Cast<UEdGraph_OasisDependencyViewer>( GetGraph() );
}

FText UEdGraphNode_OasisDependency::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NodeTitle;
}

FLinearColor UEdGraphNode_OasisDependency::GetNodeTitleColor() const
{
	if (bIsPrimaryAsset)
	{
		return FLinearColor(0.2f, 0.8f, 0.2f);
	}
	else if (bIsPackage)
	{
		return FLinearColor(0.4f, 0.62f, 1.0f);
	}
	else if (bIsCollapsed)
	{
		return FLinearColor(0.55f, 0.55f, 0.55f);
	}
	else 
	{
		return FLinearColor(0.0f, 0.55f, 0.62f);
	}
}

FText UEdGraphNode_OasisDependency::GetTooltipText() const
{
	FString TooltipString;
	for (const FExtAssetIdentifier& AssetId : Identifiers)
	{
		if (!TooltipString.IsEmpty())
		{
			TooltipString.Append(TEXT("\n"));
		}
		TooltipString.Append(AssetId.ToString());
	}
	return FText::FromString(TooltipString);
}

void UEdGraphNode_OasisDependency::AllocateDefaultPins()
{
	ReferencerPin = CreatePin( EEdGraphPinDirection::EGPD_Input, NAME_None, NAME_None);
	DependencyPin = CreatePin( EEdGraphPinDirection::EGPD_Output, NAME_None, NAME_None);

	ReferencerPin->bHidden = true;
	DependencyPin->bHidden = true;
}

UObject* UEdGraphNode_OasisDependency::GetJumpTargetForDoubleClick() const
{
#if ECB_WIP_REF_VIEWER_JUMP
	if (Identifiers.Num() > 0 )
	{
		GetDependencyViewerGraph()->SetGraphRoot(Identifiers, FIntPoint(NodePosX, NodePosY));
		GetDependencyViewerGraph()->RebuildGraph();
	}
#endif
	return NULL;
}

UEdGraphPin* UEdGraphNode_OasisDependency::GetDependencyPin()
{
	return DependencyPin;
}

UEdGraphPin* UEdGraphNode_OasisDependency::GetReferencerPin()
{
	return ReferencerPin;
}

void UEdGraphNode_OasisDependency::CacheAssetData(const FOasisAssetData& AssetData)
{
	CachedAssetData = AssetData;
	bUsesThumbnail = false;

	if ( AssetData.IsValid()/* && IsPackage()*/ )
	{
		bUsesThumbnail = true;
		bIsMissingOrInvalid = false;
	}
	else
	{
		if (Identifiers.Num() == 1 )
		{
			const FString PackageNameStr = Identifiers[0].PackageName.ToString();
			if ( FPackageName::IsValidLongPackageName(PackageNameStr, true) )
			{
				bool bIsMapPackage = false;

				const bool bIsScriptPackage = PackageNameStr.StartsWith(TEXT("/Script"));

				if (bIsScriptPackage)
				{
					CachedAssetData.AssetClass = FName(TEXT("Code"));

					bIsMissingOrInvalid = !FOasisPackageUtils::DoesPackageExist(PackageNameStr);
					if (bIsMissingOrInvalid)
					{
						CachedAssetData.AssetClass = FName(TEXT("Code Missing"));
					}
				}
				else
				{
					const FString PotentiallyMapFilename = FPackageName::LongPackageNameToFilename(PackageNameStr, FPackageName::GetMapPackageExtension());
					bIsMapPackage = FPlatformFileManager::Get().GetPlatformFile().FileExists(*PotentiallyMapFilename);
					if ( bIsMapPackage )
					{
						CachedAssetData.AssetClass = FName(TEXT("World"));

						bIsMissingOrInvalid = !FOasisPackageUtils::DoesPackageExist(PackageNameStr);
						if (bIsMissingOrInvalid)
						{
							CachedAssetData.AssetClass = FName(TEXT("Map Missing"));
						}
					}
				}

				if (!bIsScriptPackage && !bIsMapPackage)
				{
					CachedAssetData = FOasisAssetData(Identifiers[0].PackageName);
					bUsesThumbnail = CachedAssetData.HasThumbnail();

					const FString PackageFileStr = CachedAssetData.PackageFilePath.ToString();
					if (CachedAssetData.IsUAsset())
					{
						bIsMissingOrInvalid = !FExtAssetValidator::ValidateDependency({ CachedAssetData });
						if (bIsMissingOrInvalid)
						{
							CachedAssetData.AssetClass = FName(*CachedAssetData.GetInvalidReason());
						}
					}
					else
					{
						bIsMissingOrInvalid = !FOasisPackageUtils::DoesPackageExist(PackageNameStr);
						if (bIsMissingOrInvalid)
						{
							CachedAssetData.AssetClass = FName(TEXT("Package Missing"));
						}
					}
				}
			}
		}
		else
		{
			CachedAssetData = FOasisAssetData();
			CachedAssetData.AssetClass = FName(TEXT("Multiple Nodes"));
		}
	}
}

FOasisAssetData UEdGraphNode_OasisDependency::GetAssetData() const
{
	return CachedAssetData;
}

bool UEdGraphNode_OasisDependency::UsesThumbnail() const
{
	return bUsesThumbnail;
}

bool UEdGraphNode_OasisDependency::IsPackage() const
{
	return bIsPackage;
}

bool UEdGraphNode_OasisDependency::IsCollapsed() const
{
	return bIsCollapsed;
}

bool UEdGraphNode_OasisDependency::IsMissingOrInvalid() const
{
	return bIsMissingOrInvalid;
}

#undef LOCTEXT_NAMESPACE
