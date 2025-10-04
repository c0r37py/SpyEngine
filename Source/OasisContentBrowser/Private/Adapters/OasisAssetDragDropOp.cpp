// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "OasisAssetDragDropOp.h"
#include "OasisAssetThumbnail.h"
#include "OasisContentBrowser.h"

#include "Engine/Level.h"
#include "ActorFactories/ActorFactory.h"
#include "GameFramework/Actor.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "ClassIconFinder.h"

TSharedRef<FOasisAssetDragDropOp> FOasisAssetDragDropOp::New(const FOasisAssetData& InAssetData, UActorFactory* ActorFactory)
{
	TArray<FOasisAssetData> AssetDataArray;
	AssetDataArray.Emplace(InAssetData);
	return New(MoveTemp(AssetDataArray), TArray<FString>(), ActorFactory);
}

TSharedRef<FOasisAssetDragDropOp> FOasisAssetDragDropOp::New(TArray<FOasisAssetData> InAssetData, UActorFactory* ActorFactory)
{
	return New(MoveTemp(InAssetData), TArray<FString>(), ActorFactory);
}

TSharedRef<FOasisAssetDragDropOp> FOasisAssetDragDropOp::New(FString InAssetPath)
{
	TArray<FString> AssetPathsArray;
	AssetPathsArray.Emplace(MoveTemp(InAssetPath));
	return New(TArray<FOasisAssetData>(), MoveTemp(AssetPathsArray), nullptr);
}

TSharedRef<FOasisAssetDragDropOp> FOasisAssetDragDropOp::New(TArray<FString> InAssetPaths)
{
	return New(TArray<FOasisAssetData>(), MoveTemp(InAssetPaths), nullptr);
}

TSharedRef<FOasisAssetDragDropOp> FOasisAssetDragDropOp::New(TArray<FOasisAssetData> InAssetData, TArray<FString> InAssetPaths, UActorFactory* ActorFactory)
{
	TSharedRef<FOasisAssetDragDropOp> Operation = MakeShared<FOasisAssetDragDropOp>();

	Operation->MouseCursor = EMouseCursor::GrabHandClosed;

	Operation->ThumbnailSize = 64;

	Operation->AssetData = MoveTemp(InAssetData);
	Operation->AssetPaths = MoveTemp(InAssetPaths);
	Operation->ActorFactory = ActorFactory;

	Operation->Init();

	Operation->Construct();
	return Operation;
}

FOasisAssetDragDropOp::~FOasisAssetDragDropOp()
{
	ThumbnailPool.Reset();
}

TSharedPtr<SWidget> FOasisAssetDragDropOp::GetDefaultDecorator() const
{
	const int32 TotalCount = AssetData.Num() + AssetPaths.Num();

	TSharedPtr<SWidget> ThumbnailWidget;
	if (AssetThumbnail.IsValid())
	{
		ThumbnailWidget = AssetThumbnail->MakeThumbnailWidget();
	}
	else if (AssetPaths.Num() > 0)
	{
		ThumbnailWidget = 
			SNew(SOverlay)

			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("ContentBrowser.ListViewFolderIcon.Base"))
				.ColorAndOpacity(FLinearColor::Gray)
			]
		
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("ContentBrowser.ListViewFolderIcon.Mask"))
			];
	}
	else
	{
		ThumbnailWidget = 
			SNew(SImage)
			.Image(FAppStyle::GetDefaultBrush());
	}
	
	const FSlateBrush* SubTypeBrush = FAppStyle::GetDefaultBrush();
	FLinearColor SubTypeColor = FLinearColor::White;
	if (AssetThumbnail.IsValid() && AssetPaths.Num() > 0)
	{
		SubTypeBrush = FAppStyle::GetBrush("ContentBrowser.AssetTreeFolderClosed");
		SubTypeColor = FLinearColor::Gray;
	}
	else if (ActorFactory.IsValid() && AssetData.Num() > 0)
	{
#if ECB_LEGACY
		AActor* DefaultActor = ActorFactory->GetDefaultActor(AssetData[0]);
		SubTypeBrush = FClassIconFinder::FindIconForActor(DefaultActor);
#endif
	}

	return 
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ContentBrowser.AssetDragDropTooltipBackground"))
		.Content()
		[
			SNew(SHorizontalBox)

			// Left slot is for the thumbnail
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			[
				SNew(SBox) 
				.WidthOverride(ThumbnailSize) 
				.HeightOverride(ThumbnailSize)
				.Content()
				[
					SNew(SOverlay)

					+SOverlay::Slot()
					[
						ThumbnailWidget.ToSharedRef()
					]

					+SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					.Padding(FMargin(0, 4, 0, 0))
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("Menu.Background"))
						.Visibility(TotalCount > 1 ? EVisibility::Visible : EVisibility::Collapsed)
						.Content()
						[
							SNew(STextBlock)
							.Text(FText::AsNumber(TotalCount))
						]
					]

					+SOverlay::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Bottom)
					.Padding(FMargin(4, 4))
					[
						SNew(SImage)
						.Image(SubTypeBrush)
						.Visibility(SubTypeBrush != FAppStyle::GetDefaultBrush() ? EVisibility::Visible : EVisibility::Collapsed)
						.ColorAndOpacity(SubTypeColor)
					]
				]
			]

			// Right slot is for optional tooltip
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.MinDesiredWidth(80)
				.Content()
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(3.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SImage) 
						.Image(this, &FOasisAssetDragDropOp::GetIcon)
					]

					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0,0,3,0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock) 
						.Text(this, &FOasisAssetDragDropOp::GetDecoratorText)
					]
				]
			]
		];
}

FText FOasisAssetDragDropOp::GetDecoratorText() const
{
	if (CurrentHoverText.IsEmpty())
	{
		const int32 TotalCount = AssetData.Num() + AssetPaths.Num();
		if (TotalCount > 0)
		{
			const FText FirstItemText = AssetData.Num() > 0 ? FText::FromName(AssetData[0].AssetName) : FText::FromString(AssetPaths[0]);
			return (TotalCount == 1)
				? FirstItemText
				: FText::Format(NSLOCTEXT("ContentBrowser", "AssetDragDropOpDescriptionMulti", "'{0}' and {1} {1}|plural(one=other,other=others)"), FirstItemText, TotalCount - 1);
		}
	}
	
	return CurrentHoverText;
}

void FOasisAssetDragDropOp::Init()
{
	if (AssetData.Num() > 0 && ThumbnailSize > 0)
	{
#if ECB_LEGACY
		// Load all assets first so that there is no loading going on while attempting to drag
		// Can cause unsafe frame reentry 
		for (FOasisAssetData& Data : AssetData)
		{
			Data.GetAsset();
		}
#endif

		// Create a thumbnail pool to hold the single thumbnail rendered
		ThumbnailPool = MakeShared<FOasisAssetThumbnailPool>(1, /*InAreRealTileThumbnailsAllowed=*/false);

		// Create the thumbnail handle
		AssetThumbnail = MakeShared<FOasisAssetThumbnail>(AssetData[0], ThumbnailSize, ThumbnailSize, ThumbnailPool);

		// Request the texture then tick the pool once to render the thumbnail
		AssetThumbnail->GetViewportRenderTargetTexture();
		ThumbnailPool->Tick(0);
	}
}
