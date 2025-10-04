// Innerloop Interactive Production :: c0r37py

#pragma once

#include "OasisContentBrowserBlueprintLibrary.h"
#include "OasisContentBrowserSingleton.h"
#include "OasisPackageUtils.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

void UOasisContentBrowserBlueprintLibrary::ImportUAssets(const TArray<FString>& AssetFilePaths, const FUAssetImportSetting& ImportSetting, bool bOneByOne)
{
	FUAssetImportSetting MyImportSetting = ImportSetting;
	MyImportSetting.ResolveTargetContentDir(); // Target Content Dir need to be resolved for manual constructed Import Settings

	if (bOneByOne)
	{
		for (auto& AssetFilePath : AssetFilePaths)
		{
			FExtAssetImporter::ImportAssetsByFilePaths({ AssetFilePath }, MyImportSetting);
		}
	}
	else
	{
		FExtAssetImporter::ImportAssetsByFilePaths(AssetFilePaths, MyImportSetting);
	}
}

void UOasisContentBrowserBlueprintLibrary::ImportUAsset(const FString& AssetFilePath, const FUAssetImportSetting& ImportSetting)
{
	FUAssetImportSetting MyImportSetting = ImportSetting;
	MyImportSetting.ResolveTargetContentDir(); // Target Content Dir need to be resolved for manual constructed Import Settings

	FExtAssetImporter::ImportAssetsByFilePaths({ AssetFilePath }, MyImportSetting);
}

void UOasisContentBrowserBlueprintLibrary::ImportFromFileList(const FString& FilePath, const FUAssetImportSetting& ImportSetting, bool bOneByOne)
{
	struct Local
	{
		static void GetAssetFileList(const TArray<FString>& InRawFilePaths, TArray<FString>& OutAsstFlieList)
		{
			int32 Index = 0;
			for (const FString& RawPath : InRawFilePaths)
			{
				ECB_LOG(Display, TEXT(" %d - %s"), ++Index, *RawPath);
				FString Path(RawPath);
				Path.TrimStartAndEndInline();

				// Is File
				if (FPaths::FileExists(*RawPath))
				{
					ECB_LOG(Display, TEXT("    => File, extension: %s"), *FPaths::GetExtension(Path));
					FPaths::NormalizeFilename(Path);

					if (FExtAssetSupport::IsSupportedPackageFilename(Path))
					{
						OutAsstFlieList.AddUnique(Path);
					}
				}
				// Is Potential Dir
				else
				{
					FPaths::NormalizeDirectoryName(Path);

					bool bIsDir = false;
					bool bRecursive = false;

					const FString RecursiveDirPattern(TEXT("/**"));
					const FString DirPattern(TEXT("/**"));

					if (Path.EndsWith(RecursiveDirPattern))
					{
						Path.RemoveFromEnd(RecursiveDirPattern);
						bRecursive = true;
					}
					else if (Path.EndsWith(DirPattern))
					{
						Path.RemoveFromEnd(DirPattern);
					}

					if (FPaths::DirectoryExists(Path))
					{
						bIsDir = true;
					}

					if (bIsDir)
					{
						FPaths::NormalizeDirectoryName(Path);

						TArray<FString> FilesInDir;
						FOasisPackageUtils::GetAllPackages(Path, FilesInDir, bRecursive);

						OutAsstFlieList.Append(FilesInDir);

						ECB_LOG(Display, TEXT("    => Folder, normalized: %s , %d files"), *Path, FilesInDir.Num());
					}
				}
			}
		}
	};
	if (FPaths::FileExists(*FilePath))
	{
		TArray<FString> RawFilePaths;
		FFileHelper::LoadFileToStringArray(RawFilePaths, *FilePath);
		ECB_LOG(Display, TEXT("[ImportFromFileList] %d lines"), RawFilePaths.Num());
		if (RawFilePaths.Num() <= 0)
		{
			return;
		}

		TArray<FString> AssetFilePaths;
		Local::GetAssetFileList(RawFilePaths, AssetFilePaths);
		if (AssetFilePaths.Num() <= 0)
		{
			return;
		}

		FUAssetImportSetting MyImportSetting = ImportSetting;
		MyImportSetting.ResolveTargetContentDir(); // Target Content Dir need to be resolved for manual constructed Import Settings

		ImportUAssets(AssetFilePaths, MyImportSetting, bOneByOne);
	}
}


void UOasisContentBrowserBlueprintLibrary::GetGlobalImportSetting(FUAssetImportSetting& ImportSetting)
{
	ImportSetting =  FUAssetImportSetting::GetSavedImportSetting();
}

void UOasisContentBrowserBlueprintLibrary::GetSelectedUAssetFilePaths(TArray<FString>& AssetFilePaths)
{
	TArray<FOasisAssetData> SelectedAssets;
	FOasisContentBrowserSingleton::Get().GetSelectedAssets(SelectedAssets);
	
	AssetFilePaths.Empty(SelectedAssets.Num());
	for (const FOasisAssetData& AssetData : SelectedAssets)
	{
		AssetFilePaths.Add(AssetData.PackageFilePath.ToString());
	}
}

#if ECB_TODO
void UOasisContentBrowserBlueprintLibrary::GetUAssetFilePathsInFolder(FName FolderPath, TArray<FString>& AssetFilePaths, bool bRecursive/* = false*/);
{
}
#endif
