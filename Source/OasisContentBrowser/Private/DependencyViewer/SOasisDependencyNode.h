// Innerloop Interactive Production :: c0r37py

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SGraphNode.h"
#include "EdGraphUtilities.h"

class FOasisAssetThumbnail;
class UEdGraphNode_OasisDependency;

/**
 * A GraphNode representing an ext reference node
 */
class SOasisDependencyNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS( SOasisDependencyNode ){}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs, UEdGraphNode_OasisDependency* InNode );

	// SGraphNode implementation
	virtual void UpdateGraphNode() override;
	virtual bool IsNodeEditable() const override { return false; }
	// End SGraphNode implementation

private:
	FSlateColor GetNodeTitleBackgroundColor() const;
	FSlateColor GetNodeOverlayColor() const;
	FSlateColor GetNodeBodyBackgroundColor() const;

private:
	TSharedPtr<class FOasisAssetThumbnail> AssetThumbnail;
};

/**
 * SOasisDependencyNode Factory
 */
class FExtDependencyGraphPanelNodeFactory : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};


