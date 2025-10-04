// Innerloop Interactive Production :: c0r37py

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

#include "OasisContentBrowserMenuContexts.generated.h"

class FAssetContextMenu;
class IAssetTypeActions;

UCLASS()
class  UOasisContentBrowserAssetContextMenuContext : public UObject
{
	GENERATED_BODY()

public:

	TWeakPtr<FAssetContextMenu> AssetContextMenu;

	TWeakPtr<IAssetTypeActions> CommonAssetTypeActions;
	
	UPROPERTY()
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;

	UPROPERTY()
	UClass* CommonClass;

	UFUNCTION(BlueprintCallable, Category="Tool Menus")
	TArray<UObject*> GetSelectedObjects() const
	{
		TArray<UObject*> Result;
		Result.Reserve(SelectedObjects.Num());
		for (const TWeakObjectPtr<UObject>& Object : SelectedObjects)
		{
			Result.Add(Object.Get());
		}
		return Result;
	}

public:

	// Todo: GetSelectedObjects for FOasisAssetData
	void SetNumSelectedObjects(int32 InNum)
	{
		NumSelectedObjects = InNum;
	}

	int32 GetNumSelectedObjects() const 
	{
		return NumSelectedObjects;
	}

private:
	int32 NumSelectedObjects = 0;
};

