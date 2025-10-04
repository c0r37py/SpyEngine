// Innerloop Interactive Production :: c0r37py

#include "EdGraph_OasisDependencyViewer.h"
#include "EdGraphNode_OasisDependency.h"
#include "OasisContentBrowserSingleton.h"
#include "OasisAssetData.h"
#include "OasisAssetThumbnail.h"

#include "EdGraph/EdGraphPin.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetThumbnail.h"
#include "SOasisDependencyViewer.h"
#include "SOasisDependencyNode.h"
#include "GraphEditor.h"
#include "ICollectionManager.h"
#include "CollectionManagerModule.h"
#include "Engine/AssetManager.h"
#include "AssetManagerEditorModule.h"
#include "Logging/TokenizedMessage.h"

UEdGraph_OasisDependencyViewer::UEdGraph_OasisDependencyViewer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssetThumbnailPool = MakeShareable( new FOasisAssetThumbnailPool(1024) );

	MaxSearchDepth = 1;
	MaxSearchBreadth = 15;

	bLimitSearchDepth = true;
	bLimitSearchBreadth = false;
	bIsShowSoftReferences = true;
	bIsShowHardReferences = true;
	bIsShowManagementReferences = false;
	bIsShowSearchableNames = false;
	bIsShowNativePackages = true;

	NodeXSpacing = 400.f;
}

void UEdGraph_OasisDependencyViewer::BeginDestroy()
{
	AssetThumbnailPool.Reset();

	Super::BeginDestroy();
}

void UEdGraph_OasisDependencyViewer::SetGraphRoot(const TArray<FExtAssetIdentifier>& GraphRootIdentifiers, const FIntPoint& GraphRootOrigin)
{
	CurrentGraphRootIdentifiers = GraphRootIdentifiers;
	CurrentGraphRootOrigin = GraphRootOrigin;

	// If we're focused on a searchable name, enable that flag
	for (const FExtAssetIdentifier& AssetId : GraphRootIdentifiers)
	{
		if (AssetId.IsValue())
		{
			bIsShowSearchableNames = true;
		}
		else if (AssetId.GetPrimaryAssetId().IsValid())
		{
			if (UAssetManager::IsInitialized())
			{
				UAssetManager::Get().UpdateManagementDatabase();
			}
			
			bIsShowManagementReferences = true;
		}
	}
}

const TArray<FExtAssetIdentifier>& UEdGraph_OasisDependencyViewer::GetCurrentGraphRootIdentifiers() const
{
	return CurrentGraphRootIdentifiers;
}

void UEdGraph_OasisDependencyViewer::SetReferenceViewer(TSharedPtr<SOasisDependencyViewer> InViewer)
{
	ReferenceViewer = InViewer;
}

bool UEdGraph_OasisDependencyViewer::GetSelectedAssetsForMenuExtender(const class UEdGraphNode* Node, TArray<FExtAssetIdentifier>& SelectedAssets) const
{
	if (!ReferenceViewer.IsValid())
	{
		return false;
	}
	TSharedPtr<SGraphEditor> GraphEditor = ReferenceViewer.Pin()->GetGraphEditor();

	if (!GraphEditor.IsValid())
	{
		return false;
	}

	TSet<UObject*> SelectedNodes = GraphEditor->GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator It(SelectedNodes); It; ++It)
	{
		if (UEdGraphNode_OasisDependency* ReferenceNode = Cast<UEdGraphNode_OasisDependency>(*It))
		{
			if (!ReferenceNode->IsCollapsed())
			{
				SelectedAssets.Add(ReferenceNode->GetIdentifier());
			}
		}
	}
	return true;
}

UEdGraphNode_OasisDependency* UEdGraph_OasisDependencyViewer::RebuildGraph()
{
	RemoveAllNodes();
	UEdGraphNode_OasisDependency* NewRootNode = ConstructNodes(CurrentGraphRootIdentifiers, CurrentGraphRootOrigin);
	NotifyGraphChanged();

	return NewRootNode;
}

bool UEdGraph_OasisDependencyViewer::IsSearchDepthLimited() const
{
	return bLimitSearchDepth;
}

bool UEdGraph_OasisDependencyViewer::IsSearchBreadthLimited() const
{
	return bLimitSearchBreadth;
}

bool UEdGraph_OasisDependencyViewer::IsShowSoftReferences() const
{
	return bIsShowSoftReferences;
}

bool UEdGraph_OasisDependencyViewer::IsShowHardReferences() const
{
	return bIsShowHardReferences;
}

bool UEdGraph_OasisDependencyViewer::IsShowManagementReferences() const
{
	return bIsShowManagementReferences;
}

bool UEdGraph_OasisDependencyViewer::IsShowSearchableNames() const
{
	return bIsShowSearchableNames;
}

bool UEdGraph_OasisDependencyViewer::IsShowNativePackages() const
{
	return bIsShowNativePackages;
}

void UEdGraph_OasisDependencyViewer::SetSearchDepthLimitEnabled(bool newEnabled)
{
	bLimitSearchDepth = newEnabled;
}

void UEdGraph_OasisDependencyViewer::SetSearchBreadthLimitEnabled(bool newEnabled)
{
	bLimitSearchBreadth = newEnabled;
}

void UEdGraph_OasisDependencyViewer::SetShowSoftReferencesEnabled(bool newEnabled)
{
	bIsShowSoftReferences = newEnabled;
}

void UEdGraph_OasisDependencyViewer::SetShowHardReferencesEnabled(bool newEnabled)
{
	bIsShowHardReferences = newEnabled;
}

void UEdGraph_OasisDependencyViewer::SetShowManagementReferencesEnabled(bool newEnabled)
{
	bIsShowManagementReferences = newEnabled;
}

void UEdGraph_OasisDependencyViewer::SetShowSearchableNames(bool newEnabled)
{
	bIsShowSearchableNames = newEnabled;
}

void UEdGraph_OasisDependencyViewer::SetShowNativePackages(bool newEnabled)
{
	bIsShowNativePackages = newEnabled;
}

int32 UEdGraph_OasisDependencyViewer::GetSearchDepthLimit() const
{
	return MaxSearchDepth;
}

int32 UEdGraph_OasisDependencyViewer::GetSearchBreadthLimit() const
{
	return MaxSearchBreadth;
}

void UEdGraph_OasisDependencyViewer::SetSearchDepthLimit(int32 NewDepthLimit)
{
	MaxSearchDepth = NewDepthLimit;
}

void UEdGraph_OasisDependencyViewer::SetSearchBreadthLimit(int32 NewBreadthLimit)
{
	MaxSearchBreadth = NewBreadthLimit;
}

FName UEdGraph_OasisDependencyViewer::GetCurrentCollectionFilter() const
{
	return CurrentCollectionFilter;
}

void UEdGraph_OasisDependencyViewer::SetCurrentCollectionFilter(FName NewFilter)
{
	CurrentCollectionFilter = NewFilter;
}

bool UEdGraph_OasisDependencyViewer::GetEnableCollectionFilter() const
{
	return bEnableCollectionFilter;
}

void UEdGraph_OasisDependencyViewer::SetEnableCollectionFilter(bool bEnabled)
{
	bEnableCollectionFilter = bEnabled;
}

float UEdGraph_OasisDependencyViewer::GetNodeXSpacing() const
{
	return NodeXSpacing;
}

void UEdGraph_OasisDependencyViewer::SetNodeXSpacing(float NewNodeXSpacing)
{
#if ECB_FEA_REF_VIEWER_NODE_SPACING
	NodeXSpacing = FMath::Clamp<float>(NewNodeXSpacing, 200.f, 10000.f);
#endif
}

FAssetManagerDependencyQuery UEdGraph_OasisDependencyViewer::GetReferenceSearchFlags(bool bHardOnly) const
{
	using namespace UE::AssetRegistry;
	FAssetManagerDependencyQuery Query;
	Query.Categories = EDependencyCategory::None;
	Query.Flags = EDependencyQuery::NoRequirements;

	bool bLocalIsShowSoftReferences = bIsShowSoftReferences && !bHardOnly;
	if (bLocalIsShowSoftReferences || bIsShowHardReferences)
	{
		Query.Categories |= EDependencyCategory::Package;
		Query.Flags |= bLocalIsShowSoftReferences ? EDependencyQuery::NoRequirements : EDependencyQuery::Hard;
		Query.Flags |= bIsShowHardReferences ? EDependencyQuery::NoRequirements : EDependencyQuery::Soft;
		//Query.Flags |= Settings->IsShowEditorOnlyReferences() ? EDependencyQuery::NoRequirements : EDependencyQuery::Game;
	}

	if (bIsShowSearchableNames && !bHardOnly)
	{
		Query.Categories |= EDependencyCategory::SearchableName;
	}
	if (bIsShowManagementReferences)
	{
		Query.Categories |= EDependencyCategory::Manage;
		Query.Flags |= bHardOnly ? EDependencyQuery::Direct : EDependencyQuery::NoRequirements;
	}

	return Query;
}

UEdGraphNode_OasisDependency* UEdGraph_OasisDependencyViewer::ConstructNodes(const TArray<FExtAssetIdentifier>& GraphRootIdentifiers, const FIntPoint& GraphRootOrigin )
{
	UEdGraphNode_OasisDependency* RootNode = NULL;

	if (GraphRootIdentifiers.Num() > 0 )
	{
		TSet<FName> AllowedPackageNames;
		if (ShouldFilterByCollection())
		{
			FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
			TArray<FSoftObjectPath> AssetPaths;
			CollectionManagerModule.Get().GetAssetsInCollection(CurrentCollectionFilter, ECollectionShareType::CST_All, AssetPaths);
			AllowedPackageNames.Reserve(AssetPaths.Num());
			for (FSoftObjectPath& AssetPath : AssetPaths)
			{
				AllowedPackageNames.Add(FName(*FPackageName::ObjectPathToPackageName(AssetPath.ToString())));
			}
		}

		TMap<FExtAssetIdentifier, int32> ReferencerNodeSizes;
		TSet<FExtAssetIdentifier> VisitedReferencerSizeNames;
		int32 ReferencerDepth = 1;
		RecursivelyGatherSizes(/*bReferencers=*/true, GraphRootIdentifiers, AllowedPackageNames, ReferencerDepth, VisitedReferencerSizeNames, ReferencerNodeSizes);

		TMap<FExtAssetIdentifier, int32> DependencyNodeSizes;
		TSet<FExtAssetIdentifier> VisitedDependencySizeNames;
		int32 DependencyDepth = 1;
		RecursivelyGatherSizes(/*bReferencers=*/false, GraphRootIdentifiers, AllowedPackageNames, DependencyDepth, VisitedDependencySizeNames, DependencyNodeSizes);

		TSet<FName> AllPackageNames;

		auto AddPackage = [](const FExtAssetIdentifier& AssetId, TSet<FName>& PackageNames)
		{ 
			// Only look for asset data if this is a package
			if (!AssetId.IsValue())
			{
				PackageNames.Add(AssetId.PackageName);
			}
		};

		for (const FExtAssetIdentifier& AssetId : VisitedReferencerSizeNames)
		{
			AddPackage(AssetId, AllPackageNames);
		}

		for (const FExtAssetIdentifier& AssetId : VisitedDependencySizeNames)
		{
			AddPackage(AssetId, AllPackageNames);
		}

		TMap<FName, FOasisAssetData> PackagesToAssetDataMap;
		GatherAssetData(AllPackageNames, PackagesToAssetDataMap);

		// Create the root node
		RootNode = CreateReferenceNode();
		RootNode->SetupReferenceNode(GraphRootOrigin, GraphRootIdentifiers, PackagesToAssetDataMap.FindRef(GraphRootIdentifiers[0].PackageName));

		TSet<FExtAssetIdentifier> VisitedReferencerNames;
		int32 VisitedReferencerDepth = 1;
		RecursivelyConstructNodes(/*bReferencers=*/true, RootNode, GraphRootIdentifiers, GraphRootOrigin, ReferencerNodeSizes, PackagesToAssetDataMap, AllowedPackageNames, VisitedReferencerDepth, VisitedReferencerNames);

		TSet<FExtAssetIdentifier> VisitedDependencyNames;
		int32 VisitedDependencyDepth = 1;
		RecursivelyConstructNodes(/*bReferencers=*/false, RootNode, GraphRootIdentifiers, GraphRootOrigin, DependencyNodeSizes, PackagesToAssetDataMap, AllowedPackageNames, VisitedDependencyDepth, VisitedDependencyNames);
	}

	return RootNode;
}

int32 UEdGraph_OasisDependencyViewer::RecursivelyGatherSizes(bool bReferencers, const TArray<FExtAssetIdentifier>& Identifiers, const TSet<FName>& AllowedPackageNames, int32 CurrentDepth, TSet<FExtAssetIdentifier>& VisitedNames, TMap<FExtAssetIdentifier, int32>& OutNodeSizes) const
{
	check(Identifiers.Num() > 0);

	VisitedNames.Append(Identifiers);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FExtAssetIdentifier> ReferenceNames;

	const FAssetManagerDependencyQuery DependencyQuery = GetReferenceSearchFlags(false);
	if ( bReferencers )
	{
		for (const FExtAssetIdentifier& AssetId : Identifiers)
		{
			FOasisContentBrowserSingleton::GetAssetRegistry().GetReferencers(AssetId, ReferenceNames, DependencyQuery.Categories);
		}
	}
	else
	{
		for (const FExtAssetIdentifier& AssetId : Identifiers)
		{
			FOasisContentBrowserSingleton::GetAssetRegistry().GetDependencies(AssetId, ReferenceNames, DependencyQuery.Categories);
		}
	}

	if (!bIsShowNativePackages)
	{
		auto RemoveNativePackage = [](const FExtAssetIdentifier& InAsset) { return InAsset.PackageName.ToString().StartsWith(TEXT("/Script")) && !InAsset.IsValue(); };

		ReferenceNames.RemoveAll(RemoveNativePackage);
	}

	int32 NodeSize = 0;
	if ( ReferenceNames.Num() > 0 && !ExceedsMaxSearchDepth(CurrentDepth) )
	{
		int32 NumReferencesMade = 0;
		int32 NumReferencesExceedingMax = 0;
#if ECB_LEGACY
		// Filter for our registry source
		IAssetManagerEditorModule::Get().FilterAssetIdentifiersForCurrentRegistrySource(ReferenceNames, GetReferenceSearchFlags(false), !bReferencers);
#endif
		// Since there are referencers, use the size of all your combined referencers.
		// Do not count your own size since there could just be a horizontal line of nodes
		for (FExtAssetIdentifier& AssetId : ReferenceNames)
		{
			if ( !VisitedNames.Contains(AssetId) && (!AssetId.IsPackage() || !ShouldFilterByCollection() || AllowedPackageNames.Contains(AssetId.PackageName)) )
			{
				if ( !ExceedsMaxSearchBreadth(NumReferencesMade) )
				{
					TArray<FExtAssetIdentifier> NewPackageNames;
					NewPackageNames.Add(AssetId);
					NodeSize += RecursivelyGatherSizes(bReferencers, NewPackageNames, AllowedPackageNames, CurrentDepth + 1, VisitedNames, OutNodeSizes);
					NumReferencesMade++;
				}
				else
				{
					NumReferencesExceedingMax++;
				}
			}
		}

		if ( NumReferencesExceedingMax > 0 )
		{
			// Add one size for the collapsed node
			NodeSize++;
		}
	}

	if ( NodeSize == 0 )
	{
		// If you have no valid children, the node size is just 1 (counting only self to make a straight line)
		NodeSize = 1;
	}

	OutNodeSizes.Add(Identifiers[0], NodeSize);
	return NodeSize;
}

void UEdGraph_OasisDependencyViewer::GatherAssetData(const TSet<FName>& AllPackageNames, TMap<FName, FOasisAssetData>& OutPackageToAssetDataMap) const
{
	FARFilter Filter;
	for ( auto PackageIt = AllPackageNames.CreateConstIterator(); PackageIt; ++PackageIt )
	{
		const FString& PackageName = (*PackageIt).ToString();
		//const FString& PackagePath = PackageName + TEXT(".") + FPackageName::GetLongPackageAssetName(PackageName);
		//Filter.ObjectPaths.Add( FName(*PackagePath) );
		Filter.PackageNames.Add(FName(*PackageName));
	}

	TArray<FOasisAssetData> AssetDataList;
	FOasisContentBrowserSingleton::GetAssetRegistry().GetAssets(Filter, AssetDataList);
	for ( auto AssetIt = AssetDataList.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		OutPackageToAssetDataMap.Add((*AssetIt).PackageName, *AssetIt);
	}
}

UEdGraphNode_OasisDependency* UEdGraph_OasisDependencyViewer::RecursivelyConstructNodes(bool bReferencers, UEdGraphNode_OasisDependency* RootNode, const TArray<FExtAssetIdentifier>& Identifiers, const FIntPoint& NodeLoc, const TMap<FExtAssetIdentifier, int32>& NodeSizes, const TMap<FName, FOasisAssetData>& PackagesToAssetDataMap, const TSet<FName>& AllowedPackageNames, int32 CurrentDepth, TSet<FExtAssetIdentifier>& VisitedNames)
{
	check(Identifiers.Num() > 0);

	VisitedNames.Append(Identifiers);

	UEdGraphNode_OasisDependency* NewNode = NULL;
	if ( RootNode->GetIdentifier() == Identifiers[0] )
	{
		// Don't create the root node. It is already created!
		NewNode = RootNode;
	}
	else
	{
		NewNode = CreateReferenceNode();
		NewNode->SetupReferenceNode(NodeLoc, Identifiers, PackagesToAssetDataMap.FindRef(Identifiers[0].PackageName));
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FExtAssetIdentifier> ReferenceNames;
	TArray<FExtAssetIdentifier> HardReferenceNames;

	const FAssetManagerDependencyQuery HardOnlyDependencyQuery = GetReferenceSearchFlags(true);
	const FAssetManagerDependencyQuery DependencyQuery = GetReferenceSearchFlags(false);
	if ( bReferencers )
	{
		for (const FExtAssetIdentifier& AssetId : Identifiers)
		{
			FOasisContentBrowserSingleton::GetAssetRegistry().GetReferencers(AssetId, HardReferenceNames, HardOnlyDependencyQuery.Categories);
			FOasisContentBrowserSingleton::GetAssetRegistry().GetReferencers(AssetId, ReferenceNames, DependencyQuery.Categories);
		}
	}
	else
	{
		for (const FExtAssetIdentifier& AssetId : Identifiers)
		{
			FOasisContentBrowserSingleton::GetAssetRegistry().GetDependencies(AssetId, HardReferenceNames, HardOnlyDependencyQuery.Categories);
			FOasisContentBrowserSingleton::GetAssetRegistry().GetDependencies(AssetId, ReferenceNames, DependencyQuery.Categories);
		}
	}

	if (!bIsShowNativePackages)
	{
		auto RemoveNativePackage = [](const FExtAssetIdentifier& InAsset) { return InAsset.PackageName.ToString().StartsWith(TEXT("/Script")) && !InAsset.IsValue(); };

		HardReferenceNames.RemoveAll(RemoveNativePackage);
		ReferenceNames.RemoveAll(RemoveNativePackage);
	}

	if ( ReferenceNames.Num() > 0 && !ExceedsMaxSearchDepth(CurrentDepth) )
	{
		FIntPoint ReferenceNodeLoc = NodeLoc;

		if ( bReferencers )
		{
			// Referencers go left
			ReferenceNodeLoc.X -= NodeXSpacing;
		}
		else
		{
			// Dependencies go right
			ReferenceNodeLoc.X += NodeXSpacing;
		}

		const int32 NodeSizeY = 200;
		const int32 TotalReferenceSizeY = NodeSizes.FindChecked(Identifiers[0]) * NodeSizeY;

		ReferenceNodeLoc.Y -= TotalReferenceSizeY * 0.5f;
		ReferenceNodeLoc.Y += NodeSizeY * 0.5f;

		int32 NumReferencesMade = 0;
		int32 NumReferencesExceedingMax = 0;

#if ECB_LEGACY
		// Filter for our registry source
		IAssetManagerEditorModule::Get().FilterAssetIdentifiersForCurrentRegistrySource(ReferenceNames, GetReferenceSearchFlags(false), !bReferencers);
		IAssetManagerEditorModule::Get().FilterAssetIdentifiersForCurrentRegistrySource(HardReferenceNames, GetReferenceSearchFlags(false), !bReferencers);
#endif

		for ( int32 RefIdx = 0; RefIdx < ReferenceNames.Num(); ++RefIdx )
		{
			FExtAssetIdentifier ReferenceName = ReferenceNames[RefIdx];

			if ( !VisitedNames.Contains(ReferenceName) && (!ReferenceName.IsPackage() || !ShouldFilterByCollection() || AllowedPackageNames.Contains(ReferenceName.PackageName)) )
			{
				bool bIsHardReference = HardReferenceNames.Contains(ReferenceName);

				if ( !ExceedsMaxSearchBreadth(NumReferencesMade) )
				{
					int32 ThisNodeSizeY = ReferenceName.IsValue() ? 100 : NodeSizeY;

					const int32 RefSizeY = NodeSizes.FindChecked(ReferenceName);
					FIntPoint RefNodeLoc;
					RefNodeLoc.X = ReferenceNodeLoc.X;
					RefNodeLoc.Y = ReferenceNodeLoc.Y + RefSizeY * ThisNodeSizeY * 0.5 - ThisNodeSizeY * 0.5;
					
					TArray<FExtAssetIdentifier> NewIdentifiers;
					NewIdentifiers.Add(ReferenceName);
					
					UEdGraphNode_OasisDependency* ReferenceNode = RecursivelyConstructNodes(bReferencers, RootNode, NewIdentifiers, RefNodeLoc, NodeSizes, PackagesToAssetDataMap, AllowedPackageNames, CurrentDepth + 1, VisitedNames);
					if (bIsHardReference)
					{
						if (bReferencers)
						{
							ReferenceNode->GetDependencyPin()->PinType.PinCategory = TEXT("hard");
						}
						else
						{
							ReferenceNode->GetReferencerPin()->PinType.PinCategory = TEXT("hard"); //-V595
						}
					}

					bool bIsMissingOrInvalid = ReferenceNode->IsMissingOrInvalid();
					if (bIsMissingOrInvalid)
					{
						if (bReferencers)
						{
							ReferenceNode->GetDependencyPin()->PinType.PinSubCategory = TEXT("invalid");
						}
						else
						{
							ReferenceNode->GetReferencerPin()->PinType.PinSubCategory = TEXT("invalid");
						}

						ReferenceNode->bHasCompilerMessage = true;
						if (!bIsHardReference)
						{
							ReferenceNode->ErrorType = EMessageSeverity::Warning;
						}
						else
						{
							ReferenceNode->ErrorType = EMessageSeverity::Error;
						}
					}
					
					if ( ensure(ReferenceNode) )
					{
						if ( bReferencers )
						{
							NewNode->AddReferencer( ReferenceNode );
						}
						else
						{
							ReferenceNode->AddReferencer( NewNode );
						}

						ReferenceNodeLoc.Y += RefSizeY * ThisNodeSizeY;
					}

					NumReferencesMade++;
				}
				else
				{
					NumReferencesExceedingMax++;
				}
			}
		}

		if ( NumReferencesExceedingMax > 0 )
		{
			// There are more references than allowed to be displayed. Make a collapsed node.
			UEdGraphNode_OasisDependency* ReferenceNode = CreateReferenceNode();
			FIntPoint RefNodeLoc;
			RefNodeLoc.X = ReferenceNodeLoc.X;
			RefNodeLoc.Y = ReferenceNodeLoc.Y;

			if ( ensure(ReferenceNode) )
			{
				ReferenceNode->SetReferenceNodeCollapsed(RefNodeLoc, NumReferencesExceedingMax);

				if ( bReferencers )
				{
					NewNode->AddReferencer( ReferenceNode );
				}
				else
				{
					ReferenceNode->AddReferencer( NewNode );
				}
			}
		}
	}

	return NewNode;
}

const TSharedPtr<FOasisAssetThumbnailPool>& UEdGraph_OasisDependencyViewer::GetAssetThumbnailPool() const
{
	return AssetThumbnailPool;
}

bool UEdGraph_OasisDependencyViewer::ExceedsMaxSearchDepth(int32 Depth) const
{
	return bLimitSearchDepth && Depth > MaxSearchDepth;
}

bool UEdGraph_OasisDependencyViewer::ExceedsMaxSearchBreadth(int32 Breadth) const
{
	return bLimitSearchBreadth && Breadth > MaxSearchBreadth;
}

UEdGraphNode_OasisDependency* UEdGraph_OasisDependencyViewer::CreateReferenceNode()
{
	const bool bSelectNewNode = false;
	return Cast<UEdGraphNode_OasisDependency>(CreateNode(UEdGraphNode_OasisDependency::StaticClass(), bSelectNewNode));
}

void UEdGraph_OasisDependencyViewer::RemoveAllNodes()
{
	TArray<UEdGraphNode*> NodesToRemove = Nodes;
	for (int32 NodeIndex = 0; NodeIndex < NodesToRemove.Num(); ++NodeIndex)
	{
		RemoveNode(NodesToRemove[NodeIndex]);
	}
}

bool UEdGraph_OasisDependencyViewer::ShouldFilterByCollection() const
{
	return bEnableCollectionFilter && CurrentCollectionFilter != NAME_None;
}
