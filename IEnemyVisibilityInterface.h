// IEnemyVisibilityInterface.h  
#pragma once  

#include "CoreMinimal.h"  
#include "UObject/Interface.h"  
#include "IEnemyVisibilityInterface.generated.h"  

UINTERFACE(MinimalAPI, Blueprintable)
class UEnemyVisibilityInterface : public UInterface
{
    GENERATED_BODY()
};

class TOPDOWNSHOOTERPRO_API IEnemyVisibilityInterface
{
    GENERATED_IINTERFACE_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Visibility")
    void SetVisible(bool bVisible);
};