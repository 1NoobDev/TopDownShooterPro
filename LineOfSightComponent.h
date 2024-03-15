#pragma once  

#include "CoreMinimal.h"  
#include "Components/ActorComponent.h"  
#include "GameFramework/Actor.h" 
#include <Set> 
#include "TimerManager.h"  
#include "Engine/DecalActor.h"  
#include "LineOfSightComponent.generated.h"  

// Define the log category for LineOfSightComponent  
DECLARE_LOG_CATEGORY_EXTERN(LogLineOfSightComponent, Log, All);

// Add a new struct to hold ghost actors and their timers  
struct FGhostInfo
{
    AActor * GhostActor;
    float TimeWhenHidden; // Time when the enemy was last seen  

    // Constructor  
    FGhostInfo()
        : GhostActor(nullptr), TimeWhenHidden(0.0f)
    {
    }
};


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TOPDOWNSHOOTERPRO_API ULineOfSightComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULineOfSightComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // The distance of the cone trace.  
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LineOfSight")
    float TraceDistance;

    // The angle of the cone in degrees.  
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LineOfSight")
    float ConeAngleHorizontal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LineOfSight")
    float ConeAngleVertical;

    // The number of traces to perform within the cone.  
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LineOfSight")
    int32 NumberOfTraces;

    // Tag to check for when identifying actors to consider as visible  
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LineOfSight")
    FName VisibleActorTag;

    // Bool to enable/disable debug drawing  
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LineOfSight")
    bool bDrawDebug;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LineOfSight")
    UMaterialInterface* GhostMaterial;

private:
    // Perform a cone trace to check for enemies in line of sight.  
    void PerformConeTrace();

    void EndPlay(const EEndPlayReason::Type EndPlayReason);

    // Maintain a list of visible enemies
    TSet<AActor*> VisibleEnemies;
    FTimerHandle VisibilityCheckTimerHandle;

    // Map to keep track of ghost actors and their destroy timers  
    TMap<AActor*, FGhostInfo> GhostActorsMap;

    // Function to handle the destruction of ghost actors  
    void DestroyGhostActor(AActor* EnemyActor);
};
