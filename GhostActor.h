// GhostActor.h
#pragma once  

#include "CoreMinimal.h"  
#include "GameFramework/Actor.h"  
#include "Components/PoseableMeshComponent.h"
#include "GhostActor.generated.h"  

UCLASS()
class TOPDOWNSHOOTERPRO_API AGhostActor : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties  
    AGhostActor();

protected:
    // Called when the game starts or when spawned  
    virtual void BeginPlay() override;

public:
    // Posable Mesh Component for the ghost actor  
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UPoseableMeshComponent* GhostPoseableMesh;
};
