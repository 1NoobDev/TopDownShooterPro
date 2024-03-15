#pragma once  

#include "CoreMinimal.h"  
#include "Components/ActorComponent.h"  
#include "TimerManager.h"  
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
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Bool to enable/disable the LineOfSightComponent functionality  
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LineOfSight")
    bool bIsLineOfSightEnabled;

    // Call this function to disable the line of sight and remove ghost actors  
    UFUNCTION(BlueprintCallable, Category = "LineOfSight")
    void DisableLineOfSightAndCleanup();

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

    // Material for the Ghost Actor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LineOfSight")
    UMaterialInterface* GhostMaterial;

private:
    void PerformConeTrace();
    TSet<AActor*> VisibleEnemies;
    FTimerHandle VisibilityCheckTimerHandle;
    TMap<AActor*, FGhostInfo> GhostActorsMap;

    void PerformVisibilityCheck(TSet<AActor*>& OutCurrentlyVisibleEnemies);
    void UpdateVisibilityStates(const TSet<AActor*>& CurrentlyVisibleEnemies);
    void HandleActorVisibilityChange(AActor* Actor, bool bIsNowVisible);
    void SpawnGhostActor(AActor* EnemyActor);
    void UpdateGhostActors(const TSet<AActor*>& CurrentlyVisibleEnemies);
    void DestroyGhostActor(AActor* EnemyActor);
};