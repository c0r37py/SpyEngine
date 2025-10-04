// Innerloop Interactive Production :: c0r37py

#include "SOasisContentBrowser.h"
#include "OasisContentBrowser.h"
#include "OasisContentBrowserSingleton.h"
#include "SOasisAssetView.h"
#include "SOasisPathView.h"
#include "SOasisFilterList.h"
#include "SOasisDependencyViewer.h"
#include "AssetContextMenu.h"
#include "PathContextMenu.h"
#include "OasisContentBrowserCommands.h"
#include "OasisContentBrowserUtils.h"

#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Widgets/Input/SDirectoryPicker.h"

#include "Factories/Factory.h"
#include "Framework/Commands/UIAction.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UICommandList.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FeedbackContext.h"
#include "Misc/ScopedSlowTask.h"
#include "Widgets/SBoxPanel.h"
#include "Layout/WidgetPath.h"
#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SSplitter.h"
#include "Framework/Docking/TabManager.h"
#include "EditorStyleSet.h"
#include "EditorFontGlyphs.h"
#include "Settings/ContentBrowserSettings.h"
#include "Settings/EditorSettings.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Widgets/Navigation/SBreadcrumbTrail.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Input/SSpinBox.h"
#include "Framework/Commands/GenericCommands.h"
#include "Engine/Selection.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
#include "SAssetSearchBox.h"
#include "OasisFrontendFilters.h"
//#include "SCollectionView.h"
//#include "AddToProjectConfig.h"
//#include "GameProjectGenerationModule.h"

#include "Widgets/Input/SFilePathPicker.h"

#include "OasisDocumentation.h"

#define LOCTEXT_NAMESPACE "OasisContentBrowser"

/////////////////////////////////////////////////////////////
// SOasisContentBrowser implementation
//

const FString SOasisContentBrowser::SettingsIniSection = TEXT("SpyEngine");

TSharedRef<SWidget> SOasisContentBrowser::CreateChangeLogWidget()
{
	FString AboutLinkRoot(TEXT("Shared/About"));
	FDocumentationStyle DocumentationStyle = FOasisContentBrowserStyle::GetChangLogDocumentationStyle();
	const IDocumentationProvider* Provider = FDocumentationProvider::Get().GetProvider(FName(*DocumentationHostPluginName));

	if (Provider == nullptr)
	{
		return SNullWidget::NullWidget;
	}
	
	AboutPage = Provider->GetDocumentation()->GetPage(AboutLinkRoot, /*TSharedPtr<FParserConfiguration>()*/NULL, DocumentationStyle);

	if (!AboutPage.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	FExcerpt ChangeLogExcerpt;
	AboutPage->GetExcerpt(TEXT("ChangeLog"), ChangeLogExcerpt);
	AboutPage->GetExcerptContent(ChangeLogExcerpt);
	TSharedRef<SWidget> ChangeLogContentWidget = ChangeLogExcerpt.Content.IsValid() ? ChangeLogExcerpt.Content.ToSharedRef() : SNew(SSpacer);

	return SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.5f, 0.5f, 0.5f))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(6.f)
		.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([this] { return bShowChangelog ? EVisibility::Visible : EVisibility::Collapsed; })))
		[
			SNew(SVerticalBox)

			// Header
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 8).HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(FOasisContentBrowserStyle::Get(), "SpyEngine.ChangeLogHeaderText")
			.Text(LOCTEXT("SpyEngineChangeLog", "SpyEngine Change Log"))
		]

	// Change Log
	+ SVerticalBox::Slot().Padding(0, 2, 0, 2).FillHeight(1.0f).HAlign(HAlign_Center)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.3f, 0.3f, 0.3f))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(1.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot().Padding(5, 5)
		[
			ChangeLogContentWidget
		]
		]
		]

	// Close Button
	+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 4).HAlign(HAlign_Center)
		[
			SNew(SButton)
			//.ButtonStyle(FMeshToolStyle::GetButtonStyleByTheme())
		//.TextStyle(FMeshToolStyle::GetButtonTextStyleByTheme(FMeshToolStyle::EMeshToolTextSize::Normal))
		.Text(LOCTEXT("Close", "Close"))
		.HAlign(HAlign_Center)
		.OnClicked(FOnClicked::CreateLambda([this] { this->bShowChangelog = false; return FReply::Handled(); }))
		]
		];
}

SOasisContentBrowser::~SOasisContentBrowser()
{
	// Remove the listener for when view settings are changed
	UOasisContentBrowserSettings::OnSettingChanged().RemoveAll( this );

#if ECB_WIP_CACHEDB
	if (GetDefault<UOasisContentBrowserSettings>()->bCacheMode && GetDefault<UOasisContentBrowserSettings>()->bAutoSaveCacheOnExit)
	{
		FOasisContentBrowserSingleton::GetAssetRegistry().SaveCacheDB(/*bSilent =*/ false);
	}
#endif

// 	// Remove listeners for when collections/paths are renamed/deleted
// 	if (FCollectionManagerModule::IsModuleAvailable())
// 	{
// 		FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
// 
// 		CollectionManagerModule.Get().OnCollectionRenamed().RemoveAll(this);
// 		CollectionManagerModule.Get().OnCollectionDestroyed().RemoveAll(this);
// 	}
// 
// 	FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry"));
// 	if (AssetRegistryModule != nullptr)
// 	{
// 		AssetRegistryModule->Get().OnPathRemoved().RemoveAll(this);
// 	}
}


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SOasisContentBrowser::Construct(const FArguments& InArgs, const FName& InInstanceName)
{
	const UOasisContentBrowserSettings* OasisContentBrowserSettings = GetDefault<UOasisContentBrowserSettings>();

	if (InArgs._ContainingTab.IsValid())
	{
		// For content browsers that are placed in tabs, save settings when the tab is closing.
		ContainingTab = InArgs._ContainingTab;
		InArgs._ContainingTab->SetOnPersistVisualState(SDockTab::FOnPersistVisualState::CreateSP(this, &SOasisContentBrowser::OnContainingTabSavingVisualState));
		InArgs._ContainingTab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateSP(this, &SOasisContentBrowser::OnContainingTabClosed));
		InArgs._ContainingTab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateSP(this, &SOasisContentBrowser::OnContainingTabActivated));
	}

	bIsLocked = InArgs._InitiallyLocked;

	bAlwaysShowCollections = false;
	bCanSetAsPrimaryBrowser = true;

#if ECB_WIP_HISTORY
	HistoryManager.SetOnApplyHistoryData(FOnApplyHistoryData::CreateSP(this, &SOasisContentBrowser::OnApplyHistoryData));
	HistoryManager.SetOnUpdateHistoryData(FOnUpdateHistoryData::CreateSP(this, &SOasisContentBrowser::OnUpdateHistoryData));
#endif

	PathContextMenu = MakeShareable(new FPathContextMenu(AsShared()));

#if ECB_LEGACY
	PathContextMenu->SetOnImportAssetRequested(FNewAssetOrClassContextMenu::FOnImportAssetRequested::CreateSP(this, &SOasisContentBrowser::ImportAsset));
	PathContextMenu->SetOnRenameFolderRequested(FPathContextMenu::FOnRenameFolderRequested::CreateSP(this, &SOasisContentBrowser::OnRenameFolderRequested));
	PathContextMenu->SetOnFolderDeleted(FPathContextMenu::FOnFolderDeleted::CreateSP(this, &SOasisContentBrowser::OnOpenedFolderDeleted));
	PathContextMenu->SetOnFolderFavoriteToggled(FPathContextMenu::FOnFolderFavoriteToggled::CreateSP(this, &SOasisContentBrowser::ToggleFolderFavorite));
#endif

	FrontendFilters = MakeShareable(new FExtAssetFilterCollectionType());

#if ECB_FEA_SEARCH
	TextFilter = MakeShareable(new FFrontendFilter_Text());
#endif
	static const FName DefaultForegroundName("DefaultForeground");

	BindCommands();

	TSharedRef<SOverlay> MainOverlay = SNew(SOverlay);

	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(1))
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(this, &SOasisContentBrowser::GetContentBrowserBorderBackgroundColor)
		[
			MainOverlay
		]
	];


	const float WrapButtonGroupsSize = 60.f;
	const float ButtonBoxPadding = 3.f;
	const float ButtonPadding = 4.f;
	const float ButtonAutoHidePadding = 130.f;

	MainOverlay->AddSlot()
	[
	SNew(SBorder)
	.Padding(FMargin(0))
	.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
	//.BorderBackgroundColor(this, & SOasisContentBrowser::GetContentBrowserBorderBackgroundColor)
	.BorderBackgroundColor(FOasisContentBrowserStyle::CustomContentBrowserBorderBackgroundColor)
	[

		SNew(SVerticalBox)

		// Top Bar (Buttons and History) >>
#if ECB_FOLD
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 0)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SBorder)
				.Padding(FMargin(3))
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(this, &SOasisContentBrowser::GetToolbarBackgroundColor)
				[
					SNew(SHorizontalBox)

					// Button Group (Add/Remote Root Folder)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
				
						SNew(SHorizontalBox)

						// Add Root Folder Button >>
#if ECB_FOLD
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.Padding(ButtonBoxPadding, 0)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "FlatButton")
							.ContentPadding(FMargin(ButtonPadding, 1))
							.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 300.f)
							.ToolTipText(LOCTEXT("AddContentFolderTooltip", "Add a root content folder to path view"))
							.OnClicked(this, &SOasisContentBrowser::HandleAddContentFolderClicked)
							[
								SNew(SHorizontalBox)

								// Add Icon
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.AutoWidth()
								[
									SNew(STextBlock)
									.TextStyle(FOasisContentBrowserStyle::Get(), "SpyEngine.TopBar.Font")
									.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
									.Text(FEditorFontGlyphs::Plus_Circle)
								]

								// Add Text
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
								[
									SNew(STextBlock)
									.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 349.f + ButtonAutoHidePadding)
									.TextStyle(FOasisContentBrowserStyle::Get(), "SpyEngine.TopBar.Font")
									.Text(LOCTEXT("Add", "Add"))
								]
							]
						]
#endif					// Add Root Folder Button <<

						// Remove Root Folder Button >>
#if ECB_FOLD
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.Padding(ButtonBoxPadding, 0)
						[
							SNew(SButton)
							.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 230.f + ButtonAutoHidePadding)
							.ButtonStyle(FAppStyle::Get(), "FlatButton")
							.ContentPadding(FMargin(ButtonPadding, 1))
							.ToolTipText(LOCTEXT("RemoveContentFolderTooltip", "Remove selected root content folder from path view"))
							.OnClicked(this, &SOasisContentBrowser::HandleRemoveContentFoldersClicked)
							.IsEnabled(this, &SOasisContentBrowser::CanRemoveContentFolders)
							[
								SNew(SHorizontalBox)

								// Remove Icon
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.AutoWidth()
								[
									SNew(STextBlock)
									.TextStyle(FOasisContentBrowserStyle::Get(), "SpyEngine.TopBar.Font")
									.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
									.Text(FEditorFontGlyphs::Minus_Circle)
								]

								// Remove Text
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(4, 0, 0, 0)
								[
									SNew(STextBlock)
									.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 405.f + ButtonAutoHidePadding)
									.TextStyle(FOasisContentBrowserStyle::Get(), "SpyEngine.TopBar.Font")
									.Text(LOCTEXT("Remove", "Remove"))
								]
							]
						]
#endif					// Remove Root Folder Button <<

					] // SWrapBox::Slot()

					// Button Group Save CacheDB >>
#if ECB_FOLD
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SHorizontalBox)
#if ECB_WIP_CACHEDB
						// Save and Switch
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.Padding(ButtonBoxPadding, 0)
						[
							SNew(SButton)
							.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 247.f)
							.ButtonStyle(FAppStyle::Get(), "FlatButton")
							.ToolTipText(LOCTEXT("SaveCacheUAssetTooltip", "Save Cache and Switch to Cache Mode"))
							.IsEnabled(true)
							.OnClicked(this, &SOasisContentBrowser::HandleSaveAndSwitchToCacheModeClicked)
							.ContentPadding(FMargin(ButtonPadding, 1))
							[
								SNew(SHorizontalBox)

								// Icon
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.AutoWidth()
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Font(FAppStyle::Get().GetFontStyle("FontAwesome.10"))
									.Text(FEditorFontGlyphs::Database)
								]

								// Text
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(4, 0, 0, 0)
								[
									SNew(STextBlock)
									.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 276.f + ButtonAutoHidePadding)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									//.Text(GetSaveAndSwitchToCacheModeText())
									.Text_Lambda([this, OasisContentBrowserSettings]()
									{
										return GetSaveAndSwitchToCacheModeText();
									})
								]
							]
						]
#endif					

					]
#endif
					// Button Group Load/Save CacheDB <<

					// Button Group Validate Asset Button >>
#if ECB_FOLD
					+ SHorizontalBox::Slot()
						.AutoWidth()
					[
						SNew(SHorizontalBox)
#if ECB_FEA_VALIDATE
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.Padding(ButtonBoxPadding, 0)
						[
							SNew(SButton)
							.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 200.f)
							.ButtonStyle(FAppStyle::Get(), "FlatButton")
							.ToolTipText(LOCTEXT("ValidateUAssetTooltip", "Validate selected uasset files before import"))
							.IsEnabled(this, &SOasisContentBrowser::IsValidateEnabled)
							.OnClicked(this, &SOasisContentBrowser::HandleValidateclicked)
							.ContentPadding(FMargin(ButtonPadding, 1))
							[
								SNew(SHorizontalBox)

								// Validate Icon
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.AutoWidth()
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
									.Text(FEditorFontGlyphs::Check)
								]

								// Validate Text
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(4, 0, 0, 0)
								[
									SNew(STextBlock)
									.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 340.f + ButtonAutoHidePadding)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Text(LOCTEXT("Validate", "Validate"))
								]
							]
						]
#endif
					]
#endif
					// Button Group Validate Asset Button <<

					// Button Group Export Button >>
#if ECB_WIP
					+ SHorizontalBox::Slot()
						.AutoWidth()
					[
						SNew(SHorizontalBox)
						// Export >>
#if ECB_WIP_EXPORT
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "FlatButton")
							.ToolTipText(LOCTEXT("ExportTooltip", "Export uasset with dependencies"))
							.IsEnabled(this, &SOasisContentBrowser::IsExportAssetEnabled)
							.OnClicked(this, &SOasisContentBrowser::HandleExportAssetClicked)
							.ContentPadding(FMargin(1, 2))
							.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("OasisContentBrowserExport")))
							[
								SNew(SHorizontalBox)

								// Export Icon
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.AutoWidth()
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
									.Text(FEditorFontGlyphs::Archive)
								]

								// Export Text
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(4, 0, 0, 0)
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Text(LOCTEXT("Export", "Export"))
								]
							]
						]
#endif					// Export <<

						// Zip >>
#if ECB_WIP_EXPORT_ZIP
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "FlatButton")
							.ToolTipText(LOCTEXT("ZipExportTooltip", "Zip uasset with dependencies"))
							.IsEnabled(this, &SOasisContentBrowser::IsExportAssetEnabled)
							.OnClicked(this, &SOasisContentBrowser::HandleZipExportClicked)
							.ContentPadding(FMargin(1, 2))
							.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("OasisContentBrowserZipExport")))
							[
								SNew(SHorizontalBox)

								// Zip Icon
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.AutoWidth()
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
									.Text(FEditorFontGlyphs::Archive)
								]

								// Zip Text
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(4, 0, 0, 0)
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Text(LOCTEXT("ZipExport", "Zip"))
								]
							]
						]
#endif					// Zip <<

						// Import to Sandbox Button >>
#if ECB_WIP_IMPORT_SANDBOX
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "FlatButton")
							.IsEnabled(this, &SOasisContentBrowser::IsImportEnabled)
							.OnClicked(this, &SOasisContentBrowser::HandleImportToSandboxClicked)
							.ContentPadding(FMargin(1, 2))
							.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("OasisContentBrowserImportAsset")))
							[
								SNew(SHorizontalBox)

								// Import Icon
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.AutoWidth()
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
									.Text(FEditorFontGlyphs::Download)
								]

								// Import Text
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(4, 0, 0, 0)
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Text(LOCTEXT("Sandbox", "Sandbox"))
								]
							]
						]
#endif					// Import to Sandbox Button <<
					]
#endif
					// Button Group Export Button <<

					// Button Group Import Button >>
#if ECB_FOLD
					+ SHorizontalBox::Slot()
						.AutoWidth()
					[
						SNew(SHorizontalBox)
						// Flat Import Button >>
#if ECB_FEA_IMPORT_FLATMODE
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.Padding(ButtonBoxPadding, 0)
						[
							SNew(SButton)
							.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 142.f)
							.ButtonStyle(FAppStyle::Get(), "FlatButton")
							.ToolTipText(LOCTEXT("FlatImportTooltip", "Collect dependencies and import all into one selected folder"))
							.IsEnabled(this, &SOasisContentBrowser::IsImportEnabled)
							.OnClicked(this, &SOasisContentBrowser::HandleFlatImportClicked)
							.ContentPadding(FMargin(ButtonPadding, 1))
							//.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("OasisContentBrowserImportAsset")))
							[
								SNew(SHorizontalBox)

								// Import Icon
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.AutoWidth()
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
									.Text(FEditorFontGlyphs::Long_Arrow_Down)
								]

								// Import Text
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(4, 0, 0, 0)
								[
									SNew(STextBlock)
									.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 468.f + ButtonAutoHidePadding)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Text(LOCTEXT("FlatImport", "Flat Import"))
								]
							]
						]
#endif					// Flat Import Button  <<
					]

					+ SHorizontalBox::Slot()
						.AutoWidth()
					[
						SNew(SHorizontalBox)
						// Import Asset Button >>
#if ECB_FOLD
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.Padding(ButtonBoxPadding, 0)
						[
							SNew(SButton)
							.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 100.f)
							.ButtonStyle(FAppStyle::Get(), "FlatButton")
							.ToolTipText(this, &SOasisContentBrowser::GetImportTooltipText)
							.IsEnabled(this, &SOasisContentBrowser::IsImportEnabled)
							.OnClicked(this, &SOasisContentBrowser::HandleImportClicked)
							.ContentPadding(FMargin(ButtonPadding, 1))
							//.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("OasisContentBrowserImportAsset")))
							[
								SNew(SHorizontalBox)

								// Import Icon
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.AutoWidth()
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
									.Text(FEditorFontGlyphs::Download)
								]

								// Import Text
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(4, 0, 0, 0)
								[
									SNew(STextBlock)
									.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 550.f + ButtonAutoHidePadding)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Text(LOCTEXT("Import", "Import"))
								]
							]
						]
#endif					// Import Asset Button <<
					]
#endif
					// Button Group Import Button <<

			// History - BreadcrumbTrail - Import - Lock >>
#if ECB_TODO
					+ SWrapBox::Slot()
					.FillEmptySpace(true)
					[
				
						SNew(SHorizontalBox)

						// History Back Button >>
#if ECB_WIP_HISTORY
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.FillHeight(1.0f)
							[
								SNew(SButton)
								.VAlign(EVerticalAlignment::VAlign_Center)
								.ButtonStyle(FAppStyle::Get(), "FlatButton")
								.ForegroundColor(FAppStyle::GetSlateColor(DefaultForegroundName))
								.ToolTipText(this, &SOasisContentBrowser::GetHistoryBackTooltip)
								.ContentPadding(FMargin(1, 0))
								.OnClicked(this, &SOasisContentBrowser::BackClicked)
								.IsEnabled(this, &SOasisContentBrowser::IsBackEnabled)
								.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ContentBrowserHistoryBack")))
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
									.Text(FText::FromString(FString(TEXT("\xf060"))) /*fa-arrow-left*/)
								]
							]
						]
#endif				// History Back Button <<

					// History Forward Button >>
#if ECB_WIP_HISTORY
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.FillHeight(1.0f)
							[
								SNew(SButton)
								.VAlign(EVerticalAlignment::VAlign_Center)
								.ButtonStyle(FAppStyle::Get(), "FlatButton")
								.ForegroundColor(FAppStyle::GetSlateColor(DefaultForegroundName))
								.ToolTipText(this, &SOasisContentBrowser::GetHistoryForwardTooltip)
								.ContentPadding(FMargin(1, 0))
								.OnClicked(this, &SOasisContentBrowser::ForwardClicked)
								.IsEnabled(this, &SOasisContentBrowser::IsForwardEnabled)
								.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ContentBrowserHistoryForward")))
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
									.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
									.Text(FText::FromString(FString(TEXT("\xf061"))) /*fa-arrow-right*/)
								]
							]
						]
#endif					// History Forward Button <<

						// Separator >>
#if ECB_FOLD
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(3, 0)
						[
							SNew(SSeparator)
							.Orientation(Orient_Vertical)
						]
#endif				// Separator <<

					// Path picker >>
#if ECB_WIP_PATHPICKER
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Fill)
						[
							SAssignNew(PathPickerButton, SComboButton)
							.ButtonStyle(FAppStyle::Get(), "FlatButton")
							.ForegroundColor(FLinearColor::White)
							.ToolTipText(LOCTEXT("PathPickerTooltip", "Choose a path"))
							.OnGetMenuContent(this, &SOasisContentBrowser::GetPathPickerContent)
							.HasDownArrow(false)
							.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ContentBrowserPathPicker")))
							.ContentPadding(FMargin(3, 3))
							.ButtonContent()
							[
								SNew(STextBlock)
								.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
								.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
								.Text(FText::FromString(FString(TEXT("\xf07c"))) /*fa-folder-open*/)
							]
						]
#endif					// Patch picker <<	

						// BreadcrumbTrail >>
#if ECB_WIP_BREADCRUMB
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Left)
					.FillWidth(1.0f)
					.Padding(FMargin(0))
					[
						SAssignNew(PathBreadcrumbTrail, SBreadcrumbTrail<FString>)
						.ButtonContentPadding(FMargin(2, 2))
						.ButtonStyle(FAppStyle::Get(), "FlatButton")
						.DelimiterImage(FAppStyle::GetBrush("ContentBrowser.PathDelimiter"))
						.TextStyle(FAppStyle::Get(), "ContentBrowser.PathText")
						.ShowLeadingDelimiter(false)
						.InvertTextColorOnHover(false)
						.OnCrumbClicked(this, &SOasisContentBrowser::OnPathClicked)
						.GetCrumbMenuContent(this, &SOasisContentBrowser::OnGetCrumbDelimiterContent)
						.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ContentBrowserPath")))
					]
#endif				// BreadcrumbTrail <<

					// Import Asset Button >>
#if ECB_FOLD
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		.Padding(6, 0)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "FlatButton")
		.ToolTipText(this, &SOasisContentBrowser::GetImportTooltipText)
		.IsEnabled(this, &SOasisContentBrowser::IsImportEnabled)
		.OnClicked(this, &SOasisContentBrowser::HandleImportClicked)
		.ContentPadding(FMargin(6, 2))
		.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ContentBrowserImportAsset")))
		[
			SNew(SHorizontalBox)

			// Import Icon
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(STextBlock)
			.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
		.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
		.Text(FEditorFontGlyphs::Download)
		]

	// Import Text
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4, 0, 0, 0)
		[
			SNew(STextBlock)
			.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
		.Text(LOCTEXT("Import", "Import"))
		]
		]
		]
#endif				// Import Asset Button <<

					// Lock button >>
#if ECB_WIP_LOCK
	+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			.Visibility(EVisibility::SelfHitTestInvisible)

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SButton)
			.VAlign(EVerticalAlignment::VAlign_Center)
		.ButtonStyle(FAppStyle::Get(), "FlatButton")
		.ToolTipText(LOCTEXT("LockToggleTooltip", "Toggle lock. If locked, this browser will ignore Find in Content Browser requests."))
		.ContentPadding(FMargin(1, 0))
		.OnClicked(this, &SOasisContentBrowser::ToggleLockClicked)
		.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ContentBrowserLock")))
		[
			SNew(SImage)
			.Image(this, &SOasisContentBrowser::GetToggleLockImage)
		]
		]
		]
#endif				// Lock button <<
		]
		]
#endif		// History - BreadcrumbTrail - Import - Lock <<
				]
			]

			// Import Asset Options >>
#if ECB_FEA_IMPORT_OPTIONS
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SAssignNew(ImportOptionsComboButton, SComboButton)
				.ContentPadding(1)
				.ForegroundColor(this, &SOasisContentBrowser::GetImportOptionsButtonForegroundColor)
				.ButtonStyle(FAppStyle::Get(), "ToggleButton")
				.OnGetMenuContent(this, &SOasisContentBrowser::GetImportOptionsButtonContent)
				.ButtonContent()
				[
					SNew(SHorizontalBox)
#if ECB_FOLD
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						//SNew(SImage).Image(FAppStyle::GetBrush("LevelEditor.GameSettings.Small"))
						SNew(STextBlock)
						.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
						.Font(FAppStyle::Get().GetFontStyle("FontAwesome.11"))
						.Text(FEditorFontGlyphs::Info)
					]
#endif

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(3, 0, 0, 0)
					.VAlign(VAlign_Center)
					[
						//SNew(STextBlock).Text(LOCTEXT("ImportButton", "Import Options"))
						SNew(STextBlock)
						.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
						.Text(LOCTEXT(" ", " "))
					]
				]
			]
#endif		// Import Asset Options <<
		]
#endif	// Top Bar (Buttons and History) <<

		// Paths and Assets View >>
#if ECB_FOLD
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(0,2,0,0)
		[
			// The tree/assets splitter
			SAssignNew(PathAssetSplitterPtr, SSplitter)

			// Sources View >>
#if ECB_FOLD
			+ SSplitter::Slot()
			.Value(0.25f)
			[
				SNew(SBorder)
				.Padding(FMargin(0))
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(this, &SOasisContentBrowser::GetSourceViewBackgroundColor)
				.Visibility(this, &SOasisContentBrowser::GetSourcesViewVisibility)
				[

				SAssignNew(PathFavoriteSplitterPtr, SSplitter)
				.Orientation(EOrientation::Orient_Vertical)
				.MinimumSlotHeight(70.0f)
				.Visibility( this, &SOasisContentBrowser::GetSourcesViewVisibility )

				// Favorite View >>
#if ECB_FOLD
				+ SSplitter::Slot()
				.Value(.2f)
				[
					SNew(SBorder)
					.Visibility(this, &SOasisContentBrowser::GetFavoriteFolderVisibility)
					.Padding(FMargin(3))
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					[
#if ECB_WIP_FAVORITE
						SAssignNew(FavoritePathViewPtr, SFavoritePathView)
						.OnPathSelected(this, &SOasisContentBrowser::FavoritePathSelected)
						.OnGetFolderContextMenu(this, &SOasisContentBrowser::GetFolderContextMenu, /*bPathView*/true)
						.OnGetPathContextMenuExtender(this, &SOasisContentBrowser::GetPathContextMenuExtender)
						.FocusSearchBoxWhenOpened(false)
						.ShowTreeTitle(true)
						.ShowSeparator(false)
						.AllowClassesFolder(true)
						.SearchContent()
						[
							SNew(SVerticalBox)
							.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ContentBrowserSourcesToggle1")))
							+ SVerticalBox::Slot()
							.FillHeight(1.0f)
							.Padding(0, 0, 2, 0)
							[
								SNew(SButton)
								.VAlign(EVerticalAlignment::VAlign_Center)
								.ButtonStyle(FAppStyle::Get(), "ToggleButton")
								.ToolTipText(LOCTEXT("SourcesTreeToggleTooltip", "Show or hide the sources panel"))
								.ContentPadding(FMargin(1, 0))
								.ForegroundColor(FAppStyle::GetSlateColor(DefaultForegroundName))
								.OnClicked(this, &SOasisContentBrowser::SourcesViewExpandClicked)
								[
									SNew(SImage)
									.Image(this, &SOasisContentBrowser::GetSourcesToggleImage)
								]
							]
						]
#endif
						SNew(STextBlock)
						.Text(LOCTEXT("SFavoritePathView","SFavoritePathView"))
					] // end of Favorite View Border
				] // end of Favorite View Splitter
#endif			// Favorite View <<

				+ SSplitter::Slot()
				.Value(0.8f)
				[
					SAssignNew(PathCollectionSplitterPtr, SSplitter)
					.Orientation( Orient_Vertical )

					// Path View
#if ECB_FOLD
					+ SSplitter::Slot()
					.Value(0.9f)
					[
						SNew(SBorder)
						.Padding(FMargin(3))
						.BorderImage(FAppStyle::GetBrush("NoBrush"))
						//.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						//.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						//.BorderBackgroundColor(FLinearColor(1.0f, 0.f, 0.f, 1.f))
						[

							SAssignNew( PathViewPtr, SOasisPathView )
							.OnPathSelected( this, &SOasisContentBrowser::PathSelected )
							.OnGetFolderContextMenu( this, &SOasisContentBrowser::GetFolderContextMenu, /*bPathView*/true )
							.OnGetPathContextMenuExtender( this, &SOasisContentBrowser::GetPathContextMenuExtender )
							//.SearchBarVisibility(this, &SOasisContentBrowser::GetAlternateSearchBarVisibility)
							.FocusSearchBoxWhenOpened(false)							
							.ShowTreeTitle(false)
							.ShowSeparator(false)
							.AllowClassesFolder(true)
							.AllowContextMenu(true)
							.SearchContent()
							[
								SNew(SVerticalBox)
								.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("OasisContentBrowserSourcesToggle1")))
								//.Visibility(this, &SOasisContentBrowser::GetAlternateSearchBarVisibility)

								+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0, 0, 2, 0)
								[
									SNew(SButton)
									.VAlign(EVerticalAlignment::VAlign_Center)
									.ContentPadding(FMargin(1, 0))
									.ForegroundColor(FAppStyle::GetSlateColor(DefaultForegroundName))
									.ButtonStyle(FAppStyle::Get(), "ToggleButton")
									.ToolTipText(LOCTEXT("SourcesTreeToggleTooltip", "Show or hide the sources panel"))
									.OnClicked(this, &SOasisContentBrowser::SourcesViewExpandClicked)
									[
										SNew(SImage)
										.Image(this, &SOasisContentBrowser::GetSourcesToggleImage)
									]
								]
							] // end of SOasisPathView
						] // end of PathView Border
					]
#endif

					// Collection View
#if ECB_FOLD
					+ SSplitter::Slot()
					.Value(0.9f)
					[
						SNew(SBorder)
						.Visibility( this, &SOasisContentBrowser::GetCollectionViewVisibility )
						.Padding(FMargin(3))
						.BorderImage( FAppStyle::GetBrush("ToolPanel.GroupBorder") )
						[
							SNew(STextBlock).Text(LOCTEXT("SCollectionView", "SCollectionView"))
#if ECB_WIP_COLLECTION
							SAssignNew(CollectionViewPtr, SCollectionView)
							.OnCollectionSelected(this, &SOasisContentBrowser::CollectionSelected)
							.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ContentBrowserCollections")))
							.AllowCollectionDrag(true)
							.AllowQuickAssetManagement(true)
#endif
						] // end of CollectionView Border
					] // end of CollectionView Splitter
#endif
				]
				] // endo fo source view sborder
			] // end of Source View
#endif		// Sources View <<

			// Asset and Dependency View >>
#if ECB_FOLD
			+ SSplitter::Slot()
			.Value(0.75f)
			[
				// The assets/dependency splitter
				SAssignNew(AssetReferenceSplitterPtr, SSplitter)
				.Orientation(Orient_Horizontal)

				// Asset View >>
#if ECB_FOLD
				+ SSplitter::Slot()
				.Value(0.5f)
				[
					SNew(SBorder)
					//.Padding(FMargin(3))
					//.BorderImage( FAppStyle::GetBrush("ToolPanel.GroupBorder") )
					.Padding(FMargin(0))
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(this, &SOasisContentBrowser::GetAssetViewBackgroundColor)
					[				
						SNew(SVerticalBox)

					// Expand Tree - Filter - Search - Save Search - Expand RefViewer >>
#if ECB_FOLD
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 2)
						[
							SNew(SHorizontalBox)

							// Expand Tree >>
	#if ECB_FOLD
							+ SHorizontalBox::Slot().AutoWidth().Padding( 0, 0, 4, 0 )
							[
								SNew( SVerticalBox )
								.Visibility(EVisibility::SelfHitTestInvisible)
								.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("OasisContentBrowserSourcesToggle2")))

								+ SVerticalBox::Slot().FillHeight( 1.0f )
								[
									SNew( SButton ).VAlign( EVerticalAlignment::VAlign_Center ).ButtonStyle( FAppStyle::Get(), "ToggleButton" ).ContentPadding(FMargin(1, 0))
									.ToolTipText( LOCTEXT( "SourcesTreeToggleTooltip", "Show or hide the sources panel" ) )								
									.ForegroundColor( FAppStyle::GetSlateColor(DefaultForegroundName) )
									.OnClicked( this, &SOasisContentBrowser::SourcesViewExpandClicked )
									.Visibility( this, &SOasisContentBrowser::GetPathExpanderVisibility )
									[
										SNew( SImage ).Image( this, &SOasisContentBrowser::GetSourcesToggleImage )
									]
								]
							]
	#endif					// Expand Tree <<

							// Filter >>
	#if ECB_FOLD
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew( SComboButton )
								.ComboButtonStyle( FAppStyle::Get(), "GenericFilters.ComboButtonStyle" )
								.ForegroundColor(FLinearColor::White)
								.ContentPadding(0)
								.ToolTipText( LOCTEXT( "AddFilterToolTip", "Add an asset filter." ) )
								.OnGetMenuContent( this, &SOasisContentBrowser::MakeAddFilterMenu )
								.HasDownArrow( true )
								.ContentPadding( FMargin( 1, 0 ) )
								.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("OasisContentBrowserFiltersCombo")))
	#if ECB_FEA_FILTER
								.Visibility(EVisibility::Visible)
	#else
								.Visibility(EVisibility::Collapsed)
	#endif
								.ButtonContent()
								[
									SNew(SHorizontalBox)

									+ SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(STextBlock)
										.TextStyle(FAppStyle::Get(), "GenericFilters.TextStyle")
										.Font(FAppStyle::Get().GetFontStyle("FontAwesome.9"))
										.Text(FText::FromString(FString(TEXT("\xf0b0"))) /*fa-filter*/)
									]

									+ SHorizontalBox::Slot()
									.AutoWidth()
									.Padding(2,0,0,0)
									[
										SNew(STextBlock)
										.TextStyle(FAppStyle::Get(), "GenericFilters.TextStyle")
										.Text(LOCTEXT("Filters", "Filters"))
									]
								]
							]
	#endif					// Filter <<

							// Search >>
	#if ECB_FEA_SEARCH
							+SHorizontalBox::Slot()
							.Padding(4, 1, 0, 0)
							.FillWidth(1.0f)
							[
								SAssignNew(SearchBoxPtr, SAssetSearchBox)
								.HintText( this, &SOasisContentBrowser::GetSearchAssetsHintText )
								.OnTextChanged( this, &SOasisContentBrowser::OnSearchBoxChanged )
								.OnTextCommitted( this, &SOasisContentBrowser::OnSearchBoxCommitted )
								//.PossibleSuggestions( this, &SOasisContentBrowser::GetAssetSearchSuggestions )
								.OnAssetSearchBoxSuggestionFilter(this, &SOasisContentBrowser::OnAssetSearchSuggestionFilter)
								.OnAssetSearchBoxSuggestionChosen(this, &SOasisContentBrowser::OnAssetSearchSuggestionChosen)
								.DelayChangeNotificationsWhileTyping( true )
								.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("OasisContentBrowserSearchAssets")))
							]
	#endif					// Search <<

							// Save Search >>
	#if ECB_TODO
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(2.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "FlatButton")
								.ToolTipText(LOCTEXT("SaveSearchButtonTooltip", "Save the current search as a dynamic collection."))
								.IsEnabled(this, &SOasisContentBrowser::IsSaveSearchButtonEnabled)
								.OnClicked(this, &SOasisContentBrowser::OnSaveSearchButtonClicked)
								.ContentPadding( FMargin(1, 1) )
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "GenericFilters.TextStyle")
									.Font(FAppStyle::Get().GetFontStyle("FontAwesome.10"))
									.Text(FEditorFontGlyphs::Floppy_O)
								]
							]
	#endif					// Save Search <<

							// Expand RefViewer >>
	#if ECB_FEA_REF_VIEWER
							+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 4, 0)
							[
								SNew(SVerticalBox)
								.Visibility(EVisibility::SelfHitTestInvisible)
								.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("OasisContentBrowserRefViewerToggle")))

								+ SVerticalBox::Slot().FillHeight(1.0f)
								[
									SNew(SButton).VAlign(EVerticalAlignment::VAlign_Center).ButtonStyle(FAppStyle::Get(), "ToggleButton").ContentPadding(FMargin(1, 0))
									.ToolTipText(LOCTEXT("RefViewerToggleTooltip", "Show or hide the dependencies viewer"))
									.ForegroundColor(FAppStyle::GetSlateColor(DefaultForegroundName))
									.OnClicked(this, &SOasisContentBrowser::ReferenceViewerExpandClicked)
									//.Visibility(this, &SOasisContentBrowser::GetPathExpanderVisibility)
									[
										SNew(SImage).Image(this, &SOasisContentBrowser::GetDependencyViewerToggleImage)
									]
								]
							]
	#endif					// Expand RefViewer <<

						]
#endif				// Expand Tree - Filter - Search - Save Search - Expand RefViewer <<

					// FilterList >>
#if ECB_FOLD
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(FilterListPtr, SOasisFilterList)
							.OnFilterChanged(this, &SOasisContentBrowser::OnFilterChanged)
							.OnGetContextMenu(this, &SOasisContentBrowser::GetFilterContextMenu)
	#if ECB_FEA_FILTER
							.Visibility(EVisibility::Visible)
	#else
							.Visibility(EVisibility::Collapsed)
	#endif
							.FrontendFilters(FrontendFilters)
							.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("OasisContentBrowserFilters")))
						]
#endif				// FilterList <<

					// Asset Tile View >>
#if ECB_FOLD
						+ SVerticalBox::Slot()
						.FillHeight( 1.0f )
						.Padding( 0 )
						[
							SAssignNew(AssetViewPtr, SOasisAssetView)
							.ThumbnailLabel(EThumbnailLabel::ClassName)
							.ThumbnailScale(0.18f)
							.InitialViewType(EAssetViewType::Tile)
							.ShowBottomToolbar(true)
							.OnPathSelected(this, &SOasisContentBrowser::FolderEntered)
							.OnAssetSelected(this, &SOasisContentBrowser::OnAssetSelectionChanged)
	#if ECB_LEGACY
							.OnAssetsActivated(this, &SOasisContentBrowser::OnAssetsActivated)
	#endif

							.OnGetAssetContextMenu(this, &SOasisContentBrowser::OnGetAssetContextMenu)
	#if ECB_TODO
							.OnGetFolderContextMenu(this, &SOasisContentBrowser::GetFolderContextMenu, /*bPathView*/false)
							.OnGetPathContextMenuExtender(this, &SOasisContentBrowser::GetPathContextMenuExtender)
	#endif

	#if ECB_WIP_SYNC_ASSET
							.OnFindInAssetTreeRequested(this, &SOasisContentBrowser::OnFindInAssetTreeRequested)
	#endif
							.FrontendFilters(FrontendFilters)
							.HighlightedText(this, &SOasisContentBrowser::GetHighlightedText)
							.AllowThumbnailHintLabel(false)
							.CanShowFolders(true)
							.CanShowCollections(true)
							.CanShowFavorites(true)
							.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("OasisContentBrowserAssets")))

							.OnRequestShowChangeLog(this, &SOasisContentBrowser::HandleRequestShowChangeLog)
	#if ECB_FEA_SEARCH
							.OnSearchOptionsChanged(this, &SOasisContentBrowser::HandleAssetViewSearchOptionsChanged)
	#endif
				

						
						]
#endif				// Asset Tile View <<
					]
				]
#endif			// Asset View <<

				// Reference Viewer >>
#if ECB_FEA_REF_VIEWER
				+ SSplitter::Slot()
				.Value(0.5f)
				[
					SAssignNew(ReferenceViewerPtr, SOasisDependencyViewer)
					.Visibility(this, &SOasisContentBrowser::GetReferencesViewerVisibility)
					.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ExtDependencyViewer")))
				]
#endif		// Reference Viewer <<

			]
#endif		// Asset and Dependency View <<
			
		]
#endif	// Paths and Assets View <<

		// Bottom Bar (Cache Status) >>
#if ECB_WIP_CACHEDB
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 0)
		[
			SNew(SBorder)
			.Padding(FMargin(1))
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(.03f, 0.03f, 0.03f, 1.f))
			.Visibility(this, &SOasisContentBrowser::GetCacheStatusBarVisibility)
			[
				SNew(SHorizontalBox)

				// Asset count
				+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(8, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0)
					[
						SNew(STextBlock).Text(this, &SOasisContentBrowser::GetCacheStatusText)
					]
#if ECB_WIP_CACHEDB_SWITCH
					// Switch DB
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0)
					[
						SNew(SFilePathPicker)
						.BrowseButtonImage(FAppStyle::GetBrush("PropertyWindow.Button_Ellipsis"))
						.BrowseButtonStyle(FAppStyle::Get(), "HoverHintOnly")
						.BrowseButtonToolTip(LOCTEXT("CacheDBPathBrowseButtonToolTip", "Choose a file from this computer"))
						.BrowseDirectory_Lambda([this, OasisContentBrowserSettings]() -> FString
						{
							return FPaths::GetPath(OasisContentBrowserSettings->CacheFilePath.FilePath);
						})
						.FilePath_Lambda([this, OasisContentBrowserSettings]() -> FString
						{
							return OasisContentBrowserSettings->CacheFilePath.FilePath;
						})
						.FileTypeFilter_Lambda([]() -> FString
						{
							return TEXT("All files (*.*)|*.*|Cache Database files (*.cachedb)|*.cachedb");
						})
						.OnPathPicked(this, &SOasisContentBrowser::HandleSwitchAndLoadCacheDBClicked)
						.ToolTipText(LOCTEXT("CacheDBPathToolTip", "The file use as cache dababase to store parsed uasset meta data."))
					]
#if 0 // Move to Import Options Menu
					// Purge DB
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(2, 0)
					[
						SNew(SButton)
						.Visibility(this, &SOasisContentBrowser::GetVisibilityByLastFrameWidth, 132.f)
						.ButtonStyle(FAppStyle::Get(), "FlatButton")
						.ToolTipText(LOCTEXT("PurgeCacheDBTooltip", "Purge unused assets data in the cache db."))
						.IsEnabled(true)
						.OnClicked(this, &SOasisContentBrowser::HandlePurgeCacheDBClicked)
						.ContentPadding(FMargin(1, 1))
						[
							SNew(SHorizontalBox)

							// Icon
							+ SHorizontalBox::Slot()
							.VAlign(VAlign_Bottom)
							.AutoWidth()
							[
								SNew(STextBlock)
								.TextStyle(FAppStyle::Get(), "ContentBrowser.TopBar.Font")
								.Font(FAppStyle::Get().GetFontStyle("FontAwesome.9"))
								.Text(FEditorFontGlyphs::Trash)
							]
						]
					]
#endif
#endif
				]
			]
			
		]
#endif
	]];

	WidthLastFrame = 1000.f; // width enough to show all buttons

	bShowChangelog = false;
	MainOverlay->AddSlot()[CreateChangeLogWidget()];

	UpdateAssetReferenceSplitterOrientation();

	AssetContextMenu = MakeShareable(new FAssetContextMenu(AssetViewPtr));

#if ECB_LEGACY
	AssetContextMenu->BindCommands(Commands);
	AssetContextMenu->SetOnFindInAssetTreeRequested( FOnFindInAssetTreeRequested::CreateSP(this, &SOasisContentBrowser::OnFindInAssetTreeRequested) );
	AssetContextMenu->SetOnRenameRequested( FAssetContextMenu::FOnRenameRequested::CreateSP(this, &SOasisContentBrowser::OnRenameRequested) );
	AssetContextMenu->SetOnRenameFolderRequested( FAssetContextMenu::FOnRenameFolderRequested::CreateSP(this, &SOasisContentBrowser::OnRenameFolderRequested) );
	AssetContextMenu->SetOnAssetViewRefreshRequested( FAssetContextMenu::FOnAssetViewRefreshRequested::CreateSP( this, &SOasisContentBrowser::OnAssetViewRefreshRequested) );
	FavoritePathViewPtr->SetTreeTitle(LOCTEXT("Favorites", "Favorites"));
	if( Config != nullptr && Config->SelectedCollectionName.Name != NAME_None )
	{
		// Select the specified collection by default
		FSourcesData DefaultSourcesData( Config->SelectedCollectionName );
		TArray<FString> SelectedPaths;
		AssetViewPtr->SetSourcesData( DefaultSourcesData );
	}
	else
	{
		// Select /Game by default
		FSourcesData DefaultSourcesData(FName("/Game"));
		TArray<FString> SelectedPaths;
		TArray<FString> SelectedFavoritePaths;
		SelectedPaths.Add(TEXT("/Game"));
		PathViewPtr->SetSelectedPaths(SelectedPaths);
		AssetViewPtr->SetSourcesData(DefaultSourcesData);
		FavoritePathViewPtr->SetSelectedPaths(SelectedFavoritePaths);
	}
#endif

#if ECB_WIP_FAVORITE
	//Bind the path view filtering to the favorite path view search bar
	FavoritePathViewPtr->OnFavoriteSearchChanged.BindSP(PathViewPtr.Get(), &SOasisPathView::OnAssetTreeSearchBoxChanged);
	FavoritePathViewPtr->OnFavoriteSearchCommitted.BindSP(PathViewPtr.Get(), &SOasisPathView::OnAssetTreeSearchBoxCommitted);

	// Bind the favorites menu to update after folder changes in the path or asset view
	PathViewPtr->OnFolderPathChanged.BindSP(FavoritePathViewPtr.Get(), &SFavoritePathView::FixupFavoritesFromExternalChange);
	AssetViewPtr->OnFolderPathChanged.BindSP(FavoritePathViewPtr.Get(), &SFavoritePathView::FixupFavoritesFromExternalChange);
#endif

#if ECB_WIP_HISTORY
	// Set the initial history data
	HistoryManager.AddHistoryData();
#endif

	// Load settings if they were specified
	this->InstanceName = InInstanceName;
	LoadSettings(InInstanceName);


	// Expand Source View?
	{
		// in case we do not have a config, see what the global default settings are for the Sources Panel
		if (!bSourcesViewExpanded && OasisContentBrowserSettings->bOpenSourcesPanelByDefault)
		{
			SourcesViewExpandClicked();
		}
	}

#if ECB_WIP_COLLECTION
	// Bindings to manage history when items are deleted
	FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
	CollectionManagerModule.Get().OnCollectionRenamed().AddSP(this, &SOasisContentBrowser::HandleCollectionRenamed);
	CollectionManagerModule.Get().OnCollectionDestroyed().AddSP(this, &SOasisContentBrowser::HandleCollectionRemoved);
	CollectionManagerModule.Get().OnCollectionUpdated().AddSP(this, &SOasisContentBrowser::HandleCollectionUpdated);
#endif

	// Listen for when view settings are changed
	UOasisContentBrowserSettings::OnSettingChanged().AddSP(this, &SOasisContentBrowser::HandleSettingChanged);

#if ECB_WIP_BREADCRUMB
	// Update the breadcrumb trail path
	UpdatePath();
#endif
}

void SOasisContentBrowser::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (WidthLastFrame != AllottedGeometry.Size.X)
	{
		WidthLastFrame = AllottedGeometry.Size.X;
		ECB_LOG(Display, TEXT("[SOasisContentBrowser]WidthLastFrame: %.2f"), WidthLastFrame);
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SOasisContentBrowser::BindCommands()
{

	Commands = TSharedPtr< FUICommandList >(new FUICommandList);

#if ECB_LEGACY
	Commands->MapAction(FGenericCommands::Get().Delete, FUIAction(
		FExecuteAction::CreateSP(this, &SOasisContentBrowser::HandleDeleteCommandExecute),
		FCanExecuteAction::CreateSP(this, &SOasisContentBrowser::HandleDeleteCommandCanExecute)
	));

	Commands->MapAction(FContentBrowserCommands::Get().OpenAssetsOrFolders, FUIAction(
		FExecuteAction::CreateSP(this, &SOasisContentBrowser::HandleOpenAssetsOrFoldersCommandExecute)
	));

	Commands->MapAction(FContentBrowserCommands::Get().DirectoryUp, FUIAction(
		FExecuteAction::CreateSP(this, &SOasisContentBrowser::HandleDirectoryUpCommandExecute)
	));

#endif

	Commands->MapAction(FOasisContentBrowserCommands::Get().ImportSelectedUAsset, FUIAction(
		FExecuteAction::CreateSP(this, &SOasisContentBrowser::HandleImportSelectedCommandExecute)));

	Commands->MapAction(FOasisContentBrowserCommands::Get().ToggleDependencyViewer, FUIAction(
		FExecuteAction::CreateSP(this, &SOasisContentBrowser::HandleToggleDependencyViewerCommandExecute)));

#if ECB_WIP_DELEGATES
	// Allow extenders to add commands
	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FContentBrowserCommandExtender> CommmandExtenderDelegates = ContentBrowserModule.GetAllContentBrowserCommandExtenders();

	for (int32 i = 0; i < CommmandExtenderDelegates.Num(); ++i)
	{
		if (CommmandExtenderDelegates[i].IsBound())
		{
			CommmandExtenderDelegates[i].Execute(Commands.ToSharedRef(), FOnContentBrowserGetSelection::CreateSP(this, &SOasisContentBrowser::GetSelectionState));
		}
	}
#endif
}

EVisibility SOasisContentBrowser::GetCollectionViewVisibility() const
{
	return bAlwaysShowCollections ? EVisibility::Visible : ( GetDefault<UOasisContentBrowserSettings>()->GetDisplayCollections() ? EVisibility::Visible : EVisibility::Collapsed );
}

EVisibility SOasisContentBrowser::GetFavoriteFolderVisibility() const
{
	return GetDefault<UOasisContentBrowserSettings>()->GetDisplayFavorites() ? EVisibility::Visible : EVisibility::Collapsed;
}

#if ECB_DEBUG

FReply SOasisContentBrowser::HandlePrintCacheClicked()
{
	FOasisContentBrowserSingleton::GetAssetRegistry().PrintCacheStatus();
	return FReply::Handled();
}

FReply SOasisContentBrowser::HandleClearCacheClicked()
{
	FOasisContentBrowserSingleton::GetAssetRegistry().ClearCache();
	CollectGarbage(RF_NoFlags);
	return FReply::Handled();
}

FReply SOasisContentBrowser::HandlePrintAssetDataClicked()
{
	const TArray<FOasisAssetData>& SelectedAssets = AssetViewPtr->GetSelectedAssets();
	for (const FOasisAssetData& AssetData : SelectedAssets)
	{
		AssetData.PrintAssetData();
	}
	return FReply::Handled();
}

#endif

void SOasisContentBrowser::ShowPluginVersionChangeLog()
{

}

void SOasisContentBrowser::HandleSettingChanged(FName PropertyName)
{
	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UOasisContentBrowserSettings, ShowDependencyViewerUnderAssetView)) ||
		(PropertyName == NAME_None))
	{
		UpdateAssetReferenceSplitterOrientation();
	}
}

void SOasisContentBrowser::UpdateAssetReferenceSplitterOrientation()
{
	bool bVertical = GetDefault<UOasisContentBrowserSettings>()->ShowDependencyViewerUnderAssetView;
	if (bVertical)
	{
		AssetReferenceSplitterPtr->SetOrientation(Orient_Vertical);
	}
	else
	{
		AssetReferenceSplitterPtr->SetOrientation(Orient_Horizontal);
	}
}

FSlateColor SOasisContentBrowser::GetContentBrowserBorderBackgroundColor() const
{
	const UOasisContentBrowserSettings* Settings = GetDefault<UOasisContentBrowserSettings>();
#if ECB_WIP_CACHEDB
	if (Settings->bCacheMode)
	{
		return Settings->CacheModeBorderColor;
	}
	else
#endif
	{
		return FOasisContentBrowserStyle::CustomContentBrowserBorderBackgroundColor;
	}
}

FSlateColor SOasisContentBrowser::GetToolbarBackgroundColor() const
{
	return FOasisContentBrowserStyle::CustomToolbarBackgroundColor;
}

FSlateColor SOasisContentBrowser::GetSourceViewBackgroundColor() const
{
	return FOasisContentBrowserStyle::CustomSourceViewBackgroundColor;
}

FSlateColor SOasisContentBrowser::GetAssetViewBackgroundColor() const
{
	return FOasisContentBrowserStyle::CustomAssetViewBackgroundColor;
}

#if ECB_WIP_FAVORITE

void SOasisContentBrowser::ToggleFolderFavorite(const TArray<FString>& FolderPaths)
{
	bool bAddedFavorite = false;
	for (FString FolderPath : FolderPaths)
	{
		if (ContentBrowserUtils::IsFavoriteFolder(FolderPath))
		{
			ContentBrowserUtils::RemoveFavoriteFolder(FolderPath, false);
		}
		else
		{
			ContentBrowserUtils::AddFavoriteFolder(FolderPath, false);
			bAddedFavorite = true;
		}
	}
	GConfig->Flush(false, GEditorPerProjectIni);
	FavoritePathViewPtr->Populate();
	if(bAddedFavorite)
	{	
		FavoritePathViewPtr->SetSelectedPaths(FolderPaths);
		if (GetFavoriteFolderVisibility() == EVisibility::Collapsed)
		{
			UOasisContentBrowserSettings* Settings = GetMutableDefault<UOasisContentBrowserSettings>();
			Settings->SetDisplayFavorites(true);
			Settings->SaveConfig();
		}
	}
}

#endif

#if ECB_FEA_SEARCH	

EVisibility SOasisContentBrowser::GetAlternateSearchBarVisibility() const
{
	return GetDefault<UOasisContentBrowserSettings>()->GetDisplayFavorites() ? EVisibility::Collapsed : EVisibility::Visible;
}

void SOasisContentBrowser::HandleAssetViewSearchOptionsChanged()
{
#if ECB_LEGACY
	TextFilter->SetIncludeClassName(AssetViewPtr->IsIncludingClassNames());
	TextFilter->SetIncludeAssetPath(AssetViewPtr->IsIncludingAssetPaths());
	TextFilter->SetIncludeCollectionNames(AssetViewPtr->IsIncludingCollectionNames());
#endif
}

void SOasisContentBrowser::HandleRequestShowChangeLog()
{
	bShowChangelog = true;
}

#endif


FText SOasisContentBrowser::GetHighlightedText() const
{
	return TextFilter->GetRawFilterText();
}

bool SOasisContentBrowser::IsImportEnabled() const
{
	// Todo: enable import bulk aseets and folders
	const TArray<FOasisAssetData>& SelectedAssets = AssetViewPtr->GetSelectedAssets();
#if ECB_WIP_MULTI_IMPORT
	for (const auto& Asset : SelectedAssets)
	{
		if (Asset.CanImportFast())
		{
			return true;
		}
	}
	return false;
#else
	return SelectedAssets.Num() == 1 && SelectedAssets[0].CanImportFast();// || AssetViewPtr->GetSelectedFolders().Num() > 0;
#endif
}

FText SOasisContentBrowser::GetImportTooltipText() const
{
// 	const FSourcesData& SourcesData = AssetViewPtr->GetSourcesData();
// 
// 	if ( SourcesData.PackagePaths.Num() == 1 )
// 	{
// 		const FString CurrentPath = SourcesData.PackagePaths[0].ToString();
// 		if ( ContentBrowserUtils::IsClassPath( CurrentPath ) )
// 		{
// 			return LOCTEXT( "ImportAssetToolTip_InvalidClassPath", "Cannot import assets to class paths." );
// 		}
// 		else
// 		{
// 			return FText::Format( LOCTEXT( "ImportAssetToolTip", "Import to {0}..." ), FText::FromString( CurrentPath ) );
// 		}
// 	}
// 	else if ( SourcesData.PackagePaths.Num() > 1 )
// 	{
// 		return LOCTEXT( "ImportAssetToolTip_MultiplePaths", "Cannot import assets to multiple paths." );
// 	}
// 	
// 	return LOCTEXT( "ImportAssetToolTip_NoPath", "No path is selected as an import target." );
	return LOCTEXT("ImportAssetToolTip", "Import .uasset files...");
}

FReply SOasisContentBrowser::HandleAddContentFolderClicked()
{
	AddContentFolder();
	return FReply::Handled();
}

void SOasisContentBrowser::AddContentFolder()
{
	PathContextMenu->ExecuteAddRootFolder();
}

FReply SOasisContentBrowser::HandleRemoveContentFoldersClicked()
{
	RemoveContentFolders();
	return FReply::Handled();
}

#if ECB_WIP_SYNC_ASSET

void SOasisContentBrowser::PrepareToSync( const TArray<FOasisAssetData>& AssetDataList, const TArray<FString>& FolderPaths, const bool bDisableFiltersThatHideAssets )
{
	// Check to see if any of the assets require certain folders to be visible
	bool bDisplayDev = GetDefault<UOasisContentBrowserSettings>()->GetDisplayDevelopersFolder();
	bool bDisplayEngine = GetDefault<UOasisContentBrowserSettings>()->GetDisplayEngineFolder();
	bool bDisplayPlugins = GetDefault<UOasisContentBrowserSettings>()->GetDisplayPluginFolders();
	bool bDisplayLocalized = GetDefault<UOasisContentBrowserSettings>()->GetDisplayL10NFolder();
	if ( !bDisplayDev || !bDisplayEngine || !bDisplayPlugins || !bDisplayLocalized )
	{
		TSet<FString> PackagePaths = TSet<FString>(FolderPaths);
		for (const FOasisAssetData& AssetData : AssetDataList)
		{
			FString PackagePath = AssetData.PackageFilePath.PackagePath.ToString();
			PackagePaths.Add(PackagePath);
		}

		bool bRepopulate = false;
#if ECB_LEGACY
		for (const FString& PackagePath : PackagePaths)
		{
			const OasisContentBrowserUtils::ECBFolderCategory FolderCategory = OasisContentBrowserUtils::GetFolderCategory( PackagePath );
			if ( !bDisplayDev && FolderCategory == OasisContentBrowserUtils::ECBFolderCategory::DeveloperContent )
			{
				bDisplayDev = true;
				GetMutableDefault<UOasisContentBrowserSettings>()->SetDisplayDevelopersFolder(true, true);
				bRepopulate = true;
			}
			else if ( !bDisplayEngine && (FolderCategory == OasisContentBrowserUtils::ECBFolderCategory::EngineContent || FolderCategory == OasisContentBrowserUtils::ECBFolderCategory::EngineClasses) )
			{
				bDisplayEngine = true;
				GetMutableDefault<UOasisContentBrowserSettings>()->SetDisplayEngineFolder(true, true);
				bRepopulate = true;
			}
			else if ( !bDisplayPlugins && (FolderCategory == OasisContentBrowserUtils::ECBFolderCategory::PluginContent || FolderCategory == OasisContentBrowserUtils::ECBFolderCategory::PluginClasses) )
			{
				bDisplayPlugins = true;
				GetMutableDefault<UOasisContentBrowserSettings>()->SetDisplayPluginFolders(true, true);
				bRepopulate = true;
			}

			if (!bDisplayLocalized && OasisContentBrowserUtils::IsLocalizationFolder(PackagePath))
			{
				bDisplayLocalized = true;
				GetMutableDefault<UOasisContentBrowserSettings>()->SetDisplayL10NFolder(true);
				bRepopulate = true;
			}

			if (bDisplayDev && bDisplayEngine && bDisplayPlugins && bDisplayLocalized)
			{
				break;
			}
		}
#endif

		// If we have auto-enabled any flags, force a refresh
		if ( bRepopulate )
		{
			PathViewPtr->Populate();
#if ECB_WIP_FAVORITE
			FavoritePathViewPtr->Populate();
#endif
		}
	}

	if ( bDisableFiltersThatHideAssets )
	{
		// Disable the filter categories
		FilterListPtr->DisableFiltersThatHideAssets(AssetDataList);
	}

	// Disable the filter search (reset the filter, then clear the search text)
	// Note: we have to remove the filter immediately, we can't wait for OnSearchBoxChanged to hit
	SetSearchBoxText(FText::GetEmpty());
	SearchBoxPtr->SetText(FText::GetEmpty());
	SearchBoxPtr->SetError(FText::GetEmpty());
}

void SOasisContentBrowser::SyncToAssets( const TArray<FOasisAssetData>& AssetDataList, const bool bAllowImplicitSync, const bool bDisableFiltersThatHideAssets )
{
	PrepareToSync(AssetDataList, TArray<FString>(), bDisableFiltersThatHideAssets);

	// Tell the sources view first so the asset view will be up to date by the time we request the sync
	PathViewPtr->SyncToAssets(AssetDataList, bAllowImplicitSync);
#if ECB_WIP_FAVORITE
	FavoritePathViewPtr->SyncToAssets(AssetDataList, bAllowImplicitSync);
#endif
	AssetViewPtr->SyncToAssets(AssetDataList);
}

void SOasisContentBrowser::SyncToFolders( const TArray<FString>& FolderList, const bool bAllowImplicitSync )
{
	PrepareToSync(TArray<FAssetData>(), FolderList, false);

	// Tell the sources view first so the asset view will be up to date by the time we request the sync
	PathViewPtr->SyncToFolders(FolderList, bAllowImplicitSync);
#if ECB_WIP_FAVORITE
	FavoritePathViewPtr->SyncToFolders(FolderList, bAllowImplicitSync);
#endif
	AssetViewPtr->SyncToFolders(FolderList);
}

void SOasisContentBrowser::SyncTo( const FOasisContentBrowserSelection& ItemSelection, const bool bAllowImplicitSync, const bool bDisableFiltersThatHideAssets )
{
	PrepareToSync(ItemSelection.SelectedAssets, ItemSelection.SelectedFolders, bDisableFiltersThatHideAssets);

	// Tell the sources view first so the asset view will be up to date by the time we request the sync
	PathViewPtr->SyncTo(ItemSelection, bAllowImplicitSync);
#if ECB_WIP_FAVORITE
	FavoritePathViewPtr->SyncTo(ItemSelection, bAllowImplicitSync);
#endif
	AssetViewPtr->SyncTo(ItemSelection);
}

#endif

void SOasisContentBrowser::SetIsPrimaryContentBrowser(bool NewIsPrimary)
{
	if (!CanSetAsPrimaryContentBrowser()) 
	{
		return;
	}

	bIsPrimaryBrowser = NewIsPrimary;
}

bool SOasisContentBrowser::CanSetAsPrimaryContentBrowser() const
{
	return bCanSetAsPrimaryBrowser;
}

#if ECB_WIP_MULTI_INSTANCES

const FName SOasisContentBrowser::GetInstanceName() const
{
	return InstanceName;
}

TSharedPtr<FTabManager> SOasisContentBrowser::GetTabManager() const
{
	if (ContainingTab.IsValid())
	{
		return ContainingTab.Pin()->GetTabManager();
	}

	return NULL;
}

void SOasisContentBrowser::OpenNewContentBrowser()
{
	//FContentBrowserSingleton::Get().SyncBrowserToFolders(PathContextMenu->GetSelectedPaths(), false, true, NAME_None, true);
}

#endif

#if ECB_LEGACY

void SOasisContentBrowser::LoadSelectedObjectsIfNeeded()
{
	// Get the selected assets in the asset view
	const TArray<FAssetData>& SelectedAssets = AssetViewPtr->GetSelectedAssets();

	// Load every asset that isn't already in memory
	for ( auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		const FAssetData& AssetData = *AssetIt;
		const bool bShowProgressDialog = (!AssetData.IsAssetLoaded() && FEditorFileUtils::IsMapPackageAsset(AssetData.ObjectPath.ToString()));
		GWarn->BeginSlowTask(LOCTEXT("LoadingObjects", "Loading Objects..."), bShowProgressDialog);

		(*AssetIt).GetAsset();

		GWarn->EndSlowTask();
	}
}

#endif

void SOasisContentBrowser::GetSelectedAssets(TArray<FOasisAssetData>& SelectedAssets)
{
	// Make sure the asset data is up to date
	AssetViewPtr->ProcessRecentlyLoadedOrChangedAssets();

	SelectedAssets = AssetViewPtr->GetSelectedAssets();
}

#if ECB_LEGACY

void SOasisContentBrowser::GetSelectedFolders(TArray<FString>& SelectedFolders)
{
	// Make sure the asset data is up to date
	AssetViewPtr->ProcessRecentlyLoadedOrChangedAssets();

	SelectedFolders = AssetViewPtr->GetSelectedFolders();
}

TArray<FString> SOasisContentBrowser::GetSelectedPathViewFolders()
{
	check(PathViewPtr.IsValid());
	return PathViewPtr->GetSelectedPaths();
}
#endif

void SOasisContentBrowser::SaveSettings() const
{
	const FString& SettingsString = InstanceName.ToString();

	GConfig->SetBool(*SettingsIniSection, *(SettingsString + TEXT(".SourcesExpanded")), bSourcesViewExpanded, GEditorPerProjectIni);

#if ECB_WIP_LOCK
	GConfig->SetBool(*SettingsIniSection, *(SettingsString + TEXT(".Locked")), bIsLocked, GEditorPerProjectIni);
#endif

#if ECB_TODO // Save splitter position
	for(int32 SlotIndex = 0; SlotIndex < PathAssetSplitterPtr->GetChildren()->Num(); SlotIndex++)
	{
		float SplitterSize = PathAssetSplitterPtr->SlotAt(SlotIndex).SizeValue.Get();
		GConfig->SetFloat(*SettingsIniSection, *(SettingsString + FString::Printf(TEXT(".VerticalSplitter.SlotSize%d"), SlotIndex)), SplitterSize, GEditorPerProjectIni);
	}
	
	for(int32 SlotIndex = 0; SlotIndex < PathCollectionSplitterPtr->GetChildren()->Num(); SlotIndex++)
	{
		float SplitterSize = PathCollectionSplitterPtr->SlotAt(SlotIndex).SizeValue.Get();
		GConfig->SetFloat(*SettingsIniSection, *(SettingsString + FString::Printf(TEXT(".HorizontalSplitter.SlotSize%d"), SlotIndex)), SplitterSize, GEditorPerProjectIni);
	}

	for (int32 SlotIndex = 0; SlotIndex < PathFavoriteSplitterPtr->GetChildren()->Num(); SlotIndex++)
	{
		float SplitterSize = PathFavoriteSplitterPtr->SlotAt(SlotIndex).SizeValue.Get();
		GConfig->SetFloat(*SettingsIniSection, *(SettingsString + FString::Printf(TEXT(".FavoriteSplitter.SlotSize%d"), SlotIndex)), SplitterSize, GEditorPerProjectIni);
	}
#endif

	// Save all our data using the settings string as a key in the user settings ini

	FilterListPtr->SaveSettings(GEditorPerProjectIni, SettingsIniSection, SettingsString);
#if ECB_TODO // save filter and path view settings
	PathViewPtr->SaveSettings(GEditorPerProjectIni, SettingsIniSection, SettingsString);
#endif

#if ECB_WIP_FAVORITE
	FavoritePathViewPtr->SaveSettings(GEditorPerProjectIni, SettingsIniSection, SettingsString + TEXT(".Favorites"));
#endif

#if ECB_WIP_COLLECTION
	CollectionViewPtr->SaveSettings(GEditorPerProjectIni, SettingsIniSection, SettingsString);
#endif

	AssetViewPtr->SaveSettings(GEditorPerProjectIni, SettingsIniSection, SettingsString);

	ReferenceViewerPtr->SaveSettings(GEditorPerProjectIni, SettingsIniSection, SettingsString);
}

void SOasisContentBrowser::LoadSettings(const FName& InInstanceName)
{
	FString SettingsString = InInstanceName.ToString();

	// Now that we have determined the appropriate settings string, actually load the settings
	GConfig->GetBool(*SettingsIniSection, *(SettingsString + TEXT(".SourcesExpanded")), bSourcesViewExpanded, GEditorPerProjectIni);
#if ECB_WIP_LOCK
	GConfig->GetBool(*SettingsIniSection, *(SettingsString + TEXT(".Locked")), bIsLocked, GEditorPerProjectIni);
#endif

#if ECB_TODO // load filter and path view settings
	for (int32 SlotIndex = 0; SlotIndex < PathAssetSplitterPtr->GetChildren()->Num(); SlotIndex++)
	{
		float SplitterSize = PathAssetSplitterPtr->SlotAt(SlotIndex).SizeValue.Get();
		GConfig->GetFloat(*SettingsIniSection, *(SettingsString + FString::Printf(TEXT(".VerticalSplitter.SlotSize%d"), SlotIndex)), SplitterSize, GEditorPerProjectIni);
		PathAssetSplitterPtr->SlotAt(SlotIndex).SizeValue = SplitterSize;
	}

	for (int32 SlotIndex = 0; SlotIndex < PathCollectionSplitterPtr->GetChildren()->Num(); SlotIndex++)
	{
		float SplitterSize = PathCollectionSplitterPtr->SlotAt(SlotIndex).SizeValue.Get();
		GConfig->GetFloat(*SettingsIniSection, *(SettingsString + FString::Printf(TEXT(".HorizontalSplitter.SlotSize%d"), SlotIndex)), SplitterSize, GEditorPerProjectIni);
		PathCollectionSplitterPtr->SlotAt(SlotIndex).SizeValue = SplitterSize;
	}

	for (int32 SlotIndex = 0; SlotIndex < PathFavoriteSplitterPtr->GetChildren()->Num(); SlotIndex++)
	{
		float SplitterSize = PathFavoriteSplitterPtr->SlotAt(SlotIndex).SizeValue.Get();
		GConfig->GetFloat(*SettingsIniSection, *(SettingsString + FString::Printf(TEXT(".FavoriteSplitter.SlotSize%d"), SlotIndex)), SplitterSize, GEditorPerProjectIni);
		PathFavoriteSplitterPtr->SlotAt(SlotIndex).SizeValue = SplitterSize;
	}
#endif

	// Load all our data using the settings string as a key in the user settings ini

	FilterListPtr->LoadSettings(GEditorPerProjectIni, SettingsIniSection, SettingsString);
#if ECB_TODO // load filter and path view settings	
	PathViewPtr->LoadSettings(GEditorPerProjectIni, SettingsIniSection, SettingsString);
#endif

#if ECB_WIP_FAVORITE
	FavoritePathViewPtr->LoadSettings(GEditorPerProjectIni, SettingsIniSection, SettingsString + TEXT(".Favorites"));
#endif

#if ECB_WIP_COLLECTION
	CollectionViewPtr->LoadSettings(GEditorPerProjectIni, SettingsIniSection, SettingsString);
#endif

	AssetViewPtr->LoadSettings(GEditorPerProjectIni, SettingsIniSection, SettingsString);

	ReferenceViewerPtr->LoadSettings(GEditorPerProjectIni, SettingsIniSection, SettingsString);
}

#if ECB_WIP_LOCK
bool SOasisContentBrowser::IsLocked() const
{
	return bIsLocked;
}
#endif

#if ECB_FEA_SEARCH
void SOasisContentBrowser::SetKeyboardFocusOnSearch() const
{
	// Focus on the search box
	FSlateApplication::Get().SetKeyboardFocus( SearchBoxPtr, EFocusCause::SetDirectly );
}
#endif

FReply SOasisContentBrowser::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{

	if( Commands->ProcessCommandBindings( InKeyEvent ) )
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

#if ECB_LEGACY

FReply SOasisContentBrowser::OnPreviewMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	// Clicking in a content browser will shift it to be the primary browser
	FContentBrowserSingleton::Get().SetPrimaryContentBrowser(SharedThis(this));

	return FReply::Unhandled();
}

FReply SOasisContentBrowser::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	// Mouse back and forward buttons traverse history
	if ( MouseEvent.GetEffectingButton() == EKeys::ThumbMouseButton)
	{
		HistoryManager.GoBack();
		return FReply::Handled();
	}
	else if ( MouseEvent.GetEffectingButton() == EKeys::ThumbMouseButton2)
	{
		HistoryManager.GoForward();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SOasisContentBrowser::OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	// Mouse back and forward buttons traverse history
	if ( InMouseEvent.GetEffectingButton() == EKeys::ThumbMouseButton)
	{
		HistoryManager.GoBack();
		return FReply::Handled();
	}
	else if ( InMouseEvent.GetEffectingButton() == EKeys::ThumbMouseButton2)
	{
		HistoryManager.GoForward();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}
#endif
void SOasisContentBrowser::OnContainingTabSavingVisualState() const
{
	SaveSettings();
}

void SOasisContentBrowser::OnContainingTabClosed(TSharedRef<SDockTab> DockTab)
{
	//FOasisContentBrowserSingleton::Get().ContentBrowserClosed( SharedThis(this) );
}

void SOasisContentBrowser::OnContainingTabActivated(TSharedRef<SDockTab> DockTab, ETabActivationCause InActivationCause)
{
	if(InActivationCause == ETabActivationCause::UserClickedOnTab)
	{
		//FOasisContentBrowserSingleton::Get().SetPrimaryContentBrowser(SharedThis(this));
	}
}

void SOasisContentBrowser::SourcesChanged(const TArray<FString>& SelectedPaths, const TArray<FCollectionNameType>& SelectedCollections)
{
	FString NewSource = SelectedPaths.Num() > 0 ? SelectedPaths[0] : (SelectedCollections.Num() > 0 ? SelectedCollections[0].Name.ToString() : TEXT("None"));

	ECB_LOG(Display, TEXT("The content browser source was changed by the sources view to '%s'"), *NewSource);

	FSourcesData SourcesData;
	{
		TArray<FName> SelectedPathNames;
		SelectedPathNames.Reserve(SelectedPaths.Num());
		for (const FString& SelectedPath : SelectedPaths)
		{
			// exclude loading path as source
			if (OasisContentBrowserUtils::IsFolderBackgroundGathering(SelectedPath))
			{
				continue;
			}

			SelectedPathNames.Add(FName(*SelectedPath));
		}
		SourcesData = FSourcesData(MoveTemp(SelectedPathNames), SelectedCollections);
	}
#if ECB_WIP_COLLECTION
	// A dynamic collection should apply its search query to the CB search, so we need to stash the current search so that we can restore it again later
	if (SourcesData.IsDynamicCollection())
	{
		// Only stash the user search term once in case we're switching between dynamic collections
		if (!StashedSearchBoxText.IsSet())
		{
			StashedSearchBoxText = TextFilter->GetRawFilterText();
		}

		FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

		const FCollectionNameType& DynamicCollection = SourcesData.Collections[0];

		FString DynamicQueryString;
		CollectionManagerModule.Get().GetDynamicQueryText(DynamicCollection.Name, DynamicCollection.Type, DynamicQueryString);

		const FText DynamicQueryText = FText::FromString(DynamicQueryString);
		SetSearchBoxText(DynamicQueryText);
		SearchBoxPtr->SetText(DynamicQueryText);
	}
	else if (StashedSearchBoxText.IsSet())
	{
		// Restore the stashed search term
		const FText StashedText = StashedSearchBoxText.GetValue();
		StashedSearchBoxText.Reset();

		SetSearchBoxText(StashedText);
		SearchBoxPtr->SetText(StashedText);
	}

	if (!AssetViewPtr->GetSourcesData().IsEmpty())
	{
		// Update the current history data to preserve selection if there is a valid SourcesData
		HistoryManager.UpdateHistoryData();
	}
#endif
	// Change the filter for the asset view
	AssetViewPtr->SetSourcesData(SourcesData);

#if ECB_WIP_HISTORY
	// Add a new history data now that the source has changed
	HistoryManager.AddHistoryData();
#endif

#if ECB_WIP_BREADCRUMB
	// Update the breadcrumb trail path
	UpdatePath();
#endif
}

void SOasisContentBrowser::FolderEntered(const FString& FolderPath)
{
#if ECB_WIP_COLLECTION
	// Have we entered a sub-collection folder?
	FName CollectionName;
	ECollectionShareType::Type CollectionFolderShareType = ECollectionShareType::CST_All;
	if (ContentBrowserUtils::IsCollectionPath(FolderPath, &CollectionName, &CollectionFolderShareType))
	{
		const FCollectionNameType SelectedCollection(CollectionName, CollectionFolderShareType);

		TArray<FCollectionNameType> Collections;
		Collections.Add(SelectedCollection);
		CollectionViewPtr->SetSelectedCollections(Collections);

		CollectionSelected(SelectedCollection);
	}
	else
#endif
	{
		// set the path view to the incoming path
		TArray<FString> SelectedPaths;
		SelectedPaths.Add(FolderPath);
		PathViewPtr->SetSelectedPaths(SelectedPaths);

		PathSelected(SelectedPaths[0]);
	}
}

void SOasisContentBrowser::PathSelected(const FString& FolderPath)
{
	// You may not select both collections and paths
#if ECB_WIP_COLLECTION
	CollectionViewPtr->ClearSelection();
#endif

	TArray<FString> SelectedPaths = PathViewPtr->GetSelectedPaths();

#if ECB_WIP_COLLECTION
	// Selecting a folder shows it in the favorite list also
	FavoritePathViewPtr->SetSelectedPaths(SelectedPaths);
#endif

	TArray<FCollectionNameType> SelectedCollections;
	SourcesChanged(SelectedPaths, SelectedCollections);

#if ECB_LEGACY
	// Notify 'asset path changed' delegate
	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>( TEXT("ContentBrowser") );
	FContentBrowserModule::FOnAssetPathChanged& PathChangedDelegate = ContentBrowserModule.GetOnAssetPathChanged();
	if(PathChangedDelegate.IsBound())
	{
		PathChangedDelegate.Broadcast(FolderPath);
	}
#endif

	// Update the context menu's selected paths list
	PathContextMenu->SetSelectedPaths(SelectedPaths);

	bCanRemoveContentFolders = FOasisContentBrowserSingleton::GetAssetRegistry().IsRootFolders(SelectedPaths);
}

#if ECB_WIP_FAVORITE
void SOasisContentBrowser::FavoritePathSelected(const FString& FolderPath)
{
	// You may not select both collections and paths
	CollectionViewPtr->ClearSelection();

	TArray<FString> SelectedPaths = FavoritePathViewPtr->GetSelectedPaths();
	// Selecting a favorite shows it in the main list also
	PathViewPtr->SetSelectedPaths(SelectedPaths);
	TArray<FCollectionNameType> SelectedCollections;
	SourcesChanged(SelectedPaths, SelectedCollections);

	// Notify 'asset path changed' delegate
	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	FContentBrowserModule::FOnAssetPathChanged& PathChangedDelegate = ContentBrowserModule.GetOnAssetPathChanged();
	if (PathChangedDelegate.IsBound())
	{
		PathChangedDelegate.Broadcast(FolderPath);
	}

	// Update the context menu's selected paths list
	PathContextMenu->SetSelectedPaths(SelectedPaths);
}
#endif

TSharedRef<FExtender> SOasisContentBrowser::GetPathContextMenuExtender(const TArray<FString>& InSelectedPaths) const
{
	return PathContextMenu->MakePathViewContextMenuExtender(InSelectedPaths);
}

#if ECB_WIP_COLLECTION
void SOasisContentBrowser::CollectionSelected(const FCollectionNameType& SelectedCollection)
{
	// You may not select both collections and paths
	PathViewPtr->ClearSelection();
	FavoritePathViewPtr->ClearSelection();

	TArray<FCollectionNameType> SelectedCollections = CollectionViewPtr->GetSelectedCollections();
	TArray<FString> SelectedPaths;

	if( SelectedCollections.Num() == 0  )
	{
		// just select the game folder
		SelectedPaths.Add(TEXT("/Game"));
		SourcesChanged(SelectedPaths, SelectedCollections);
	}
	else
	{
		SourcesChanged(SelectedPaths, SelectedCollections);
	}

}

void SOasisContentBrowser::PathPickerPathSelected(const FString& FolderPath)
{
	PathPickerButton->SetIsOpen(false);

	if ( !FolderPath.IsEmpty() )
	{
		TArray<FString> Paths;
		Paths.Add(FolderPath);
		PathViewPtr->SetSelectedPaths(Paths);
		FavoritePathViewPtr->SetSelectedPaths(Paths);
	}

	PathSelected(FolderPath);
}

void SOasisContentBrowser::SetSelectedPaths(const TArray<FString>& FolderPaths, bool bNeedsRefresh/* = false */)
{
	if (FolderPaths.Num() > 0)
	{
		if (bNeedsRefresh)
		{
			PathViewPtr->Populate();
			FavoritePathViewPtr->Populate();
		}

		PathViewPtr->SetSelectedPaths(FolderPaths);
		FavoritePathViewPtr->SetSelectedPaths(FolderPaths);
		PathSelected(FolderPaths[0]);
	}
}

void SOasisContentBrowser::PathPickerCollectionSelected(const FCollectionNameType& SelectedCollection)
{
	PathPickerButton->SetIsOpen(false);

	TArray<FCollectionNameType> Collections;
	Collections.Add(SelectedCollection);
	CollectionViewPtr->SetSelectedCollections(Collections);

	CollectionSelected(SelectedCollection);
}

void SOasisContentBrowser::OnApplyHistoryData( const FHistoryData& History )
{
	PathViewPtr->ApplyHistoryData(History);
	FavoritePathViewPtr->ApplyHistoryData(History);
	CollectionViewPtr->ApplyHistoryData(History);
	AssetViewPtr->ApplyHistoryData(History);

	// Update the breadcrumb trail path
	UpdatePath();

	if (History.SourcesData.HasPackagePaths())
	{
		// Notify 'asset path changed' delegate
		FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
		FContentBrowserModule::FOnAssetPathChanged& PathChangedDelegate = ContentBrowserModule.GetOnAssetPathChanged();
		if (PathChangedDelegate.IsBound())
		{
			PathChangedDelegate.Broadcast(History.SourcesData.PackagePaths[0].ToString());
		}
	}
}

void SOasisContentBrowser::OnUpdateHistoryData(FHistoryData& HistoryData) const
{
	const FSourcesData& SourcesData = AssetViewPtr->GetSourcesData();
	const TArray<FAssetData>& SelectedAssets = AssetViewPtr->GetSelectedAssets();

	const FText NewSource = SourcesData.HasPackagePaths() ? FText::FromName(SourcesData.PackagePaths[0]) : (SourcesData.HasCollections() ? FText::FromName(SourcesData.Collections[0].Name) : LOCTEXT("AllAssets", "All Assets"));

	HistoryData.HistoryDesc = NewSource;
	HistoryData.SourcesData = SourcesData;

	HistoryData.SelectionData.Reset();
	HistoryData.SelectionData.SelectedFolders = TSet<FString>(AssetViewPtr->GetSelectedFolders());
	for (const FAssetData& SelectedAsset : SelectedAssets)
	{
		HistoryData.SelectionData.SelectedAssets.Add(SelectedAsset.ObjectPath);
	}
}

#endif

#if ECB_FEA_SEARCH
void SOasisContentBrowser::SetSearchBoxText(const FText& InSearchText)
{
	// Has anything changed? (need to test case as the operators are case-sensitive)
	if (!InSearchText.ToString().Equals(TextFilter->GetRawFilterText().ToString(), ESearchCase::CaseSensitive))
	{
		TextFilter->SetRawFilterText( InSearchText );
		SearchBoxPtr->SetError( TextFilter->GetFilterErrorText() );
		if(InSearchText.IsEmpty())
		{
			FrontendFilters->Remove(TextFilter);
			AssetViewPtr->SetUserSearching(false);
		}
		else
		{
			FrontendFilters->Add(TextFilter);
			AssetViewPtr->SetUserSearching(true);
		}
	}
}

void SOasisContentBrowser::OnSearchBoxChanged(const FText& InSearchText)
{
	SetSearchBoxText(InSearchText);

#if ECB_LEGACY
	// Broadcast 'search box changed' delegate
	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>( TEXT("ContentBrowser") );
	ContentBrowserModule.GetOnSearchBoxChanged().Broadcast(InSearchText, bIsPrimaryBrowser);	
#endif
}

void SOasisContentBrowser::OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo)
{
	SetSearchBoxText(InSearchText);
}
#endif

#if ECB_TODO // todo: Save search?
bool SOasisContentBrowser::IsSaveSearchButtonEnabled() const
{
	return !TextFilter->GetRawFilterText().IsEmptyOrWhitespace();
}

FReply SOasisContentBrowser::OnSaveSearchButtonClicked()
{
	// Need to make sure we can see the collections view
	if (!bSourcesViewExpanded)
	{
		SourcesViewExpandClicked();
	}

	// We want to add any currently selected paths to the final saved query so that you get back roughly the same list of objects as what you're currently seeing
	FString SelectedPathsQuery;
	{
		const FSourcesData& SourcesData = AssetViewPtr->GetSourcesData();
		for (int32 SelectedPathIndex = 0; SelectedPathIndex < SourcesData.PackagePaths.Num(); ++SelectedPathIndex)
		{
			SelectedPathsQuery.Append(TEXT("Path:'"));
			SelectedPathsQuery.Append(SourcesData.PackagePaths[SelectedPathIndex].ToString());
			SelectedPathsQuery.Append(TEXT("'..."));

			if (SelectedPathIndex + 1 < SourcesData.PackagePaths.Num())
			{
				SelectedPathsQuery.Append(TEXT(" OR "));
			}
		}
	}

	// todo: should we automatically append any type filters too?

	// Produce the final query
	FText FinalQueryText;
	if (SelectedPathsQuery.IsEmpty())
	{
		FinalQueryText = TextFilter->GetRawFilterText();
	}
	else
	{
		FinalQueryText = FText::FromString(FString::Printf(TEXT("(%s) AND (%s)"), *TextFilter->GetRawFilterText().ToString(), *SelectedPathsQuery));
	}

	CollectionViewPtr->MakeSaveDynamicCollectionMenu(FinalQueryText);
	return FReply::Handled();
}

void SOasisContentBrowser::OnPathClicked( const FString& CrumbData )
{
	FSourcesData SourcesData = AssetViewPtr->GetSourcesData();

	if ( SourcesData.HasCollections() )
	{
		// Collection crumb was clicked. See if we've clicked on a different collection in the hierarchy, and change the path if required.
		TOptional<FCollectionNameType> CollectionClicked;
		{
			FString CollectionName;
			FString CollectionTypeString;
			if (CrumbData.Split(TEXT("?"), &CollectionName, &CollectionTypeString))
			{
				const int32 CollectionType = FCString::Atoi(*CollectionTypeString);
				if (CollectionType >= 0 && CollectionType < ECollectionShareType::CST_All)
				{
					CollectionClicked = FCollectionNameType(FName(*CollectionName), ECollectionShareType::Type(CollectionType));
				}
			}
		}

		if ( CollectionClicked.IsSet() && SourcesData.Collections[0] != CollectionClicked.GetValue() )
		{
			TArray<FCollectionNameType> Collections;
			Collections.Add(CollectionClicked.GetValue());
			CollectionViewPtr->SetSelectedCollections(Collections);

			CollectionSelected(CollectionClicked.GetValue());
		}
	}
	else if ( !SourcesData.HasPackagePaths() )
	{
		// No collections or paths are selected. This is "All Assets". Don't change the path when this is clicked.
	}
	else if ( SourcesData.PackagePaths.Num() > 1 || SourcesData.PackagePaths[0].ToString() != CrumbData )
	{
		// More than one path is selected or the crumb that was clicked is not the same path as the current one. Change the path.
		TArray<FString> SelectedPaths;
		SelectedPaths.Add(CrumbData);
		PathViewPtr->SetSelectedPaths(SelectedPaths);
		FavoritePathViewPtr->SetSelectedPaths(SelectedPaths);
		PathSelected(SelectedPaths[0]);
	}
}

void SOasisContentBrowser::OnPathMenuItemClicked(FString ClickedPath)
{
	OnPathClicked( ClickedPath );
}

TSharedPtr<SWidget> SOasisContentBrowser::OnGetCrumbDelimiterContent(const FString& CrumbData) const
{
	FSourcesData SourcesData = AssetViewPtr->GetSourcesData();

	TSharedPtr<SWidget> Widget = SNullWidget::NullWidget;
	TSharedPtr<SWidget> MenuWidget;

	if( SourcesData.HasCollections() )
	{
		TOptional<FCollectionNameType> CollectionClicked;
		{
			FString CollectionName;
			FString CollectionTypeString;
			if (CrumbData.Split(TEXT("?"), &CollectionName, &CollectionTypeString))
			{
				const int32 CollectionType = FCString::Atoi(*CollectionTypeString);
				if (CollectionType >= 0 && CollectionType < ECollectionShareType::CST_All)
				{
					CollectionClicked = FCollectionNameType(FName(*CollectionName), ECollectionShareType::Type(CollectionType));
				}
			}
		}

		if( CollectionClicked.IsSet() )
		{
			FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

			TArray<FCollectionNameType> ChildCollections;
			CollectionManagerModule.Get().GetChildCollections(CollectionClicked->Name, CollectionClicked->Type, ChildCollections);

			if( ChildCollections.Num() > 0 )
			{
				FMenuBuilder MenuBuilder( true, nullptr );

				for( const FCollectionNameType& ChildCollection : ChildCollections )
				{
					const FString ChildCollectionCrumbData = FString::Printf(TEXT("%s?%s"), *ChildCollection.Name.ToString(), *FString::FromInt(ChildCollection.Type));

					MenuBuilder.AddMenuEntry(
						FText::FromName(ChildCollection.Name),
						FText::GetEmpty(),
						FSlateIcon(FAppStyle::GetStyleSetName(), ECollectionShareType::GetIconStyleName(ChildCollection.Type)),
						FUIAction(FExecuteAction::CreateSP(const_cast<SOasisContentBrowser*>(this), &SOasisContentBrowser::OnPathMenuItemClicked, ChildCollectionCrumbData))
						);
				}

				MenuWidget = MenuBuilder.MakeWidget();
			}
		}
	}
	else if( SourcesData.HasPackagePaths() )
	{
		TArray<FString> SubPaths;
		const bool bRecurse = false;
		if( ContentBrowserUtils::IsClassPath(CrumbData) )
		{
			TSharedRef<FNativeClassHierarchy> NativeClassHierarchy = FContentBrowserSingleton::Get().GetNativeClassHierarchy();

			FNativeClassHierarchyFilter ClassFilter;
			ClassFilter.ClassPaths.Add(FName(*CrumbData));
			ClassFilter.bRecursivePaths = bRecurse;

			NativeClassHierarchy->GetMatchingFolders(ClassFilter, SubPaths);
		}
		else
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

			AssetRegistry.GetSubPaths( CrumbData, SubPaths, bRecurse );
		}

		if( SubPaths.Num() > 0 )
		{
			FMenuBuilder MenuBuilder( true, nullptr );

			for( int32 PathIndex = 0; PathIndex < SubPaths.Num(); ++PathIndex )
			{
				const FString& SubPath = SubPaths[PathIndex];

				// For displaying in the menu cut off the parent path since it is redundant
				FString PathWithoutParent = SubPath.RightChop( CrumbData.Len() + 1 );
				MenuBuilder.AddMenuEntry(
					FText::FromString(PathWithoutParent),
					FText::GetEmpty(),
					FSlateIcon(FAppStyle::GetStyleSetName(), "ContentBrowser.BreadcrumbPathPickerFolder"),
					FUIAction(FExecuteAction::CreateSP(const_cast<SOasisContentBrowser*>(this), &SOasisContentBrowser::OnPathMenuItemClicked, SubPath))
					);
			}

			MenuWidget = MenuBuilder.MakeWidget();
		}
	}

	if( MenuWidget.IsValid() )
	{
		// Do not allow the menu to become too large if there are many directories
		Widget =
			SNew( SVerticalBox )
			+SVerticalBox::Slot()
			.MaxHeight( 400.0f )
			[
				MenuWidget.ToSharedRef()
			];
	}

	return Widget;
}

TSharedRef<SWidget> SOasisContentBrowser::GetPathPickerContent()
{
	FPathPickerConfig PathPickerConfig;

	FSourcesData SourcesData = AssetViewPtr->GetSourcesData();
	if ( SourcesData.HasPackagePaths() )
	{
		PathPickerConfig.DefaultPath = SourcesData.PackagePaths[0].ToString();
	}
	
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateSP(this, &SOasisContentBrowser::PathPickerPathSelected);
	PathPickerConfig.bAllowContextMenu = false;
	PathPickerConfig.bAllowClassesFolder = true;

	return SNew(SBox)
		.WidthOverride(300)
		.HeightOverride(500)
		.Padding(4)
		[
			SNew(SVerticalBox)

			// Path Picker
			+SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				FContentBrowserSingleton::Get().CreatePathPicker(PathPickerConfig)
			]

			// Collection View
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 6, 0, 0)
			[
				SNew(SCollectionView)
				.AllowCollectionButtons(false)
				.OnCollectionSelected(this, &SOasisContentBrowser::PathPickerCollectionSelected)
				.AllowContextMenu(false)
			]
		];
}

FString SOasisContentBrowser::GetCurrentPath() const
{
	FString CurrentPath;
	const FSourcesData& SourcesData = AssetViewPtr->GetSourcesData();
	if ( SourcesData.HasPackagePaths() && SourcesData.PackagePaths[0] != NAME_None )
	{
		CurrentPath = SourcesData.PackagePaths[0].ToString();
	}

	return CurrentPath;
}
#endif

TSharedRef<SWidget> SOasisContentBrowser::MakeAddNewContextMenu(bool bShowGetContent, bool bShowImport)
{
#if ECB_LEGACY
	const FSourcesData& SourcesData = AssetViewPtr->GetSourcesData();

	int32 NumAssetPaths, NumClassPaths;
	ContentBrowserUtils::CountPathTypes(SourcesData.PackagePaths, NumAssetPaths, NumClassPaths);

	// Get all menu extenders for this context menu from the content browser module
	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>( TEXT("ContentBrowser") );
	TArray<FContentBrowserMenuExtender_SelectedPaths> MenuExtenderDelegates = ContentBrowserModule.GetAllAssetContextMenuExtenders();
	
	// Delegate wants paths as FStrings
	TArray<FString> SelectPaths;
	for (FName PathName: SourcesData.PackagePaths)
	{
		SelectPaths.Add(PathName.ToString());
	}

	TArray<TSharedPtr<FExtender>> Extenders;
	for (int32 i = 0; i < MenuExtenderDelegates.Num(); ++i)
	{
		if (MenuExtenderDelegates[i].IsBound())
		{
			Extenders.Add(MenuExtenderDelegates[i].Execute(SelectPaths));
		}
	}
	TSharedPtr<FExtender> MenuExtender = FExtender::Combine(Extenders);

	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, NULL, MenuExtender);

	// Only add "New Folder" item if we do not have a collection selected
	FNewAssetOrClassContextMenu::FOnNewFolderRequested OnNewFolderRequested;
	if (CollectionViewPtr->GetSelectedCollections().Num() == 0)
	{
		OnNewFolderRequested = FNewAssetOrClassContextMenu::FOnNewFolderRequested::CreateSP(this, &SOasisContentBrowser::NewFolderRequested);
	}


	// New feature packs don't depend on the current paths, so we always add this item if it was requested
	FNewAssetOrClassContextMenu::FOnGetContentRequested OnGetContentRequested;
	if(bShowGetContent)
	{
		OnGetContentRequested = FNewAssetOrClassContextMenu::FOnGetContentRequested::CreateSP(this, &SOasisContentBrowser::OnAddContentRequested);
	}

	// Only the asset items if we have an asset path selected
	FNewAssetOrClassContextMenu::FOnNewAssetRequested OnNewAssetRequested;
	FNewAssetOrClassContextMenu::FOnImportAssetRequested OnImportAssetRequested;
	if(NumAssetPaths > 0)
	{
		OnNewAssetRequested = FNewAssetOrClassContextMenu::FOnNewAssetRequested::CreateSP(this, &SOasisContentBrowser::NewAssetRequested);
		if(bShowImport)
		{
			OnImportAssetRequested = FNewAssetOrClassContextMenu::FOnImportAssetRequested::CreateSP(this, &SOasisContentBrowser::ImportAsset);
		}
	}

	// This menu always lets you create classes, but uses your default project source folder if the selected path is invalid for creating classes
	FNewAssetOrClassContextMenu::FOnNewClassRequested OnNewClassRequested = FNewAssetOrClassContextMenu::FOnNewClassRequested::CreateSP(this, &SOasisContentBrowser::NewClassRequested);

	FNewAssetOrClassContextMenu::MakeContextMenu(
		MenuBuilder, 
		SourcesData.PackagePaths, 
		OnNewAssetRequested,
		OnNewClassRequested,
		OnNewFolderRequested,
		OnImportAssetRequested,
		OnGetContentRequested
		);

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetCachedDisplayMetrics( DisplayMetrics );

	const FVector2D DisplaySize(
		DisplayMetrics.PrimaryDisplayWorkAreaRect.Right - DisplayMetrics.PrimaryDisplayWorkAreaRect.Left,
		DisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom - DisplayMetrics.PrimaryDisplayWorkAreaRect.Top );

	return 
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.MaxHeight(DisplaySize.Y * 0.9)
		[
			MenuBuilder.MakeWidget()
		];
#endif
	return SNullWidget::NullWidget;
}

#if ECB_LEGACY
bool SOasisContentBrowser::IsAddNewEnabled() const
{
	const FSourcesData& SourcesData = AssetViewPtr->GetSourcesData();
	return SourcesData.PackagePaths.Num() == 1;
}

FText SOasisContentBrowser::GetAddNewToolTipText() const
{
	const FSourcesData& SourcesData = AssetViewPtr->GetSourcesData();

	if ( SourcesData.PackagePaths.Num() == 1 )
	{
		const FString CurrentPath = SourcesData.PackagePaths[0].ToString();
		if ( ContentBrowserUtils::IsClassPath( CurrentPath ) )
		{
			return FText::Format( LOCTEXT("AddNewToolTip_AddNewClass", "Create a new class in {0}..."), FText::FromString(CurrentPath) );
		}
		else
		{
			return FText::Format( LOCTEXT("AddNewToolTip_AddNewAsset", "Create a new asset in {0}..."), FText::FromString(CurrentPath) );
		}
	}
	else if ( SourcesData.PackagePaths.Num() > 1 )
	{
		return LOCTEXT( "AddNewToolTip_MultiplePaths", "Cannot add assets or classes to multiple paths." );
	}
	
	return LOCTEXT( "AddNewToolTip_NoPath", "No path is selected as an add target." );
}
#endif

TSharedRef<SWidget> SOasisContentBrowser::MakeAddFilterMenu()
{
	return FilterListPtr->ExternalMakeAddFilterMenu();
}

TSharedPtr<SWidget> SOasisContentBrowser::GetFilterContextMenu()
{
	return FilterListPtr->ExternalMakeAddFilterMenu();
}

FReply SOasisContentBrowser::HandleImportClicked()
{
	const int32 NumSelected = AssetViewPtr->GetNumSelectedAssets();

#if ECB_WIP_MULTI_IMPORT
	if (NumSelected > 0)
#else
	if (NumSelected == 1)
#endif
	{
		const TArray<FOasisAssetData> SelectedAssets = AssetViewPtr->GetSelectedAssets();

		ImportAsset(SelectedAssets);
	}

	return FReply::Handled();

}

FReply SOasisContentBrowser::HandleFlatImportClicked()
{
	const TArray<FOasisAssetData> SelectedAssets = AssetViewPtr->GetSelectedAssets();
	ImportAssetFlatMode(SelectedAssets);

	return FReply::Handled();
}

FReply SOasisContentBrowser::HandleImportToSandboxClicked()
{
	const TArray<FOasisAssetData> SelectedAssets = AssetViewPtr->GetSelectedAssets();
	ImportAssetToSandbox(SelectedAssets);

	return FReply::Handled();
}

FReply SOasisContentBrowser::HandleExportAssetClicked()
{
	const TArray<FOasisAssetData> SelectedAssets = AssetViewPtr->GetSelectedAssets();
	FExtAssetImporter::ExportAssets(SelectedAssets);
	return FReply::Handled();
}

bool SOasisContentBrowser::IsExportAssetEnabled() const
{
	return IsImportEnabled();
}

FReply SOasisContentBrowser::HandleZipExportClicked()
{
	const TArray<FOasisAssetData> SelectedAssets = AssetViewPtr->GetSelectedAssets();
	FExtAssetImporter::ZipUpSourceAssets(SelectedAssets);
	return FReply::Handled();
}

void SOasisContentBrowser::ImportAsset(const TArray<FOasisAssetData>& InAssetDatas)
{	
	FExtAssetImporter::ImportAssets(InAssetDatas, FUAssetImportSetting::GetSavedImportSetting());
}

void SOasisContentBrowser::ImportAssetToSandbox(const TArray<FOasisAssetData>& InAssetDatas)
{
	FUAssetImportSetting ImportSetting = FUAssetImportSetting::GetSandboxImportSetting();
	FExtAssetImporter::ImportAssets(InAssetDatas, ImportSetting);
}

void SOasisContentBrowser::ImportAssetFlatMode(const TArray<FOasisAssetData>& InAssetDatas)
{
#if ECB_TODO // todo: double check flat import a level
	// Map is not supported
	for (const FOasisAssetData& AssetData : InAssetDatas)
	{
		if (AssetData.AssetClass == UWorld::StaticClass()->GetFName())
		{
			OasisContentBrowserUtils::DisplayMessagePopup(FText::FromString(TEXT("Map is not supporting Flat Import.")));
			return;
		}
	}
#endif

	FUAssetImportSetting ImportSetting = FUAssetImportSetting::GetFlatModeImportSetting();
	FExtAssetImporter::ImportAssetsWithPathPicker(InAssetDatas, ImportSetting);
}

bool SOasisContentBrowser::IsValidateEnabled() const
{
	return IsImportEnabled();
}

FReply SOasisContentBrowser::HandleValidateclicked()
{
	const TArray<FOasisAssetData> SelectedAssets = AssetViewPtr->GetSelectedAssets();
	if (SelectedAssets.Num() > 0)
	{
		FString ResultMessage;
		FExtAssetValidator::ValidateDependency(SelectedAssets, &ResultMessage, /*bShowProgess*/ true);

		if (SelectedAssets.Num() == 1)
		{
			OasisContentBrowserUtils::DisplayMessagePopup(FText::FromString(ResultMessage));
		}
		else
		{
			OasisContentBrowserUtils::NotifyMessage(FText::FromString(ResultMessage));
		}
	}
	return FReply::Handled();
}

#if ECB_WIP_CACHEDB
FReply SOasisContentBrowser::HandleLoadCacheDBclicked()
{
	FOasisContentBrowserSingleton::GetAssetRegistry().LoadCacheDB();
	return FReply::Handled();
}

FReply SOasisContentBrowser::HandleSaveCacheDBClicked()
{
	FOasisContentBrowserSingleton::GetAssetRegistry().SaveCacheDB(/*bSilent =*/ false);
	return FReply::Handled();
}

FReply SOasisContentBrowser::HandleSaveAndSwitchToCacheModeClicked()
{
	FOasisContentBrowserSingleton::GetAssetRegistry().SaveCacheDB(/*bSilent =*/ false);

	UOasisContentBrowserSettings* OasisContentBrowserSetting = GetMutableDefault<UOasisContentBrowserSettings>();
	if (!OasisContentBrowserSetting->bCacheMode)
	{
		OasisContentBrowserSetting->bCacheMode = true;
		FOasisContentBrowserSingleton::GetAssetRegistry().SwitchCacheMode();
	}
	return FReply::Handled();
}

FReply SOasisContentBrowser::HandlePurgeCacheDBClicked()
{
	int32 NumPurged = FOasisContentBrowserSingleton::GetAssetRegistry().PurgeCacheDB(/*bSilent*/false);

	return FReply::Handled();
}

void SOasisContentBrowser::HandleSwitchAndLoadCacheDBClicked(const FString& PickedPath)
{
	FString CleanPath = PickedPath.TrimStartAndEnd();
	CleanPath = FPaths::ConvertRelativePathToFull(CleanPath);
	FPaths::NormalizeDirectoryName(CleanPath);

	UOasisContentBrowserSettings* OasisContentBrowserSettings = GetMutableDefault<UOasisContentBrowserSettings>();
	if (OasisContentBrowserSettings->CacheFilePath.FilePath.Equals(CleanPath, ESearchCase::IgnoreCase))
	{
		return;
	}

	const bool bCacheMode = OasisContentBrowserSettings->bCacheMode;

	FString OrigFilePath = OasisContentBrowserSettings->CacheFilePath.FilePath;
	OasisContentBrowserSettings->CacheFilePath.FilePath = CleanPath;

	if (!FPaths::FileExists(CleanPath))
	{
		FOasisContentBrowserSingleton::GetAssetRegistry().SaveCacheDB(/*bSilent =*/ true);
		FString Message = FString::Printf(TEXT("Cache file path: %s does not exist, new file created."), *CleanPath);
		OasisContentBrowserUtils::NotifyMessage(Message);
	}
	else
	{
		FString Message;
		if (bCacheMode)
		{
			if (FOasisContentBrowserSingleton::GetAssetRegistry().LoadCacheDB(/*bSilent =*/ true))
			{
				Message = FString::Printf(TEXT("Use cache file path: %s."), *CleanPath);
				FOasisContentBrowserSingleton::GetAssetRegistry().LoadRootContentPaths();
			}
			else
			{
				Message = FString::Printf(TEXT("Can't use cache file: %s."), *CleanPath);
				OasisContentBrowserSettings->CacheFilePath.FilePath = OrigFilePath;
			}
		}
		OasisContentBrowserUtils::NotifyMessage(Message);		
	}

	OasisContentBrowserSettings->PostEditChange();
}

FText SOasisContentBrowser::GetSaveAndSwitchToCacheModeText() const
{
	const UOasisContentBrowserSettings* OasisContentBrowserSettings = GetDefault<UOasisContentBrowserSettings>();
	const bool bCacheMode = OasisContentBrowserSettings->bCacheMode;
	return bCacheMode ? LOCTEXT("Save", "Save") : LOCTEXT("Cache", "Cache");
}

FText SOasisContentBrowser::GetCacheStatusText() const
{
	int32 NumFoldersCached = FOasisContentBrowserSingleton::GetAssetRegistry().GetNumFoldersInCache();
	int32 NumAssetsCached = FOasisContentBrowserSingleton::GetAssetRegistry().GetNumAssetsInCache();
	int32 NumFoldersMem = FOasisContentBrowserSingleton::GetAssetRegistry().GetNumFoldersInMem();	
	int32 NumAssetsMem = FOasisContentBrowserSingleton::GetAssetRegistry().GetNumAssetsInMem();	

	FText CacheStatus = FText::Format(LOCTEXT("CacheStatusText", "{0} folders {1} assets cached, {2} assets in memory. ")
		, FText::AsNumber(NumFoldersCached)
		, FText::AsNumber(NumAssetsCached)
		, FText::AsNumber(NumAssetsMem));

	FText CacheStatusNonCacheMode = FText::Format(LOCTEXT("CacheStatusNonCacheModeText", "{0} folders {1} assets in memory. ")
		, FText::AsNumber(NumFoldersMem)
		, FText::AsNumber(NumAssetsMem));

	const UOasisContentBrowserSettings* OasisContentBrowserSettings = GetDefault<UOasisContentBrowserSettings>();
	const bool bCacheMode = OasisContentBrowserSettings->bCacheMode;
	return bCacheMode ? CacheStatus : CacheStatusNonCacheMode;
}

EVisibility SOasisContentBrowser::GetCacheStatusBarVisibility() const
{
	const UOasisContentBrowserSettings* OasisContentBrowserSettings = GetDefault<UOasisContentBrowserSettings>();
	return (OasisContentBrowserSettings->bCacheMode || OasisContentBrowserSettings->bShowCacheStatusBarInLiveMode)
		? EVisibility::Visible : EVisibility::Collapsed;
}
#endif

#if ECB_FEA_IMPORT_OPTIONS
FSlateColor SOasisContentBrowser::GetImportOptionsButtonForegroundColor() const
{
	static const FName InvertedForegroundName("InvertedForeground");
	static const FName DefaultForegroundName("DefaultForeground");

	return ImportOptionsComboButton->IsHovered() ? FAppStyle::GetSlateColor(InvertedForegroundName) : FAppStyle::GetSlateColor(DefaultForegroundName);
}

DECLARE_DELEGATE(FOnColorPicked);
DECLARE_DELEGATE(FOnSpintBoxVauleComitted);
namespace LocalWidgetHelpers
{
	TSharedRef<SWidget> CreateColorWidget(FLinearColor* Color, FOnColorPicked OnColorPicked = FOnColorPicked())
	{
		auto GetValue = [=] {
			return *Color;
		};

		auto SetValue = [=](FLinearColor NewColor) {
			*Color = NewColor;
			OnColorPicked.ExecuteIfBound();
		};

		auto OnGetMenuContent = [=]() -> TSharedRef<SWidget> {
			// Open a color picker
			return SNew(SColorPicker)
				.TargetColorAttribute_Lambda(GetValue)
				.UseAlpha(true)
				.DisplayInlineVersion(true)
				.OnColorCommitted_Lambda(SetValue);
		};

		return SNew(SComboButton)
			.ContentPadding(0)
			.HasDownArrow(false)
			.ButtonStyle(FAppStyle::Get(), "Sequencer.AnimationOutliner.ColorStrip")
			.OnGetMenuContent_Lambda(OnGetMenuContent)
			.CollapseMenuOnParentFocus(true)
			.ButtonContent()
			[
				SNew(SColorBlock)
				.Color_Lambda(GetValue)
			.ShowBackgroundForAlpha(true)
			.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Ignore)
			.Size(FVector2D(10.0f, 10.0f))
			];
	}

	template<typename T>
	TSharedRef<SWidget> CreateSpinBox(T* Value, T Min, T Max, FOnSpintBoxVauleComitted OnSpintBoxVauleComitted = FOnSpintBoxVauleComitted(), TSharedPtr<INumericTypeInterface<T>> TypeInterface = nullptr)
	{
		auto GetValue = [=] { return *Value; };
		auto SetValue = [=](T NewValue) { *Value = NewValue; };
		auto SetValueCommitted = [=](T NewValue, ETextCommit::Type) { *Value = NewValue; OnSpintBoxVauleComitted.ExecuteIfBound(); };

		return SNew(SSpinBox<T>)
			.MinValue(Min)
			.MaxValue(Max)
			.Value_Lambda(GetValue)
			.OnValueChanged_Lambda(SetValue)
			.OnValueCommitted_Lambda(SetValueCommitted)
			.TypeInterface(TypeInterface);
	}
}

TSharedRef<SWidget> SOasisContentBrowser::GetImportOptionsButtonContent()
{
	UOasisContentBrowserSettings* OasisContentBrowserSetting = GetMutableDefault<UOasisContentBrowserSettings>();

	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, NULL, TSharedPtr<FExtender>(), /*bCloseSelfOnly=*/ true);

#if ECB_WIP_CACHEDB
	MenuBuilder.BeginSection("CacheDB", LOCTEXT("CacheHeading", "Cache"));
	{
		// Enable CacheMode
		MenuBuilder.AddMenuEntry(
			LOCTEXT("CacheMode", "Cache Mode"),
			LOCTEXT("CacheModeTooltip", "Use Cache for uasset files information. Please note that cache info might not in sync with uasset files on disk."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] 
				{
						bool bWasCacheMode = OasisContentBrowserSetting->bCacheMode;
						OasisContentBrowserSetting->bCacheMode = !OasisContentBrowserSetting->bCacheMode;
						OasisContentBrowserSetting->PostEditChange();
						if (bWasCacheMode && OasisContentBrowserSetting->bAutoSaveCacheOnSwitchToCacheMode)
						{
							FOasisContentBrowserSingleton::GetAssetRegistry().SaveCacheDB();
						}
						FOasisContentBrowserSingleton::GetAssetRegistry().SwitchCacheMode();
				}),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bCacheMode; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		// Cache Mode Auto Save on Exit
#if ECB_FOLD 
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AutoSaveCacheOnExit", "Auto Save Cache on Exit"),
			LOCTEXT("AutoSaveCacheOnExitTooltip", "Auto save cache on close SpyEngine or leaving cache mode"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bAutoSaveCacheOnExit = !OasisContentBrowserSetting->bAutoSaveCacheOnExit; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bAutoSaveCacheOnExit; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AutoSaveCacheOnSwitchToCacheMode", "Auto Save Cache When Switch To Cache Mode"),
			LOCTEXT("AutoSaveCacheOnSwitchToCacheModeTooltip", "Auto save cache when switching to cache mode"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bAutoSaveCacheOnSwitchToCacheMode = !OasisContentBrowserSetting->bAutoSaveCacheOnSwitchToCacheMode; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bAutoSaveCacheOnSwitchToCacheMode; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
		// Cache Mode Auto Save on Close
		MenuBuilder.AddMenuEntry(
			LOCTEXT("KeepCachedAssetsWhenRootRemoved", "Keep Cached Assets When Folder Removed"),
			LOCTEXT("KeepCachedAssetsWhenRootRemovedTooltip", "Keep cached assets when root content folder removed."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bKeepCachedAssetsWhenRootRemoved = !OasisContentBrowserSetting->bKeepCachedAssetsWhenRootRemoved; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bKeepCachedAssetsWhenRootRemoved; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif
		// Always Show Cache Status Bar
#if ECB_FOLD 
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowCacheStatusBarOnLiveMode", "Show Cache StatusBar in Live Mode"),
			LOCTEXT("ShowCacheStatusBarOnLiveModeTooltip", "Show Cache StatusBar in both Live Mode and Cache Mode"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bShowCacheStatusBarInLiveMode = !OasisContentBrowserSetting->bShowCacheStatusBarInLiveMode; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bShowCacheStatusBarInLiveMode; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif

		// Cache Mode Border Color
		MenuBuilder.AddWidget(
			LocalWidgetHelpers::CreateColorWidget(&OasisContentBrowserSetting->CacheModeBorderColor, FOnColorPicked::CreateLambda([this, OasisContentBrowserSetting] {
				OasisContentBrowserSetting->PostEditChange();
			}))
			, LOCTEXT("CacheModeBorderColor", "Cache Mode Border Color"));

		MenuBuilder.AddSeparator();

		// Purge Cache DB
#if ECB_FOLD		
		MenuBuilder.AddMenuEntry(
			LOCTEXT("PurgeCacheDB", "Purge and Save Cache"),
			LOCTEXT("PurgeCacheDBTooltip", "Purge unused assets data in the cache db."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this] { SOasisContentBrowser::HandlePurgeCacheDBClicked(); })
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);
#endif

#if 0
		TSharedRef<SWidget> CacheDBFilePathPicker = SNew(SFilePathPicker)
			.BrowseButtonImage(FAppStyle::GetBrush("PropertyWindow.Button_Ellipsis"))
			.BrowseButtonStyle(FAppStyle::Get(), "HoverHintOnly")
			.BrowseButtonToolTip(LOCTEXT("FilePathBrowseButtonToolTip", "Choose a file to uase as SpyEngine Cache Database"))
			.BrowseDirectory(FPaths::ProjectSavedDir() )
			.FilePath_Lambda([this, OasisContentBrowserSetting]() -> FString
				{
					return OasisContentBrowserSetting->CacheDBFilePath.FilePath;
				})
			.FileTypeFilter_Lambda([]() -> FString
				{
					return TEXT("All files (*.*)|*.*|CacheDB files (*.cachedb)|*.cachedb");
				})
			.OnPathPicked(this, &SOasisContentBrowser::HandleCacheDBFilePathPicked)
			.ToolTipText(LOCTEXT("FilePathToolTip", "The path to SpyEngine Cache Database file"));

			MenuBuilder.AddWidget(CacheDBFilePathPicker, LOCTEXT("CacheDBFilePath", "CacheDB File"));
#endif
	}
	MenuBuilder.EndSection();
#endif

#if ECB_WIP_OBJECT_THUMB_POOL
	MenuBuilder.BeginSection("CacheDB", LOCTEXT("ThumbnailPoolHeading", "Thumbnail Pool"));
	{
		// Use Thumbnail Pool
		MenuBuilder.AddMenuEntry(
			LOCTEXT("UseThumbnailPool", "Use Thumbnail Pool"),
			LOCTEXT("UseThumbnailPoolTooltip", "Use thumbnial pool in memeory to store thumbnail data."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting]
				{
					bool bWasUseThumbnailPool = OasisContentBrowserSetting->bUseThumbnailPool;
					OasisContentBrowserSetting->bUseThumbnailPool = !OasisContentBrowserSetting->bUseThumbnailPool;
					OasisContentBrowserSetting->PostEditChange();

					if (OasisContentBrowserSetting->bUseThumbnailPool) // now use thumbnail pool
					{
						FOasisContentBrowserSingleton::GetThumbnailPool().Resize(OasisContentBrowserSetting->NumThumbnailsInPool);
					}
					else // turn off thumbnail pool
					{
						FOasisContentBrowserSingleton::GetThumbnailPool().Free();
					}
				}),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bUseThumbnailPool; })
				),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		// Cache Mode Border Color
		MenuBuilder.AddWidget(
			LocalWidgetHelpers::CreateSpinBox<int32>(&OasisContentBrowserSetting->NumThumbnailsInPool, 1, 1024 * 1024, FOnSpintBoxVauleComitted::CreateLambda([this, OasisContentBrowserSetting] {
				if (OasisContentBrowserSetting->bUseThumbnailPool) // now use thumbnail pool
				{
					FOasisContentBrowserSingleton::GetThumbnailPool().Resize(OasisContentBrowserSetting->NumThumbnailsInPool);
				}
				OasisContentBrowserSetting->PostEditChange();
				}))
			, LOCTEXT("NumThumbnailsInPool", "Max Num Thumbnails in Pool"));
	}
	MenuBuilder.EndSection();
#endif

	MenuBuilder.BeginSection("Import", LOCTEXT("ImportHeading", "Import"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SkipImportIfAnyDependencyMissing", "Abort If Dependency Missing"),
			LOCTEXT("SkipImportIfAnyDependencyMissingTooltip", "Abort importing if any dependency of selected uasset file is missing"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting]	{ OasisContentBrowserSetting->bSkipImportIfAnyDependencyMissing = !OasisContentBrowserSetting->bSkipImportIfAnyDependencyMissing; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bSkipImportIfAnyDependencyMissing; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

#if ECB_FEA_IMPORT_IGNORE_SOFT_ERROR
		MenuBuilder.AddMenuEntry(
			LOCTEXT("IgnoreSoftReferencesError", "Ignore Soft References Error"),
			LOCTEXT("IgnoreSoftReferencesErrorTooltip", "Always ignore missing or invalid soft references when importing"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bImportIgnoreSoftReferencesError = !OasisContentBrowserSetting->bImportIgnoreSoftReferencesError; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bImportIgnoreSoftReferencesError; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif

		MenuBuilder.AddMenuEntry(
			LOCTEXT("OverwriteExistingFiles", "Overwrite Existing Assets"),
			LOCTEXT("OverwriteExistingFilesTooltip", "Overwrite existing files when importing uasset and its dependencies or use existing files instead"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bImportOverwriteExistingFiles = !OasisContentBrowserSetting->bImportOverwriteExistingFiles; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bImportOverwriteExistingFiles; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("RollbackIfFailed", "Rollback If Failed"),
			LOCTEXT("RollbackIfFailedTooltip", "Delete imported uasset and dependencies and restore any replaced files if import failed"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bRollbackImportIfFailed = !OasisContentBrowserSetting->bRollbackImportIfFailed; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bRollbackImportIfFailed; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddSeparator();

#if ECB_WIP_IMPORT_FOLDER_COLOR
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ImportFolderColor", "Import Folder Color"),
			LOCTEXT("ImportFolderColorTooltip", "Import folder color definiation to current project"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bImportFolderColor = !OasisContentBrowserSetting->bImportFolderColor; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bImportFolderColor; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#if ECB_WIP_IMPORT_FOLDER_COLOR_OVERRIDE
		MenuBuilder.AddMenuEntry(
			LOCTEXT("OverrideExistingFolderColor", "Override Existing Folder Color"),
			LOCTEXT("OverrideExistingFolderColorTooltip", "Wheter override existing folder color or skip import folder color when exist"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bOverrideExistingFolderColor = !OasisContentBrowserSetting->bOverrideExistingFolderColor; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bOverrideExistingFolderColor; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
#endif
#endif

		MenuBuilder.AddSeparator();

#if ECB_WIP_IMPORT_TO_PLUGIN_FOLDER
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ImportToPlugin", "Import to Plugin"),
			LOCTEXT("ImportToPluginTooltip", "Import to project's plugin content folder"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bImportToPluginFolder = !OasisContentBrowserSetting->bImportToPluginFolder; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bImportToPluginFolder; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);


		MenuBuilder.AddMenuEntry(
			LOCTEXT("WarnBeforeImportToPluginFolder", "Ask Before Import to Plugin"),
			LOCTEXT("WarnBeforeImportToPluginFolderTooltip", "Always show confirmation dialog before importing to a plugin"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bWarnBeforeImportToPluginFolder = !OasisContentBrowserSetting->bWarnBeforeImportToPluginFolder; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bImportToPluginFolder; }),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bWarnBeforeImportToPluginFolder; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		TSharedRef<SWidget> ImportToPluginName = SNew(SEditableTextBox)
			.Text_Lambda([this, OasisContentBrowserSetting]()
			{
				return FText::FromName(OasisContentBrowserSetting->ImportToPluginName);
			})
				.IsEnabled_Lambda([this, OasisContentBrowserSetting]()
			{
				return OasisContentBrowserSetting->bImportToPluginFolder;
			})
			.OnTextCommitted_Lambda([this, OasisContentBrowserSetting](const FText& InText, ETextCommit::Type)
			{
				if (!InText.IsEmpty())
				{
					OasisContentBrowserSetting->ImportToPluginName = *InText.ToString();
				}
			});

		MenuBuilder.AddWidget(ImportToPluginName, LOCTEXT("PluginName", "Plugin Name"));
#endif

	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("PostImport", LOCTEXT("PostImportHeading", "Post Import"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SyncAssetsInContentBrowser", "Sync Improted Assets In Content Browser"),
			LOCTEXT("SyncAssetsInContentBrowserTooltip", "Find imported assets in Content Browser"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bImportSyncAssetsInContentBrowser = !OasisContentBrowserSetting->bImportSyncAssetsInContentBrowser; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bImportSyncAssetsInContentBrowser; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("SyncExistingAssets", "Sync Skipped Existing Assets"),
			LOCTEXT("SyncExistingAssetsTooltip", "Find existing assets skipped for import in Content Browser, by default only find new imported assets"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bImportSyncExistingAssets = !OasisContentBrowserSetting->bImportSyncExistingAssets; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bImportSyncAssetsInContentBrowser; }),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bImportSyncExistingAssets; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("LoadAssetAfterImport", "Load/Reload Assets After Import"),
			LOCTEXT("LoadAssetAfterImportTooltip", "Load or reload import assets into memory immediately after import"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bLoadAssetAfterImport = !OasisContentBrowserSetting->bLoadAssetAfterImport; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bLoadAssetAfterImport; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

#if ECB_WIP_IMPORT_ADD_TO_COLLECTION
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AddImportedToCollection", "Add to Collection"),
			LOCTEXT("AddImportedToCollectionTooltip", "Add imported assets and dependencies into collection"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bAddImportedAssetsToCollection = !OasisContentBrowserSetting->bAddImportedAssetsToCollection; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bAddImportedAssetsToCollection; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("UniqueCollectionForEachImport", "Unique Collection for Each Import"),
			LOCTEXT("UniqueCollectionForEachImportTooltip", "Make a unique collection for each import session"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bUniqueCollectionNameForEachImportSession = !OasisContentBrowserSetting->bUniqueCollectionNameForEachImportSession; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bAddImportedAssetsToCollection; }),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bUniqueCollectionNameForEachImportSession; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		TSharedRef<SWidget> ImportedCollectionName = SNew(SEditableTextBox)
			.Text_Lambda([this, OasisContentBrowserSetting]()
			{
				return FText::FromName(OasisContentBrowserSetting->DefaultImportedUAssetCollectionName);
			})
			.IsEnabled_Lambda([this, OasisContentBrowserSetting]()
			{
				return OasisContentBrowserSetting->bAddImportedAssetsToCollection;
			})
			.OnTextCommitted_Lambda([this, OasisContentBrowserSetting](const FText& InText, ETextCommit::Type)
			{
				if (!InText.IsEmpty())
				{
					OasisContentBrowserSetting->DefaultImportedUAssetCollectionName = *InText.ToString();
				}
			});

		MenuBuilder.AddWidget(ImportedCollectionName, LOCTEXT("Collection", "Collection"));
#endif
	}
	MenuBuilder.EndSection();

#if ECB_WIP_EXPORT
	MenuBuilder.BeginSection("Export", LOCTEXT("ExportHeading", "Export"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SkipExportIfAnyDependencyMissing", "Abort If Dependency Missing"),
			LOCTEXT("SkipExportIfAnyDependencyMissingTooltip", "Abort exporting if any dependency of selected uasset file is missing"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bSkipExportIfAnyDependencyMissing = !OasisContentBrowserSetting->bSkipExportIfAnyDependencyMissing; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bSkipExportIfAnyDependencyMissing; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("IgnoreSoftReferencesError", "Ignore Soft References Error"),
			LOCTEXT("ExportIgnoreSoftReferencesErrorTooltip", "Always ignore missing or invalid soft references when exporting"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bExportIgnoreSoftReferencesError = !OasisContentBrowserSetting->bExportIgnoreSoftReferencesError; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bExportIgnoreSoftReferencesError; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("OverwriteExistingFiles", "Overwrite Existing Assets"),
			LOCTEXT("ExportOverwriteExistingFilesTooltip", "Overwrite existing files when exporting uasset and its dependencies or use existing files instead"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bExportOverwriteExistingFiles = !OasisContentBrowserSetting->bExportOverwriteExistingFiles; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bExportOverwriteExistingFiles; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("RollbackIfFailed", "Rollback If Failed"),
			LOCTEXT("ExportRollbackIfFailedTooltip", "Delete exported uasset and dependencies and restore any replaced files if export failed"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bRollbackExportIfFailed = !OasisContentBrowserSetting->bRollbackExportIfFailed; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bRollbackExportIfFailed; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("OpenFolderAfterExport", "Open Folder After Export"),
			LOCTEXT("OpenFolderAfterExportTooltip", "Open selected folder after export"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->bOpenFolderAfterExport = !OasisContentBrowserSetting->bOpenFolderAfterExport; OasisContentBrowserSetting->PostEditChange(); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, OasisContentBrowserSetting] { return OasisContentBrowserSetting->bOpenFolderAfterExport; })
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
	}
	MenuBuilder.EndSection();
#endif

#if ECB_DEBUG
	MenuBuilder.BeginSection("Debug", LOCTEXT("DebugHeading", "Debug"));
	{
		// Print Cache
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Print Cache", "Print Cache"),
			LOCTEXT("Print Cache", "Print Cache"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this] { HandlePrintCacheClicked(); })
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);
		// Clear Cache
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Clear Cache", "Clear Cache"),
			LOCTEXT("Clear Cache", "Clear Cache"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this] { HandleClearCacheClicked(); })
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);
		// Print AssetData
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Print AssetData", "Print AssetData"),
			LOCTEXT("Print AssetData", "Print AssetData"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this] { HandlePrintAssetDataClicked(); })
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);
	}
	MenuBuilder.EndSection();
#endif

	MenuBuilder.BeginSection("Reset", LOCTEXT("ResetHeading", "Reset"));
	{
		MenuBuilder.AddMenuEntry(
#if ECB_WIP_EXPORT
			LOCTEXT("ResetImportExportOptions", "Reset Import/Export Options"),
			LOCTEXT("ResetImportExportOptionsTooltip", "Reset import and export options to default"),
#else
			LOCTEXT("ResetImportOptions", "Reset Import Options"),
			LOCTEXT("ResetImportOptionsTooltip", "Reset import options to default"),
#endif
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, OasisContentBrowserSetting] { OasisContentBrowserSetting->ResetImportSettings(); OasisContentBrowserSetting->PostEditChange(); })
				//FCanExecuteAction(),
				//FIsActionChecked::CreateLambda([this] { return false; })
			),
			NAME_None,
			EUserInterfaceActionType::Button
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}
#endif

#if ECB_LEGACY
void SOasisContentBrowser::OnAddContentRequested()
{
	IAddContentDialogModule& AddContentDialogModule = FModuleManager::LoadModuleChecked<IAddContentDialogModule>("AddContentDialog");
	FWidgetPath WidgetPath;
	FSlateApplication::Get().GeneratePathToWidgetChecked(AsShared(), WidgetPath);
	AddContentDialogModule.ShowDialog(WidgetPath.GetWindow());
}
#endif

void SOasisContentBrowser::RemoveContentFolders()
{
	PathContextMenu->ExecuteRemoveRootFolder();
}

bool SOasisContentBrowser::CanRemoveContentFolders() const
{
	return bCanRemoveContentFolders && PathViewPtr->GetNumPathsSelected() > 0;
}

void SOasisContentBrowser::OnAssetSelectionChanged(const FOasisAssetData& SelectedAsset)
{
	// Notify 'asset selection changed' delegate
	FOasisContentBrowserModule& OasisContentBrowserModule = FModuleManager::GetModuleChecked<FOasisContentBrowserModule>( TEXT("OasisContentBrowser") );
	FOnExtAssetSelectionChanged& AssetSelectionChangedDelegate = OasisContentBrowserModule.GetOnAssetSelectionChanged();
	
	const TArray<FOasisAssetData>& SelectedAssets = AssetViewPtr->GetSelectedAssets();
	AssetContextMenu->SetSelectedAssets(SelectedAssets);
	//CollectionViewPtr->SetSelectedAssets(SelectedAssets);

	const bool bReferenceViewerLocked = false;
	if (bReferenceViewerExpanded && !bReferenceViewerLocked)
	{
		TArray<FExtAssetIdentifier> AssetIdentifiers;
		FExtAssetRegistry::ExtractAssetIdentifiersFromAssetDataList(SelectedAssets, AssetIdentifiers);

		// Validate to cache dependency asset data info
// 		for (const FOasisAssetData& AssetData : SelectedAssets)
// 		{
// 			FExtAssetValidator::ValidateDependency(AssetData);
// 		}
		EDependencyNodeStatus Status;
		FExtAssetValidator::ValidateDependency(SelectedAssets, nullptr, /*bShowProgess*/ true, &Status);
		if (Status != EDependencyNodeStatus::AbortGathering)
		{
			ReferenceViewerPtr->SetGraphRootIdentifiers(AssetIdentifiers);
		}
	}

	if(AssetSelectionChangedDelegate.IsBound())
	{
		AssetSelectionChangedDelegate.Broadcast(SelectedAssets, bIsPrimaryBrowser);
	}
}

#if ECB_LEGACY
void SOasisContentBrowser::OnAssetsActivated(const TArray<FAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationMethod)
{
	TMap< TSharedRef<IAssetTypeActions>, TArray<UObject*> > TypeActionsToObjects;
	TArray<UObject*> ObjectsWithoutTypeActions;

	const FText LoadingTemplate = LOCTEXT("LoadingAssetName", "Loading {0}...");
	const FText DefaultText = ActivatedAssets.Num() == 1 ? FText::Format(LoadingTemplate, FText::FromName(ActivatedAssets[0].AssetName)) : LOCTEXT("LoadingObjects", "Loading Objects...");
	FScopedSlowTask SlowTask(100, DefaultText);

	// Iterate over all activated assets to map them to AssetTypeActions.
	// This way individual asset type actions will get a batched list of assets to operate on
	for ( auto AssetIt = ActivatedAssets.CreateConstIterator(); AssetIt; ++AssetIt )
	{
		const FAssetData& AssetData = *AssetIt;
		if (!AssetData.IsAssetLoaded() && FEditorFileUtils::IsMapPackageAsset(AssetData.ObjectPath.ToString()))
		{
			SlowTask.MakeDialog();
		}

		SlowTask.EnterProgressFrame(75.f/ActivatedAssets.Num(), FText::Format(LoadingTemplate, FText::FromName(AssetData.AssetName)));

		UObject* Asset = (*AssetIt).GetAsset();

		if ( Asset != NULL )
		{
			FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
			TWeakPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(Asset->GetClass());
			if ( AssetTypeActions.IsValid() )
			{
				// Add this asset to the list associated with the asset type action object
				TArray<UObject*>& ObjList = TypeActionsToObjects.FindOrAdd(AssetTypeActions.Pin().ToSharedRef());
				ObjList.AddUnique(Asset);
			}
			else
			{
				ObjectsWithoutTypeActions.AddUnique(Asset);
			}
		}
	}

	// Now that we have created our map, activate all the lists of objects for each asset type action.
	for ( auto TypeActionsIt = TypeActionsToObjects.CreateConstIterator(); TypeActionsIt; ++TypeActionsIt )
	{
		SlowTask.EnterProgressFrame(25.f/TypeActionsToObjects.Num());

		const TSharedRef<IAssetTypeActions>& TypeActions = TypeActionsIt.Key();
		const TArray<UObject*>& ObjList = TypeActionsIt.Value();

		TypeActions->AssetsActivated(ObjList, ActivationMethod);
	}

	// Finally, open a simple asset editor for all assets which do not have asset type actions if activating with enter or double click
	if ( ActivationMethod == EAssetTypeActivationMethod::DoubleClicked || ActivationMethod == EAssetTypeActivationMethod::Opened )
	{
		ContentBrowserUtils::OpenEditorForAsset(ObjectsWithoutTypeActions);
	}
}
#endif

TSharedPtr<SWidget> SOasisContentBrowser::OnGetAssetContextMenu(const TArray<FOasisAssetData>& SelectedAssets)
{
	if ( SelectedAssets.Num() == 0 )
	{
		return MakeAddNewContextMenu( false, true );
	}
	else
	{
		return AssetContextMenu->MakeContextMenu(SelectedAssets, AssetViewPtr->GetSourcesData(), Commands);
	}
}

#if ECB_WIP_LOCK

FReply SOasisContentBrowser::ToggleLockClicked()
{
	bIsLocked = !bIsLocked;

	return FReply::Handled();
}

const FSlateBrush* SOasisContentBrowser::GetToggleLockImage() const
{
	if ( bIsLocked )
	{
		return FAppStyle::GetBrush("ContentBrowser.LockButton_Locked");
	}
	else
	{
		return FAppStyle::GetBrush("ContentBrowser.LockButton_Unlocked");
	}
}

#endif

EVisibility SOasisContentBrowser::GetVisibilityByLastFrameWidth(float MinWidthToShow) const
{
#if ECB_FEA_TOOLBAR_BUTTON_AUTOHIDE
	return (WidthLastFrame < MinWidthToShow) ? EVisibility::Collapsed : EVisibility::Visible;
#else
	return EVisibility::Visible;
#endif
}

EVisibility SOasisContentBrowser::GetSourcesViewVisibility() const
{
	return bSourcesViewExpanded ? EVisibility::Visible : EVisibility::Collapsed;
}

const FSlateBrush* SOasisContentBrowser::GetSourcesToggleImage() const
{
	if ( bSourcesViewExpanded )
	{
		return FOasisContentBrowserStyle::Get().GetBrush("SpyEngine.HideSourcesView");
	}
	else
	{
		return FOasisContentBrowserStyle::Get().GetBrush("SpyEngine.ShowSourcesView");
	}
}

FReply SOasisContentBrowser::SourcesViewExpandClicked()
{
	bSourcesViewExpanded = !bSourcesViewExpanded;

#if ECB_LEGACY
	// Notify 'Sources View Expanded' delegate
	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>( TEXT("ContentBrowser") );
	FContentBrowserModule::FOnSourcesViewChanged& SourcesViewChangedDelegate = ContentBrowserModule.GetOnSourcesViewChanged();
	if(SourcesViewChangedDelegate.IsBound())
	{
		SourcesViewChangedDelegate.Broadcast(bSourcesViewExpanded);
	}
#endif

	return FReply::Handled();
}

EVisibility SOasisContentBrowser::GetPathExpanderVisibility() const
{
	return bSourcesViewExpanded ? EVisibility::Collapsed : EVisibility::Visible;
}

#if ECB_FEA_REF_VIEWER
EVisibility SOasisContentBrowser::GetReferencesViewerVisibility() const
{
	return bReferenceViewerExpanded ? EVisibility::Visible : EVisibility::Collapsed;
}

const FSlateBrush* SOasisContentBrowser::GetDependencyViewerToggleImage() const
{
	if (bReferenceViewerExpanded)
	{
		return FOasisContentBrowserStyle::Get().GetBrush("SpyEngine.ShowDependencyViewer");

	}
	else
	{
		return FOasisContentBrowserStyle::Get().GetBrush("SpyEngine.HideDependencyViewer");
	}
}

FReply SOasisContentBrowser::ReferenceViewerExpandClicked()
{
	bReferenceViewerExpanded = !bReferenceViewerExpanded;

	if (bReferenceViewerExpanded)
	{
		const TArray<FOasisAssetData>& SelectedAssets = AssetViewPtr->GetSelectedAssets();
		TArray<FExtAssetIdentifier> AssetIdentifiers;
		FExtAssetRegistry::ExtractAssetIdentifiersFromAssetDataList(SelectedAssets, AssetIdentifiers);

		EDependencyNodeStatus Status;
		FExtAssetValidator::ValidateDependency(SelectedAssets, nullptr, /*bShowProgess*/ true, &Status);
		if (Status != EDependencyNodeStatus::AbortGathering)
		{
			ReferenceViewerPtr->SetGraphRootIdentifiers(AssetIdentifiers);
		}
	}

	return FReply::Handled();
}
#endif

#if ECB_WIP_HISTORY
FReply SOasisContentBrowser::BackClicked()
{
	HistoryManager.GoBack();

	return FReply::Handled();
}

FReply SOasisContentBrowser::ForwardClicked()
{
	HistoryManager.GoForward();

	return FReply::Handled();
}

bool SOasisContentBrowser::IsBackEnabled() const
{
	return HistoryManager.CanGoBack();
}

bool SOasisContentBrowser::IsForwardEnabled() const
{
	return HistoryManager.CanGoForward();
}

FText SOasisContentBrowser::GetHistoryBackTooltip() const
{
	if (HistoryManager.CanGoBack())
	{
		return FText::Format(LOCTEXT("HistoryBackTooltipFmt", "Back to {0}"), HistoryManager.GetBackDesc());
	}
	return FText::GetEmpty();
}

FText SOasisContentBrowser::GetHistoryForwardTooltip() const
{
	if (HistoryManager.CanGoForward())
	{
		return FText::Format(LOCTEXT("HistoryForwardTooltipFmt", "Forward to {0}"), HistoryManager.GetForwardDesc());
	}
	return FText::GetEmpty();
}

#endif

#if ECB_LEGACY
void SOasisContentBrowser::HandleSaveAssetCommand()
{
	const TArray<TSharedPtr<FExtAssetViewItem>>& SelectedItems = AssetViewPtr->GetSelectedItems();
	if (SelectedItems.Num() > 0)
	{
		AssetContextMenu->ExecuteSaveAsset();
	}
}

void SOasisContentBrowser::HandleOpenAssetsOrFoldersCommandExecute()
{
	AssetViewPtr->OnOpenAssetsOrFolders();
}

void SOasisContentBrowser::HandlePreviewAssetsCommandExecute()
{
	AssetViewPtr->OnPreviewAssets();
}

void SOasisContentBrowser::HandleDirectoryUpCommandExecute()
{
	TArray<FString> SelectedPaths = PathViewPtr->GetSelectedPaths();
	if(SelectedPaths.Num() == 1 && !ContentBrowserUtils::IsRootDir(SelectedPaths[0]))
	{
		FString ParentDir = SelectedPaths[0] / TEXT("..");
		FPaths::CollapseRelativeDirectories(ParentDir);
		FolderEntered(ParentDir);
	}
}

void SOasisContentBrowser::GetSelectionState(TArray<FAssetData>& SelectedAssets, TArray<FString>& SelectedPaths)
{
	SelectedAssets.Reset();
	SelectedPaths.Reset();
	if (AssetViewPtr->HasAnyUserFocusOrFocusedDescendants())
	{
		SelectedAssets = AssetViewPtr->GetSelectedAssets();
		SelectedPaths = AssetViewPtr->GetSelectedFolders();
	}
	else if (PathViewPtr->HasAnyUserFocusOrFocusedDescendants())
	{
		SelectedPaths = PathViewPtr->GetSelectedPaths();
	}
}



bool SOasisContentBrowser::CanExecuteDirectoryUp() const
{
	TArray<FString> SelectedPaths = PathViewPtr->GetSelectedPaths();
	return (SelectedPaths.Num() == 1 && !ContentBrowserUtils::IsRootDir(SelectedPaths[0]));
}

FText SOasisContentBrowser::GetDirectoryUpTooltip() const
{
	TArray<FString> SelectedPaths = PathViewPtr->GetSelectedPaths();
	if(SelectedPaths.Num() == 1 && !ContentBrowserUtils::IsRootDir(SelectedPaths[0]))
	{
		FString ParentDir = SelectedPaths[0] / TEXT("..");
		FPaths::CollapseRelativeDirectories(ParentDir);
		return FText::Format(LOCTEXT("DirectoryUpTooltip", "Up to {0}"), FText::FromString(ParentDir) );
	}

	return FText();
}

EVisibility SOasisContentBrowser::GetDirectoryUpToolTipVisibility() const
{
	EVisibility ToolTipVisibility = EVisibility::Collapsed;

	// if we have text in our tooltip, make it visible. 
	if(GetDirectoryUpTooltip().IsEmpty() == false)
	{
		ToolTipVisibility = EVisibility::Visible;
	}

	return ToolTipVisibility;
}

#endif

void SOasisContentBrowser::HandleImportSelectedCommandExecute()
{
	if (IsImportEnabled())
	{
		HandleImportClicked();
	}
}


void SOasisContentBrowser::HandleToggleDependencyViewerCommandExecute()
{
	ReferenceViewerExpandClicked();
}

#if ECB_WIP_BREADCRUMB
void SOasisContentBrowser::UpdatePath()
{
	FSourcesData SourcesData = AssetViewPtr->GetSourcesData();

	PathBreadcrumbTrail->ClearCrumbs();

	if ( SourcesData.HasPackagePaths() )
	{
		TArray<FString> Crumbs;
		SourcesData.PackagePaths[0].ToString().ParseIntoArray(Crumbs, TEXT("/"), true);

		FString CrumbPath = TEXT("/");
		for ( auto CrumbIt = Crumbs.CreateConstIterator(); CrumbIt; ++CrumbIt )
		{
			// If this is the root part of the path, try and get the localized display name to stay in sync with what we see in SOasisPathView
			const FText DisplayName = (CrumbIt.GetIndex() == 0) ? OasisContentBrowserUtils::GetRootDirDisplayName(*CrumbIt) : FText::FromString(*CrumbIt);

			CrumbPath += *CrumbIt;
			PathBreadcrumbTrail->PushCrumb(DisplayName, CrumbPath);
			CrumbPath += TEXT("/");
		}
	}
	else if ( SourcesData.HasCollections() )
	{
		FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
		TArray<FCollectionNameType> CollectionPathItems;

		// Walk up the parents of this collection so that we can generate a complete path (this loop also adds the child collection to the array)
		for (TOptional<FCollectionNameType> CurrentCollection = SourcesData.Collections[0]; 
			CurrentCollection.IsSet(); 
			CurrentCollection = CollectionManagerModule.Get().GetParentCollection(CurrentCollection->Name, CurrentCollection->Type)
			)
		{
			CollectionPathItems.Insert(CurrentCollection.GetValue(), 0);
		}

		// Now add each part of the path to the breadcrumb trail
		for (const FCollectionNameType& CollectionPathItem : CollectionPathItems)
		{
			const FString CrumbData = FString::Printf(TEXT("%s?%s"), *CollectionPathItem.Name.ToString(), *FString::FromInt(CollectionPathItem.Type));

			FFormatNamedArguments Args;
			Args.Add(TEXT("CollectionName"), FText::FromName(CollectionPathItem.Name));
			const FText DisplayName = FText::Format(LOCTEXT("CollectionPathIndicator", "{CollectionName} (Collection)"), Args);

			PathBreadcrumbTrail->PushCrumb(DisplayName, CrumbData);
		}
	}
	else
	{
		PathBreadcrumbTrail->PushCrumb(LOCTEXT("AllAssets", "All Assets"), TEXT(""));
	}
}
#endif

void SOasisContentBrowser::OnFilterChanged()
{
	FARFilter Filter = FilterListPtr->GetCombinedBackendFilter();
	AssetViewPtr->SetBackendFilter(Filter);
#if ECB_LEGACY
	// Notify 'filter changed' delegate
	FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>( TEXT("ContentBrowser") );
	ContentBrowserModule.GetOnFilterChanged().Broadcast(Filter, bIsPrimaryBrowser);
#endif
}

#if ECB_LEGACY
FText SOasisContentBrowser::GetPathText() const
{
	FText PathLabelText;

	if ( IsFilteredBySource() )
	{
		const FSourcesData& SourcesData = AssetViewPtr->GetSourcesData();

		// At least one source is selected
		const int32 NumSources = SourcesData.PackagePaths.Num() + SourcesData.Collections.Num();

		if (NumSources > 0)
		{
			PathLabelText = FText::FromName(SourcesData.HasPackagePaths() ? SourcesData.PackagePaths[0] : SourcesData.Collections[0].Name);

			if (NumSources > 1)
			{
				PathLabelText = FText::Format(LOCTEXT("PathTextFmt", "{0} and {1} {1}|plural(one=other,other=others)..."), PathLabelText, NumSources - 1);
			}
		}
	}
	else
	{
		PathLabelText = LOCTEXT("AllAssets", "All Assets");
	}

	return PathLabelText;
}

bool SOasisContentBrowser::IsFilteredBySource() const
{
	const FSourcesData& SourcesData = AssetViewPtr->GetSourcesData();
	return !SourcesData.IsEmpty();
}

void SOasisContentBrowser::OnAssetRenameCommitted(const TArray<FAssetData>& Assets)
{
	// After a rename is committed we allow an implicit sync so as not to
	// disorientate the user if they are looking at a parent folder

	const bool bAllowImplicitSync = true;
	const bool bDisableFiltersThatHideAssets = false;
	SyncToAssets(Assets, bAllowImplicitSync, bDisableFiltersThatHideAssets);
}
#endif

#if ECB_WIP_SYNC_ASSET
void SOasisContentBrowser::OnFindInAssetTreeRequested(const TArray<FAssetData>& AssetsToFind)
{
	SyncToAssets(AssetsToFind);
}
#endif

#if ECB_LEGACY

void SOasisContentBrowser::OnRenameRequested(const FAssetData& AssetData)
{
	AssetViewPtr->RenameAsset(AssetData);
}

void SOasisContentBrowser::OnRenameFolderRequested(const FString& FolderToRename)
{
	const TArray<FString>& SelectedFolders = AssetViewPtr->GetSelectedFolders();
	if (SelectedFolders.Num() > 0)
	{
		AssetViewPtr->RenameFolder(FolderToRename);
	}
	else
	{
		const TArray<FString>& SelectedPaths = PathViewPtr->GetSelectedPaths();
		if (SelectedPaths.Num() > 0)
		{
			PathViewPtr->RenameFolder(FolderToRename);
		}
	}
}

void SOasisContentBrowser::OnOpenedFolderDeleted()
{
	// Since the contents of the asset view have just been deleted, set the selected path to the default "/Game"
	TArray<FString> DefaultSelectedPaths;
	DefaultSelectedPaths.Add(TEXT("/Game"));
	PathViewPtr->SetSelectedPaths(DefaultSelectedPaths);
	FavoritePathViewPtr->SetSelectedPaths(DefaultSelectedPaths);
	FSourcesData DefaultSourcesData(FName("/Game"));
	AssetViewPtr->SetSourcesData(DefaultSourcesData);

	UpdatePath();
}

void SOasisContentBrowser::OnAssetViewRefreshRequested()
{
	AssetViewPtr->RequestSlowFullListRefresh();
}

void SOasisContentBrowser::HandleCollectionRemoved(const FCollectionNameType& Collection)
{
	AssetViewPtr->SetSourcesData(FSourcesData());

	auto RemoveHistoryDelegate = [&](const FHistoryData& HistoryData)
	{
		return (HistoryData.SourcesData.Collections.Num() == 1 &&
				HistoryData.SourcesData.PackagePaths.Num() == 0 &&
				HistoryData.SourcesData.Collections.Contains(Collection));
	};

	HistoryManager.RemoveHistoryData(RemoveHistoryDelegate);
}

void SOasisContentBrowser::HandleCollectionRenamed(const FCollectionNameType& OriginalCollection, const FCollectionNameType& NewCollection)
{
	return HandleCollectionRemoved(OriginalCollection);
}

void SOasisContentBrowser::HandleCollectionUpdated(const FCollectionNameType& Collection)
{
	const FSourcesData& SourcesData = AssetViewPtr->GetSourcesData();

	// If we're currently viewing the dynamic collection that was updated, make sure our active filter text is up-to-date
	if (SourcesData.IsDynamicCollection() && SourcesData.Collections[0] == Collection)
	{
		FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

		const FCollectionNameType& DynamicCollection = SourcesData.Collections[0];

		FString DynamicQueryString;
		CollectionManagerModule.Get().GetDynamicQueryText(DynamicCollection.Name, DynamicCollection.Type, DynamicQueryString);

		const FText DynamicQueryText = FText::FromString(DynamicQueryString);
		SetSearchBoxText(DynamicQueryText);
		SearchBoxPtr->SetText(DynamicQueryText);
	}
}

void SOasisContentBrowser::HandlePathRemoved(const FString& Path)
{
	const FName PathName(*Path);

	auto RemoveHistoryDelegate = [&](const FHistoryData& HistoryData)
	{
		return (HistoryData.SourcesData.PackagePaths.Num() == 1 &&
				HistoryData.SourcesData.Collections.Num() == 0 &&
				HistoryData.SourcesData.PackagePaths.Contains(PathName));
	};

	HistoryManager.RemoveHistoryData(RemoveHistoryDelegate);
}
#endif
#if ECB_FEA_SEARCH
FText SOasisContentBrowser::GetSearchAssetsHintText() const
{
	if (PathViewPtr.IsValid())
	{
		TArray<FString> Paths = PathViewPtr->GetSelectedPaths();
		if (Paths.Num() != 0)
		{
			FString SearchHint = NSLOCTEXT( "ContentBrowser", "SearchBoxPartialHint", "Search" ).ToString();
			SearchHint += TEXT(" ");
			for(int32 i = 0; i < Paths.Num(); i++)
			{
				const FString& Path = Paths[i];
#if ECB_LEGACY
				if (ContentBrowserUtils::IsRootDir(Path))
				{
					SearchHint += ContentBrowserUtils::GetRootDirDisplayName(Path).ToString();
				}
				else
#endif
				{
					SearchHint += FPaths::GetCleanFilename(Path);
				}

				if (i + 1 < Paths.Num())
				{
					SearchHint += ", ";
				}
			}

			return FText::FromString(SearchHint);
		}
	}
	
	return NSLOCTEXT( "ContentBrowser", "SearchBoxHint", "Search Assets" );
}

void ExtractAssetSearchFilterTerms(const FText& SearchText, FString* OutFilterKey, FString* OutFilterValue, int32* OutSuggestionInsertionIndex)
{
	const FString SearchString = SearchText.ToString();

	if (OutFilterKey)
	{
		OutFilterKey->Reset();
	}
	if (OutFilterValue)
	{
		OutFilterValue->Reset();
	}
	if (OutSuggestionInsertionIndex)
	{
		*OutSuggestionInsertionIndex = SearchString.Len();
	}

	// Build the search filter terms so that we can inspect the tokens
	FTextFilterExpressionEvaluator LocalFilter(ETextFilterExpressionEvaluatorMode::Complex);
	LocalFilter.SetFilterText(SearchText);

	// Inspect the tokens to see what the last part of the search term was
	// If it was a key->value pair then we'll use that to control what kinds of results we show
	// For anything else we just use the text from the last token as our filter term to allow incremental auto-complete
	const TArray<FExpressionToken>& FilterTokens = LocalFilter.GetFilterExpressionTokens();
	if (FilterTokens.Num() > 0)
	{
		const FExpressionToken& LastToken = FilterTokens.Last();

		// If the last token is a text token, then consider it as a value and walk back to see if we also have a key
		if (LastToken.Node.Cast<TextFilterExpressionParser::FTextToken>())
		{
			if (OutFilterValue)
			{
				*OutFilterValue = LastToken.Context.GetString();
			}
			if (OutSuggestionInsertionIndex)
			{
				*OutSuggestionInsertionIndex = FMath::Min(*OutSuggestionInsertionIndex, LastToken.Context.GetCharacterIndex());
			}

			if (FilterTokens.IsValidIndex(FilterTokens.Num() - 2))
			{
				const FExpressionToken& ComparisonToken = FilterTokens[FilterTokens.Num() - 2];
				if (ComparisonToken.Node.Cast<TextFilterExpressionParser::FEqual>())
				{
					if (FilterTokens.IsValidIndex(FilterTokens.Num() - 3))
					{
						const FExpressionToken& KeyToken = FilterTokens[FilterTokens.Num() - 3];
						if (KeyToken.Node.Cast<TextFilterExpressionParser::FTextToken>())
						{
							if (OutFilterKey)
							{
								*OutFilterKey = KeyToken.Context.GetString();
							}
							if (OutSuggestionInsertionIndex)
							{
								*OutSuggestionInsertionIndex = FMath::Min(*OutSuggestionInsertionIndex, KeyToken.Context.GetCharacterIndex());
							}
						}
					}
				}
			}
		}

		// If the last token is a comparison operator, then walk back and see if we have a key
		else if (LastToken.Node.Cast<TextFilterExpressionParser::FEqual>())
		{
			if (FilterTokens.IsValidIndex(FilterTokens.Num() - 2))
			{
				const FExpressionToken& KeyToken = FilterTokens[FilterTokens.Num() - 2];
				if (KeyToken.Node.Cast<TextFilterExpressionParser::FTextToken>())
				{
					if (OutFilterKey)
					{
						*OutFilterKey = KeyToken.Context.GetString();
					}
					if (OutSuggestionInsertionIndex)
					{
						*OutSuggestionInsertionIndex = FMath::Min(*OutSuggestionInsertionIndex, KeyToken.Context.GetCharacterIndex());
					}
				}
			}
		}
	}
}

void SOasisContentBrowser::OnAssetSearchSuggestionFilter(const FText& SearchText, TArray<FAssetSearchBoxSuggestion>& PossibleSuggestions, FText& SuggestionHighlightText) const
{
	// We don't bind the suggestion list, so this list should be empty as we populate it here based on the search term
	check(PossibleSuggestions.Num() == 0);

	FString FilterKey;
	FString FilterValue;
	ExtractAssetSearchFilterTerms(SearchText, &FilterKey, &FilterValue, nullptr);

	auto PassesValueFilter = [&FilterValue](const FString& InOther)
	{
		return FilterValue.IsEmpty() || InOther.Contains(FilterValue);
	};

	if (FilterKey.IsEmpty() || (FilterKey == TEXT("Type") || FilterKey == TEXT("Class")))
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		TArray< TWeakPtr<IAssetTypeActions> > AssetTypeActionsList;
		AssetToolsModule.Get().GetAssetTypeActionsList(AssetTypeActionsList);

		const FText TypesCategoryName = NSLOCTEXT("ContentBrowser", "TypesCategoryName", "Types");
		for (auto TypeActionsIt = AssetTypeActionsList.CreateConstIterator(); TypeActionsIt; ++TypeActionsIt)
		{
			if ((*TypeActionsIt).IsValid())
			{
				const TSharedPtr<IAssetTypeActions> TypeActions = (*TypeActionsIt).Pin();
				if (TypeActions->GetSupportedClass())
				{
					const FString TypeName = TypeActions->GetSupportedClass()->GetName();
					const FText TypeDisplayName = TypeActions->GetSupportedClass()->GetDisplayNameText();
					FString TypeSuggestion = FString::Printf(TEXT("Type=%s"), *TypeName);
					if (PassesValueFilter(TypeSuggestion))
					{
						PossibleSuggestions.Add(FAssetSearchBoxSuggestion{ MoveTemp(TypeSuggestion), TypeDisplayName, TypesCategoryName });
					}
				}
			}
		}
	}
#if ECB_WIP_COLLECTION
	if (FilterKey.IsEmpty() || (FilterKey == TEXT("Collection") || FilterKey == TEXT("Tag")))
	{
		ICollectionManager& CollectionManager = FCollectionManagerModule::GetModule().Get();

		TArray<FCollectionNameType> AllCollections;
		CollectionManager.GetCollections(AllCollections);

		const FText CollectionsCategoryName = NSLOCTEXT("ContentBrowser", "CollectionsCategoryName", "Collections");
		for (const FCollectionNameType& Collection : AllCollections)
		{
			FString CollectionName = Collection.Name.ToString();
			FString CollectionSuggestion = FString::Printf(TEXT("Collection=%s"), *CollectionName);
			if (PassesValueFilter(CollectionSuggestion))
			{
				PossibleSuggestions.Add(FAssetSearchBoxSuggestion{ MoveTemp(CollectionSuggestion), FText::FromString(MoveTemp(CollectionName)), CollectionsCategoryName });
			}
		}
	}
#endif
#if ECB_TODO
	if (FilterKey.IsEmpty())
	{
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

		if (const FAssetRegistryState* StatePtr = AssetRegistry.GetAssetRegistryState())
		{
			const FText MetaDataCategoryName = NSLOCTEXT("ContentBrowser", "MetaDataCategoryName", "Meta-Data");
			for (const auto& TagAndArrayPair : StatePtr->GetTagToAssetDatasMap())
			{
				const FString TagNameStr = TagAndArrayPair.Key.ToString();
				if (PassesValueFilter(TagNameStr))
				{
					PossibleSuggestions.Add(FAssetSearchBoxSuggestion{ TagNameStr, FText::FromString(TagNameStr), MetaDataCategoryName });
				}
			}
		}
	}
#endif
	SuggestionHighlightText = FText::FromString(FilterValue);
}

FText SOasisContentBrowser::OnAssetSearchSuggestionChosen(const FText& SearchText, const FString& Suggestion) const
{
	int32 SuggestionInsertionIndex = 0;
	ExtractAssetSearchFilterTerms(SearchText, nullptr, nullptr, &SuggestionInsertionIndex);

	FString SearchString = SearchText.ToString();
	SearchString.RemoveAt(SuggestionInsertionIndex, SearchString.Len() - SuggestionInsertionIndex, false);
	SearchString.Append(Suggestion);

	return FText::FromString(SearchString);
}

#endif
TSharedPtr<SWidget> SOasisContentBrowser::GetFolderContextMenu(const TArray<FString>& SelectedPaths, FContentBrowserMenuExtender_SelectedPaths InMenuExtender, FOnCreateNewFolder InOnCreateNewFolder, bool bPathView)
{
	// Clear any selection in the asset view, as it'll conflict with other view info
	// This is important for determining which context menu may be open based on the asset selection for rename/delete operations
	if (bPathView)
	{
		AssetViewPtr->ClearSelection();
	}
	
	// Ensure the path context menu has the up-to-date list of paths being worked on
	PathContextMenu->SetSelectedPaths(SelectedPaths);

	TSharedPtr<FExtender> Extender;
	if(InMenuExtender.IsBound())
	{
		Extender = InMenuExtender.Execute(SelectedPaths);
	}

	const bool bInShouldCloseWindowAfterSelection = true;
	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterSelection, Commands, Extender, true);

// 	MenuBuilder.AddMenuEntry(
// 		LOCTEXT("AddContentFolder", "Add Content Folder"),
// 		LOCTEXT("ShowInNewContentBrowserTooltip", "Opens a new Content Browser at this folder location (at least 1 Content Browser window needs to be locked)"),
// 		FSlateIcon(),
// 		FUIAction()
// 	);

	if (SelectedPaths.Num() > 0)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ClearFolderSelection", "Clear Folder Selection"),
			LOCTEXT("ClearFolderSelectionTooltip", "Clear all folder selection."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SOasisContentBrowser::ClearFolderSelection)),
			"FolderContext"
		);
	}
	else
	{
		MenuBuilder.BeginSection("FolderContext", LOCTEXT("FolderOptionsMenuHeading", "Folder Options"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("AddContentFolder", "Add Content Folder"),
				LOCTEXT("AddContentFolderTooltip", "Add root content folder"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SOasisContentBrowser::AddContentFolder))
				//,"FolderContext"
			);
		}
		MenuBuilder.EndSection();
	}

	//MenuBuilder.AddMenuSeparator("FolderContext");

#if ECB_WIP_MULTI_INSTANCES
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ShowInNewContentBrowser", "Show in New Content Browser"),
		LOCTEXT("ShowInNewContentBrowserTooltip", "Opens a new Content Browser at this folder location (at least 1 Content Browser window needs to be locked)"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SOasisContentBrowser::OpenNewContentBrowser)),
		"FolderContext"
	);
#endif
	return MenuBuilder.MakeWidget();
	//return SNullWidget::NullWidget;
}

void SOasisContentBrowser::ClearFolderSelection()
{
	PathViewPtr->ClearSelection();

	AssetViewPtr->SetSourcesData(FSourcesData());

	bCanRemoveContentFolders = false;
}

#if ECB_WIP_CACHEDB
void SOasisContentBrowser::HandleCacheDBFilePathPicked(const FString& PickedPath)
{
	FString FinalPath = PickedPath;
	UOasisContentBrowserSettings* OasisContentBrowserSetting = GetMutableDefault<UOasisContentBrowserSettings>();
	ECB_LOG(Display, TEXT("HandleCacheDBFilePathPicked: was: %s, now: %s"), *OasisContentBrowserSetting->CacheFilePath.FilePath, *FinalPath);
}
#endif


#undef LOCTEXT_NAMESPACE

#ifdef EXT_DOC_NAMESPACE
#undef EXT_DOC_NAMESPACE
#endif
