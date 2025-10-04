// Innerloop Interactive Production :: c0r37py

#pragma once

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"
#include "Styling/SlateStyle.h"
#include "IDocumentation.h"

#include "DocumentationDefines.h"

namespace EXT_DOC_NAMESPACE
{

class FOasisDocumentationStyle
{
public:
	static void Initialize();

	static void Shutdown();

	static const ISlateStyle& Get();

	static FDocumentationStyle GetDefaultDocumentationStyle();

private:
	static TSharedPtr< class FSlateStyleSet > StyleSet;
};

}
