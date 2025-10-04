// Innerloop Interactive Production :: c0r37py

#include "OasisContentBrowser.h"
#include "OasisContentBrowserSingleton.h"

#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "OasisDocumentation.h"

DEFINE_LOG_CATEGORY(LogECB);

#define LOCTEXT_NAMESPACE "OasisContentBrowser"

/////////////////////////////////////////////////////////////
// FOasisContentBrowserModule implementation
//

void FOasisContentBrowserModule::StartupModule()
{
	ContentBrowserSingleton = new FOasisContentBrowserSingleton();

	// Documentation
	FOasisDocumentationStyle::Initialize();
	Documentation = FOasisDocumentation::Create();
	FDocumentationProvider::Get().RegisterProvider(*DocumentationHostPluginName, this);
}

void FOasisContentBrowserModule::ShutdownModule()
{
	if (ContentBrowserSingleton)
	{
		delete ContentBrowserSingleton;
		ContentBrowserSingleton = NULL;
	}

	// Documentation
	FOasisDocumentationStyle::Shutdown();
}

IOasisContentBrowserSingleton& FOasisContentBrowserModule::Get() const
{
	check(ContentBrowserSingleton);
	return *ContentBrowserSingleton;
}

TSharedRef<IDocumentation> FOasisContentBrowserModule::GetDocumentation() const
{
	return Documentation.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOasisContentBrowserModule, OasisContentBrowser)

#ifdef EXT_DOC_NAMESPACE
#undef EXT_DOC_NAMESPACE
#endif
