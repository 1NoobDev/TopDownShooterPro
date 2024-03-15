#include "CoreMinimal.h"  
#include "Components/ActorComponent.h"  
#include "Components/BoxComponent.h"  
#include "VisibilityComponent.generated.h"  

USTRUCT(BlueprintType)
struct FFloorData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Data")
    TArray<FString> MeshComponentNames;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Data")
    FString TriggerBoxName;
};
  
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))  
class TOPDOWNSHOOTERPRO_API UVisibilityComponent : public UActorComponent  
{  
    GENERATED_BODY()  
  
protected:  
    virtual void BeginPlay() override;  
  
public:  
    UVisibilityComponent();  
  
    UFUNCTION(BlueprintCallable)  
    void MakeUpperFloorsTransparent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);  
  
    UFUNCTION(BlueprintCallable)  
    void RestoreUpperFloorsMaterials(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);  
  
    UPROPERTY(EditAnywhere, Category = "Visibility")  
    TArray<FFloorData> Floors;  
  
    UPROPERTY(EditAnywhere, Category = "Visibility")  
    UMaterialInterface* TransparentMaterial;  
  
private:  
    TMap<UStaticMeshComponent*, TArray<UMaterialInterface*>> OriginalMaterialsMap;  
    TMap<UStaticMeshComponent*, int32> TransparencyReferenceCount;  
    TMap<UBoxComponent*, TArray<UStaticMeshComponent*>> FloorMeshComponentsMap;  
};  
