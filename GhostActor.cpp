// GhostActor.cpp
#include "GhostActor.h"  
#include "Components/SkeletalMeshComponent.h"  

// Sets default values  
AGhostActor::AGhostActor()
{
    // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.  
    PrimaryActorTick.bCanEverTick = false;

    // Create a skeletal mesh component and set it as the root component  
    GhostSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GhostSkeletalMesh"));
    SetRootComponent(GhostSkeletalMesh);

    // Disable physics and collision on the skeletal mesh  
    GhostSkeletalMesh->SetSimulatePhysics(false);
    GhostSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned  
void AGhostActor::BeginPlay()
{
    Super::BeginPlay();
}

// Called every frame  
void AGhostActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
