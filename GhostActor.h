// GhostActor.h
#pragma once  

#include "CoreMinimal.h"  
#include "GameFramework/Actor.h"  
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
    // Called every frame  
    virtual void Tick(float DeltaTime) override;

    // Skeletal Mesh Component for the ghost actor  
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USkeletalMeshComponent* GhostSkeletalMesh;
};
