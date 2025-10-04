// Innerloop Interactive Production :: c0r37py

#pragma once

#include "OasisAssetData.h"

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph_OasisDependencyViewer.h"

#include "EdGraphNode_OasisDependency.generated.h"

class UEdGraphPin;

UCLASS()
class UEdGraphNode_OasisDependency : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

	/** Returns first asset identifier */
	FExtAssetIdentifier GetIdentifier() const;
	
	/** Returns all identifiers on this node including virtual things */
	void GetAllIdentifiers(TArray<FExtAssetIdentifier>& OutIdentifiers) const;

	/** Returns only the packages in this node, skips searchable names */
	void GetAllPackageNames(TArray<FName>& OutPackageNames) const;

	/** Returns our owning graph */
	UEdGraph_OasisDependencyViewer* GetDependencyViewerGraph() const;

	// UEdGraphNode implementation
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual void AllocateDefaultPins() override;
	virtual UObject* GetJumpTargetForDoubleClick() const override;
	// End UEdGraphNode implementation

	bool UsesThumbnail() const;
	bool IsPackage() const;
	bool IsCollapsed() const;
	bool IsMissingOrInvalid() const;
	FOasisAssetData GetAssetData() const;

	UEdGraphPin* GetDependencyPin();
	UEdGraphPin* GetReferencerPin();

private:
	void CacheAssetData(const FOasisAssetData& AssetData);
	void SetupReferenceNode(const FIntPoint& NodeLoc, const TArray<FExtAssetIdentifier>& NewIdentifiers, const FOasisAssetData& InAssetData);
	void SetReferenceNodeCollapsed(const FIntPoint& NodeLoc, int32 InNumReferencesExceedingMax);
	void AddReferencer(class UEdGraphNode_OasisDependency* ReferencerNode);

	TArray<FExtAssetIdentifier> Identifiers;
	FText NodeTitle;

	bool bUsesThumbnail;
	bool bIsPackage;
	bool bIsPrimaryAsset;
	bool bIsCollapsed;
	bool bIsMissingOrInvalid;
	FOasisAssetData CachedAssetData;

	UEdGraphPin* DependencyPin;
	UEdGraphPin* ReferencerPin;

	friend UEdGraph_OasisDependencyViewer;
};


