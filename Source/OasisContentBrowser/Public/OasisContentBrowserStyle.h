// Innerloop Interactive Production :: c0r37py

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"
#include "IDocumentation.h"

/**  */
class FOasisContentBrowserStyle
{
public:

	static void Initialize();

	static void Shutdown();

	/** reloads textures used by slate renderer */
	static void ReloadTextures();

	/** @return The Slate style set for the Shooter game */
	static const ISlateStyle& Get();

	static FName GetStyleSetName();

	static FSlateFontInfo GetFontStyle(FName PropertyName, const ANSICHAR* Specifier = NULL);

	static FDocumentationStyle GetChangLogDocumentationStyle();


public:
	// For debug
	static FLinearColor CustomContentBrowserBorderBackgroundColor;
	static FLinearColor CustomToolbarBackgroundColor;
	static FLinearColor CustomSourceViewBackgroundColor;
	static FLinearColor CustomAssetViewBackgroundColor;

private:

	static TSharedRef< class FSlateStyleSet > Create();

private:

	static TSharedPtr< class FSlateStyleSet > StyleInstance;
};
