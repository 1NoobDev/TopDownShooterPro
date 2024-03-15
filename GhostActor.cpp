// GhostActor.cpp
#include "GhostActor.h"  

// Sets default values  
AGhostActor::AGhostActor()
{
    // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.  
    PrimaryActorTick.bCanEverTick = false;

    // Create a posable mesh component and set it as the root component  
    GhostPoseableMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("GhostPoseableMesh"));
    SetRootComponent(GhostPoseableMesh);

    // Disable physics and collision on the skeletal mesh  
    GhostPoseableMesh->SetSimulatePhysics(false);
    GhostPoseableMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned  
void AGhostActor::BeginPlay()
{
    Super::BeginPlay();
}
