// Innerloop Interactive Production :: c0r37py

#include "OasisContentBrowserStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

#include "DocumentationStyle.h"

TSharedPtr< FSlateStyleSet > FOasisContentBrowserStyle::StyleInstance = NULL;

FLinearColor FOasisContentBrowserStyle::CustomContentBrowserBorderBackgroundColor(.05f, 0.05f, 0.05f, 1.0f);
FLinearColor FOasisContentBrowserStyle::CustomToolbarBackgroundColor(0.0f, 0.0f, 0.0f, .2f);
FLinearColor FOasisContentBrowserStyle::CustomSourceViewBackgroundColor(0.0f, 0.0f, 0.0f, .1f);
FLinearColor FOasisContentBrowserStyle::CustomAssetViewBackgroundColor(0.0f, 0.0f, 0.0f, .1f);

void FOasisContentBrowserStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FOasisContentBrowserStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FOasisContentBrowserStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("SpyEngineStyle"));
	return StyleSetName;
}

FSlateFontInfo FOasisContentBrowserStyle::GetFontStyle(FName PropertyName, const ANSICHAR* Specifier /*= NULL*/)
{
	return Get().GetFontStyle(PropertyName, Specifier);
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( StyleSet->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( StyleSet->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

const FVector2D Icon12x12(12.0f, 12.0f);
const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon24x24(24.0f, 24.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > FOasisContentBrowserStyle::Create()
{
	TSharedRef< FSlateStyleSet > StyleSet = MakeShareable(new FSlateStyleSet("SpyEngineStyle"));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin("SpyEngine")->GetBaseDir() / TEXT("Resources"));

	// Icons
	{
		StyleSet->Set("SpyEngine.Icon16x", new IMAGE_BRUSH(TEXT("Icons/SpyEngineIcon24"), Icon16x16));
		StyleSet->Set("SpyEngine.Icon24x", new IMAGE_BRUSH(TEXT("Icons/SpyEngineIcon24"), Icon24x24));
		StyleSet->Set("SpyEngine.OpenSpyEngine", new IMAGE_BRUSH(TEXT("Icons/SpyEngineIcon64"), Icon40x40));
		StyleSet->Set("SpyEngine.OpenSpyEngine.Small", new IMAGE_BRUSH(TEXT("Icons/SpyEngineIcon24"), Icon16x16));
	}

	// Images
	{
		StyleSet->Set("SpyEngine.Help", new IMAGE_BRUSH(TEXT("Images/Documentation16"), Icon16x16));

		StyleSet->Set("SpyEngine.Rotation16px", new IMAGE_BRUSH(TEXT("Images/Loading"), Icon16x16));
		
		StyleSet->Set("SpyEngine.ShowSourcesView", new IMAGE_BRUSH(TEXT("Images/AssetTreeToggleExpanded16"), Icon16x16));
		StyleSet->Set("SpyEngine.HideSourcesView", new IMAGE_BRUSH(TEXT("Images/AssetTreeToggleCollapsed16"), Icon16x16));

		StyleSet->Set("SpyEngine.ShowDependencyViewer", new IMAGE_BRUSH(TEXT("Images/DependencyViewerExpanded16"), Icon16x16));
		StyleSet->Set("SpyEngine.HideDependencyViewer", new IMAGE_BRUSH(TEXT("Images/DependencyViewer16"), Icon16x16));

		StyleSet->Set("SpyEngine.ValidationUknown", new IMAGE_BRUSH(TEXT("Images/ValidationUknown"), Icon16x16));
		StyleSet->Set("SpyEngine.ValidationValid", new IMAGE_BRUSH(TEXT("Images/ValidationValid"), Icon16x16));
		StyleSet->Set("SpyEngine.ValidationInValid", new IMAGE_BRUSH(TEXT("Images/ValidationInValid"), Icon16x16));
		StyleSet->Set("SpyEngine.ValidationIssue", new IMAGE_BRUSH(TEXT("Images/ValidationIssue"), Icon16x16));

		// Folder Icons
		StyleSet->Set("SpyEngine.AssetTreeFolderDeveloper", new IMAGE_BRUSH("Images/FolderClosed", FVector2D(18, 16))); 
		StyleSet->Set("SpyEngine.AssetTreeFolderOpenCode", new IMAGE_BRUSH("Images/FolderOpen_Code", FVector2D(18, 16)));
		StyleSet->Set("SpyEngine.AssetTreeFolderClosedCode", new IMAGE_BRUSH("Images/FolderClosed_Code", FVector2D(18, 16)));

		StyleSet->Set("SpyEngine.AssetTreeFolderClosed", new IMAGE_BRUSH("Images/FolderClosed", FVector2D(18, 16)));
		StyleSet->Set("SpyEngine.AssetTreeFolderOpen", new IMAGE_BRUSH("Images/FolderOpen", FVector2D(18, 16)));
		
		StyleSet->Set("SpyEngine.AssetTreeFolderClosedPlugin", new IMAGE_BRUSH("Images/FolderClosed_Plugin", FVector2D(18, 16)));
		StyleSet->Set("SpyEngine.AssetTreeFolderOpenPlugin", new IMAGE_BRUSH("Images/FolderOpen_Plugin", FVector2D(18, 16)));

		StyleSet->Set("SpyEngine.AssetTreeFolderClosedProject", new IMAGE_BRUSH("Images/FolderClosed_Project", FVector2D(18, 16)));
		StyleSet->Set("SpyEngine.AssetTreeFolderOpenProject", new IMAGE_BRUSH("Images/FolderOpen_Project", FVector2D(18, 16)));
		
		StyleSet->Set("SpyEngine.AssetTreeFolderClosedVaultCache", new IMAGE_BRUSH("Images/FolderClosed_VaultCache", FVector2D(18, 16)));
		StyleSet->Set("SpyEngine.AssetTreeFolderOpenVaultCache", new IMAGE_BRUSH("Images/FolderOpen_VaultCache", FVector2D(18, 16)));
	}

	// Font
	{
		StyleSet->Set("SpyEngine.SourceTreeRootItemFont", DEFAULT_FONT("Regular", 12));
		StyleSet->Set("SpyEngine.SourceTreeRootItemFont.LoadingFont", DEFAULT_FONT("Regular", 9));

		StyleSet->Set("SpyEngine.SourceTreeItemFont", DEFAULT_FONT("Regular", 10));
	}

	// TextStyle
	{
		FTextBlockStyle NormalText = FTextBlockStyle()
			.SetFont(DEFAULT_FONT("Regular", FCoreStyle::RegularTextSize))
			.SetColorAndOpacity(FSlateColor::UseForeground())
			.SetShadowOffset(FVector2D::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Black);

		StyleSet->Set("SpyEngine.TopBar.Font", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Bold", 11))
			.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetShadowOffset(FVector2D(1, 1))
			.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f)));

		StyleSet->Set("SpyEngine.TopBar.DebugFont", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", 10))
			.SetColorAndOpacity(FLinearColor(.7f, .7f, .7f))
			.SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetShadowOffset(FVector2D(1, 1))
			.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f)));

		StyleSet->Set("SpyEngine.ChangeLogHeaderText", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", 10))
			.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetShadowOffset(FVector2D(1, 1))
			.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f)));

		StyleSet->Set("SpyEngine.SourceTreeRootItem.Loading", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", 8))
			.SetColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f)));

		StyleSet->Set("SpyEngine.AssetThumbnail.EngineOverlay", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", 9))
			.SetColorAndOpacity(FLinearColor(1.0, 1.0, 1.0, 0.9))
			.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.8))
			.SetShadowOffset(FVector2D(1.0f, 1.0f)));
	}

	// Colors
	{
		StyleSet->Set("ErrorReporting.HardReferenceColor", FLinearColor(0.35f, 0, 0));
		StyleSet->Set("ErrorReporting.HardReferenceColor.Darker", FLinearColor(0.35f * 0.6f, 0, 0));
		StyleSet->Set("ErrorReporting.SoftReferenceColor", FLinearColor(0.828f, 0.364f, 0.003f));
		StyleSet->Set("ErrorReporting.SoftReferenceColor.Darker", FLinearColor(0.828f * 0.6f, 0.364f * 0.6f, 0.003f));
	}

	return StyleSet;
}

void FOasisContentBrowserStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}


FDocumentationStyle FOasisContentBrowserStyle::GetChangLogDocumentationStyle()
{
	FDocumentationStyle DocumentationStyle;
	{
		DocumentationStyle
			.ContentStyle(TEXT("ChangeLog.Content.Text"))
			.BoldContentStyle(TEXT("ChangeLog.Content.TextBold"))
			.NumberedContentStyle(TEXT("ChangeLog.Content.Text"))
			.Header1Style(TEXT("ChangeLog.Content.HeaderText1"))
			.Header2Style(TEXT("ChangeLog.Content.HeaderText2"))
			.HyperlinkStyle(TEXT("Tutorials.Content.Hyperlink"))
			.HyperlinkTextStyle(TEXT("Tutorials.Content.HyperlinkText"))
			.SeparatorStyle(TEXT("Tutorials.Separator"));
	}
	return DocumentationStyle;
}


const ISlateStyle& FOasisContentBrowserStyle::Get()
{
	return *StyleInstance;
}

using namespace SpyEngineDOC;
TSharedPtr< FSlateStyleSet > FOasisDocumentationStyle::StyleSet = nullptr;
void FOasisDocumentationStyle::Initialize()
{

	// Only register once
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet(DocumentationStyleSetName));
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));
	StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));

	const FTextBlockStyle& NormalText = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");

	// Documentation
	{
		// Documentation tooltip defaults
		const FSlateColor HyperlinkColor(FLinearColor(0.1f, 0.1f, 0.5f));
		{
			const FTextBlockStyle DocumentationTooltipText = FTextBlockStyle(NormalText)
				.SetFont(DEFAULT_FONT("Regular", 9))
				.SetColorAndOpacity(FLinearColor::Black);
			StyleSet->Set("Documentation.SDocumentationTooltip", FTextBlockStyle(DocumentationTooltipText));

			const FTextBlockStyle DocumentationTooltipTextSubdued = FTextBlockStyle(NormalText)
				.SetFont(DEFAULT_FONT("Regular", 8))
				.SetColorAndOpacity(FLinearColor(0.1f, 0.1f, 0.1f));
			StyleSet->Set("Documentation.SDocumentationTooltipSubdued", FTextBlockStyle(DocumentationTooltipTextSubdued));

			const FTextBlockStyle DocumentationTooltipHyperlinkText = FTextBlockStyle(NormalText)
				.SetFont(DEFAULT_FONT("Regular", 8))
				.SetColorAndOpacity(HyperlinkColor);
			StyleSet->Set("Documentation.SDocumentationTooltipHyperlinkText", FTextBlockStyle(DocumentationTooltipHyperlinkText));

			const FButtonStyle DocumentationTooltipHyperlinkButton = FButtonStyle()
				.SetNormal(BORDER_BRUSH("Old/HyperlinkDotted", FMargin(0, 0, 0, 3 / 16.0f), HyperlinkColor))
				.SetPressed(FSlateNoResource())
				.SetHovered(BORDER_BRUSH("Old/HyperlinkUnderline", FMargin(0, 0, 0, 3 / 16.0f), HyperlinkColor));
			StyleSet->Set("Documentation.SDocumentationTooltipHyperlinkButton", FButtonStyle(DocumentationTooltipHyperlinkButton));
		}

		// Documentation defaults
		const FTextBlockStyle DocumentationText = FTextBlockStyle(NormalText)
			.SetColorAndOpacity(FLinearColor::Black)
			.SetFont(DEFAULT_FONT("Regular", 11));

		const FTextBlockStyle DocumentationHyperlinkText = FTextBlockStyle(DocumentationText)
			.SetColorAndOpacity(HyperlinkColor);

		const FTextBlockStyle DocumentationHeaderText = FTextBlockStyle(NormalText)
			.SetColorAndOpacity(FLinearColor::Black)
			.SetFont(DEFAULT_FONT("Black", 32));

		const FButtonStyle DocumentationHyperlinkButton = FButtonStyle()
			.SetNormal(BORDER_BRUSH("Old/HyperlinkDotted", FMargin(0, 0, 0, 3 / 16.0f), HyperlinkColor))
			.SetPressed(FSlateNoResource())
			.SetHovered(BORDER_BRUSH("Old/HyperlinkUnderline", FMargin(0, 0, 0, 3 / 16.0f), HyperlinkColor));

		StyleSet->Set("Documentation.Content", FTextBlockStyle(DocumentationText));

		const FHyperlinkStyle DocumentationHyperlink = FHyperlinkStyle()
			.SetUnderlineStyle(DocumentationHyperlinkButton)
			.SetTextStyle(DocumentationText)
			.SetPadding(FMargin(0.0f));
		StyleSet->Set("Documentation.Hyperlink", DocumentationHyperlink);

		StyleSet->Set("Documentation.Hyperlink.Button", FButtonStyle(DocumentationHyperlinkButton));
		StyleSet->Set("Documentation.Hyperlink.Text", FTextBlockStyle(DocumentationHyperlinkText));
		StyleSet->Set("Documentation.NumberedContent", FTextBlockStyle(DocumentationText));
		StyleSet->Set("Documentation.BoldContent", FTextBlockStyle(DocumentationText)
			.SetTypefaceFontName(TEXT("Bold")));

		StyleSet->Set("Documentation.Header1", FTextBlockStyle(DocumentationHeaderText)
			.SetFontSize(32));

		StyleSet->Set("Documentation.Header2", FTextBlockStyle(DocumentationHeaderText)
			.SetFontSize(24));

		StyleSet->Set("Documentation.Separator", new BOX_BRUSH("Common/Separator", 1 / 4.0f, FLinearColor(1, 1, 1, 0.5f)));
	}

	{
		//StyleSet->Set("Documentation.ToolTip.Background", new BOX_BRUSH("Tutorials/TutorialContentBackground", FMargin(4 / 16.0f)));
	}

	// Tutorial
	{
		const FTextBlockStyle RichTextNormal = FTextBlockStyle()
			.SetFont(DEFAULT_FONT("Regular", 11))
			.SetColorAndOpacity(FSlateColor::UseForeground())
			.SetShadowOffset(FVector2D::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Black)
			.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f))
			.SetHighlightShape(BOX_BRUSH("Common/TextBlockHighlightShape", FMargin(3.f / 8.f)));

		StyleSet->Set("Tutorials.Content.Text", RichTextNormal);

		StyleSet->Set("Tutorials.Content.TextBold", FTextBlockStyle(RichTextNormal)
			.SetFont(DEFAULT_FONT("Bold", 11)));

		StyleSet->Set("Tutorials.Content.HeaderText1", FTextBlockStyle(RichTextNormal)
			.SetFontSize(20));

		StyleSet->Set("Tutorials.Content.HeaderText2", FTextBlockStyle(RichTextNormal)
			.SetFontSize(16));

		const FButtonStyle RichTextHyperlinkButton = FButtonStyle()
			.SetNormal(BORDER_BRUSH("Old/HyperlinkDotted", FMargin(0, 0, 0, 3 / 16.0f), FLinearColor::Blue))
			.SetPressed(FSlateNoResource())
			.SetHovered(BORDER_BRUSH("Old/HyperlinkUnderline", FMargin(0, 0, 0, 3 / 16.0f), FLinearColor::Blue));

		const FTextBlockStyle RichTextHyperlinkText = FTextBlockStyle(RichTextNormal)
			.SetColorAndOpacity(FLinearColor::Blue);

		StyleSet->Set("Tutorials.Content.HyperlinkText", RichTextHyperlinkText);

		const FHyperlinkStyle RichTextHyperlink = FHyperlinkStyle()
			.SetUnderlineStyle(RichTextHyperlinkButton)
			.SetTextStyle(RichTextHyperlinkText)
			.SetPadding(FMargin(0.0f));
		StyleSet->Set("Tutorials.Content.Hyperlink", RichTextHyperlink);

		StyleSet->Set("Tutorials.Content.ExternalLink", new IMAGE_BRUSH("Tutorials/ExternalLink", Icon16x16, FLinearColor::Blue));

		StyleSet->Set("Tutorials.Separator", new BOX_BRUSH("Common/Separator", 1 / 4.0f, FLinearColor::Black));
	}

	// ChangLog
	{
		const FTextBlockStyle RichTextNormal = FTextBlockStyle()
			.SetFont(DEFAULT_FONT("Regular", 10))
			.SetColorAndOpacity(FSlateColor::UseForeground())
			.SetShadowOffset(FVector2D::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Black)
			.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f))
			.SetHighlightShape(BOX_BRUSH("Common/TextBlockHighlightShape", FMargin(3.f / 8.f)));

		const FLinearColor ColorChangeLogText(0.8f, 0.8f, 0.8f, 0.7f);
		StyleSet->Set("ChangeLog.Content.Text", FTextBlockStyle(RichTextNormal)
			.SetColorAndOpacity(ColorChangeLogText));

		StyleSet->Set("ChangeLog.Content.TextBold", FTextBlockStyle(RichTextNormal)
			.SetFont(DEFAULT_FONT("Bold", 10)));

		StyleSet->Set("ChangeLog.Content.HeaderText1", FTextBlockStyle(RichTextNormal)
			.SetFont(DEFAULT_FONT("Bold", 10)));

		StyleSet->Set("ChangeLog.Content.HeaderText2", FTextBlockStyle(RichTextNormal)
			.SetFont(DEFAULT_FONT("Bold", 11)));
	}

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
};

void FOasisDocumentationStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT