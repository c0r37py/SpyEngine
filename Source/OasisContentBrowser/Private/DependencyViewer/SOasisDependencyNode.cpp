// Innerloop Interactive Production :: c0r37py

#include "SOasisDependencyNode.h"
#include "OasisContentBrowser.h"
#include "EdGraphNode_OasisDependency.h"
#include "OasisAssetThumbnail.h"
#include "OasisContentBrowserSettings.h"
#include "OasisContentBrowserStyle.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "SCommentBubble.h"

#define LOCTEXT_NAMESPACE "ExtDependencyViewer"

void SOasisDependencyNode::Construct( const FArguments& InArgs, UEdGraphNode_OasisDependency* InNode )
{
	const int32 ThumbnailSize = 128;

	ECB_LOG(Display, TEXT("SOasisDependencyNode::Construct: %s"), *InNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

	if (InNode->UsesThumbnail())
	{
		// Create a thumbnail from the graph's thumbnail pool
		TSharedPtr<FOasisAssetThumbnailPool> AssetThumbnailPool = InNode->GetDependencyViewerGraph()->GetAssetThumbnailPool();
		AssetThumbnail = MakeShareable( new FOasisAssetThumbnail( InNode->GetAssetData(), ThumbnailSize, ThumbnailSize, AssetThumbnailPool ) );
	}
	else if (InNode->IsPackage() || InNode->IsCollapsed())
	{
		// Just make a generic thumbnail
		AssetThumbnail = MakeShareable( new FOasisAssetThumbnail( InNode->GetAssetData(), ThumbnailSize, ThumbnailSize, NULL ) );
	}

	GraphNode = InNode;
	SetCursor( EMouseCursor::CardinalCross );
	UpdateGraphNode();
}

void SOasisDependencyNode::UpdateGraphNode()
{
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	UpdateErrorInfo();

	//
	//             ______________________
	//            |      TITLE AREA      |
	//            +-------+------+-------+
	//            | (>) L |      | R (>) |
	//            | (>) E |      | I (>) |
	//            | (>) F |      | G (>) |
	//            | (>) T |      | H (>) |
	//            |       |      | T (>) |
	//            |_______|______|_______|
	//
	TSharedPtr<SVerticalBox> MainVerticalBox;
	TSharedPtr<SErrorText> ErrorText;
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	const float LeftRightWidth = 10.f;

	TSharedRef<SWidget> ThumbnailWidget = SNullWidget::NullWidget;
	if ( AssetThumbnail.IsValid() )
	{
		UEdGraphNode_OasisDependency* RefGraphNode = CastChecked<UEdGraphNode_OasisDependency>(GraphNode);

		FOasisAssetThumbnailConfig ThumbnailConfig;
		ThumbnailConfig.bAllowFadeIn = RefGraphNode->UsesThumbnail();
		ThumbnailConfig.bForceGenericThumbnail = !RefGraphNode->UsesThumbnail();
		ThumbnailConfig.bAllowAssetSpecificThumbnailOverlay = false; // Disable asset thumbnail overlay

		ThumbnailWidget =
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.Node.Body"))
			.Padding(0)
			[
				SNew(SBox)
				.WidthOverride(AssetThumbnail->GetSize().X)
				.HeightOverride(AssetThumbnail->GetSize().Y)
				[
					AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig)
				]
			];
	}

	ContentScale.Bind( this, &SOasisDependencyNode::GetContentScale );
	GetOrAddSlot( ENodeZone::Center )
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SOverlay)

		// MainVerticalBox Overlay >>
#if ECB_FOLD
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[

		SAssignNew(MainVerticalBox, SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush( "Graph.Node.Body" ) )
			.Padding(0)
			[
				SNew(SVerticalBox)
				. ToolTipText( this, &SOasisDependencyNode::GetNodeTooltip )
				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					SNew(SOverlay)
// 					+SOverlay::Slot()
// 					[
// 						SNew(SImage)
// 						.Image( FAppStyle::GetBrush("Graph.Node.TitleGloss") )
// 					]
					+SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("Graph.Node.ColorSpill") )
						// The extra margin on the right
						// is for making the color spill stretch well past the node title
						//.Padding( FMargin(10,5,30,3) )
						.Padding(FMargin(10, 5, 5, 3))
						.BorderBackgroundColor( this, &SOasisDependencyNode::GetNodeTitleBackgroundColor)
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.WidthOverride(AssetThumbnail->GetSize().X - 5.f)
								[
									SAssignNew(InlineEditableText, SInlineEditableTextBlock)
									.Style(FAppStyle::Get(), "Graph.Node.NodeTitleInlineEditableText")
									.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
									.OnVerifyTextChanged(this, &SOasisDependencyNode::OnVerifyNameTextChanged)
									.OnTextCommitted(this, &SOasisDependencyNode::OnNameTextCommited)
									.IsReadOnly(this, &SOasisDependencyNode::IsNameReadOnly)
									.IsSelected(this, &SOasisDependencyNode::IsSelectedExclusively)
								]
							]
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								NodeTitle.ToSharedRef()
							]
						]
					]
// 					+SOverlay::Slot()
// 					.VAlign(VAlign_Top)
// 					[
// 						SNew(SBorder)
// 						.BorderImage( FAppStyle::GetBrush( "Graph.Node.TitleHighlight" ) )
// 						.Visibility(EVisibility::HitTestInvisible)			
// 						[
// 							SNew(SSpacer)
// 							.Size(FVector2D(20,20))
// 						]
// 					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(1.0f)
				[
					// POPUP ERROR MESSAGE
					SAssignNew(ErrorText, SErrorText )
					.BackgroundColor( this, &SOasisDependencyNode::GetErrorColor )
					.ToolTipText( this, &SOasisDependencyNode::GetErrorMsgToolTip )
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					// NODE CONTENT AREA
					SNew(SBorder)
					//.BorderImage( FAppStyle::GetBrush("NoBorder") )
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(this, &SOasisDependencyNode::GetNodeBodyBackgroundColor)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.Padding( FMargin(0,3) )
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							// LEFT
							SNew(SBox)
							.WidthOverride(LeftRightWidth)
							[
								SAssignNew(LeftNodeBox, SVerticalBox)
							]
						]
						
						+SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.FillWidth(1.0f)
						[
							// Thumbnail
							ThumbnailWidget
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							// RIGHT
							SNew(SBox)
							.WidthOverride(LeftRightWidth)
							[
								SAssignNew(RightNodeBox, SVerticalBox)
							]
						]
					]
				]
			]
		]
		]
#endif	// MainVerticalBox Overlay <<

		// Tint Overlay >>
#if ECB_WIP_REF_VIEWER_HIGHLIGHT_ERROR
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(this, &SOasisDependencyNode::GetNodeOverlayColor)
			.Padding(FMargin(4.0f, 0.0f))
			//.Visibility(this, &SGraphNode_BehaviorTree::GetDebuggerSearchFailedMarkerVisibility)
			[
				SNew(SBox)
				.HeightOverride(4)
			]
		]
#endif	// Tint Overlay >>

	];
#if ECB_LEGACY	
	// Create comment bubble if comment text is valid
	GetNodeObj()->bCommentBubbleVisible = !GetNodeObj()->NodeComment.IsEmpty();
	if( GetNodeObj()->bCommentBubbleVisible )
	{
		TSharedPtr<SCommentBubble> CommentBubble;

		SAssignNew( CommentBubble, SCommentBubble )
		.GraphNode( GraphNode )
		.Text( this, &SGraphNode::GetNodeComment )
		.ColorAndOpacity( this, &SOasisDependencyNode::GetCommentColor );

		GetOrAddSlot( ENodeZone::TopCenter )
		.SlotOffset( TAttribute<FVector2D>( CommentBubble.Get(), &SCommentBubble::GetOffset ))
		.SlotSize( TAttribute<FVector2D>( CommentBubble.Get(), &SCommentBubble::GetSize ))
		.AllowScaling( TAttribute<bool>( CommentBubble.Get(), &SCommentBubble::IsScalingAllowed ))
		.VAlign( VAlign_Top )
		[
			CommentBubble.ToSharedRef()
		];
	}
#endif	

	ErrorReporting = ErrorText;
	ErrorReporting->SetError(ErrorMsg);
	CreateBelowWidgetControls(MainVerticalBox);

	CreatePinWidgets();
}

FSlateColor SOasisDependencyNode::GetNodeTitleBackgroundColor() const
{
	return FLinearColor(0.3f, 0.3f, 0.3f, 0.8f);
}

FSlateColor SOasisDependencyNode::GetNodeOverlayColor() const
{
#if ECB_WIP_REF_VIEWER_HIGHLIGHT_ERROR
	if (UEdGraphNode_OasisDependency* RefGraphNode = CastChecked<UEdGraphNode_OasisDependency>(GraphNode))
	{
		if (!RefGraphNode->IsMissingOrInvalid() && GetDefault<UOasisContentBrowserSettings>()->HighlightErrorNodesInDependencyViewer)
		{
			return FLinearColor(0.f, 0.f, 0.f, 0.5f);
		}
	}
#endif	
	return FLinearColor(0.f, 0.f, 0.f, 0.f);
}

FSlateColor SOasisDependencyNode::GetNodeBodyBackgroundColor() const
{
	if (UEdGraphNode_OasisDependency* RefGraphNode = CastChecked<UEdGraphNode_OasisDependency>(GraphNode))
	{
		if (RefGraphNode->IsMissingOrInvalid())
		{
			if (RefGraphNode->GetDependencyPin()->PinType.PinCategory == TEXT("hard")
				|| RefGraphNode->GetReferencerPin()->PinType.PinCategory == TEXT("hard"))
			{
				return FOasisContentBrowserStyle::Get().GetColor("ErrorReporting.HardReferenceColor.Darker");
			}
			else
			{
				return FOasisContentBrowserStyle::Get().GetColor("ErrorReporting.SoftReferenceColor.Darker");
			}
		}
	}

	return FLinearColor(0.f, 0.f, 0.f, 1.f);
}

TSharedPtr<class SGraphNode> FExtDependencyGraphPanelNodeFactory::CreateNode(UEdGraphNode* Node) const

{
	if (UEdGraphNode_OasisDependency* DependencyNode = Cast<UEdGraphNode_OasisDependency>(Node))
	{
		return SNew(SOasisDependencyNode, DependencyNode);
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE

