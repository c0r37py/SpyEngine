// Innerloop Interactive Production :: c0r37py

#pragma once

#include "OasisContentBrowser.h"
#include "OasisContentBrowserTypes.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "OasisContentBrowserBlueprintLibrary.generated.h"

UCLASS(meta = (ScriptName = "SpyEngineLibrary"))
class OASISCONTENTBROWSER_API UOasisContentBrowserBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (Category = "SpyEngine|Import", AutoCreateRefTerm = "AssetFilePaths, ImportSetting"))
	static void ImportUAssets(const TArray<FString>& AssetFilePaths, const FUAssetImportSetting& ImportSetting, bool bOneByOne = false);

	UFUNCTION(BlueprintCallable, meta = (Category = "SpyEngine|Import", AutoCreateRefTerm = "ImportSetting"))
	static void ImportUAsset(const FString& AssetFilePath, const FUAssetImportSetting& ImportSetting);

	UFUNCTION(BlueprintCallable, meta = (Category = "SpyEngine|Import", AutoCreateRefTerm = "ImportSetting"))
	static void ImportFromFileList(const FString& FilePath, const FUAssetImportSetting& ImportSetting, bool bOneByOne = false);

	UFUNCTION(BlueprintPure, meta = (Category = "SpyEngine|Get"))
	static void GetGlobalImportSetting(FUAssetImportSetting& ImportSetting);

	UFUNCTION(BlueprintPure, meta = (Category = "SpyEngine|Get"))
	static void GetSelectedUAssetFilePaths(TArray<FString>& AssetFilePaths);

#if ECB_TODO
	//UFUNCTION(BlueprintPure, meta = (Category = "SpyEngine|Get"))
	static void GetUAssetFilePathsInFolder(FName FolderPath, TArray<FString>& AssetFilePaths, bool bRecursive = false);
#endif
};
