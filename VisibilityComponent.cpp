#include "VisibilityComponent.h"  
#include "Components/StaticMeshComponent.h"  
#include "Components/BoxComponent.h"  
#include "Engine/StaticMeshActor.h"  

UVisibilityComponent::UVisibilityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UVisibilityComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (!Owner) return;

    TArray<UActorComponent*> AllMeshComponents;
    Owner->GetComponents(UStaticMeshComponent::StaticClass(), AllMeshComponents);
    TArray<UActorComponent*> AllBoxComponents;
    Owner->GetComponents(UBoxComponent::StaticClass(), AllBoxComponents);

    for (const FFloorData& Floor : Floors)
    {
        TArray<UStaticMeshComponent*> MeshComponentsForFloor;
        for (const FString& MeshName : Floor.MeshComponentNames)
        {
            UActorComponent** FoundComponent = AllMeshComponents.FindByPredicate([&](UActorComponent* Comp) {
                return Comp->GetName() == MeshName;
                });
            UStaticMeshComponent* MeshComp = FoundComponent ? Cast<UStaticMeshComponent>(*FoundComponent) : nullptr;
            if (MeshComp)
            {
                MeshComponentsForFloor.Add(MeshComp);
                // Cache the original materials      
                TArray<UMaterialInterface*> OriginalMaterials;
                for (int32 i = 0; i < MeshComp->GetNumMaterials(); ++i)
                {
                    OriginalMaterials.Add(MeshComp->GetMaterial(i));
                }
                OriginalMaterialsMap.Add(MeshComp, OriginalMaterials);
            }
        }

        UActorComponent** FoundTriggerComponent = AllBoxComponents.FindByPredicate([&](UActorComponent* Comp) {
            return Comp->GetName() == Floor.TriggerBoxName;
            });
        UBoxComponent* BoxComp = FoundTriggerComponent ? Cast<UBoxComponent>(*FoundTriggerComponent) : nullptr;
        if (BoxComp)
        {
            FloorMeshComponentsMap.Add(BoxComp, MeshComponentsForFloor);
            // Set up overlap events      
            BoxComp->OnComponentBeginOverlap.AddDynamic(this, &UVisibilityComponent::MakeUpperFloorsTransparent);
            BoxComp->OnComponentEndOverlap.AddDynamic(this, &UVisibilityComponent::RestoreUpperFloorsMaterials);
        }
    }
}

void UVisibilityComponent::MakeUpperFloorsTransparent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !OtherActor->Tags.Contains(FName("Player")))
    {
        return;
    }

    UBoxComponent* OverlappedBox = Cast<UBoxComponent>(OverlappedComponent);
    if (OverlappedBox && FloorMeshComponentsMap.Contains(OverlappedBox))
    {
        TArray<UStaticMeshComponent*> MeshComponents = FloorMeshComponentsMap[OverlappedBox];
        for (UStaticMeshComponent* MeshComp : MeshComponents)
        {
            if (MeshComp)
            {
                // Increase reference count for transparency  
                TransparencyReferenceCount.FindOrAdd(MeshComp)++;
                int32 NumMaterials = MeshComp->GetNumMaterials();
                for (int32 i = 0; i < NumMaterials; ++i)
                {
                    MeshComp->SetMaterial(i, TransparentMaterial);
                }
            }
        }
    }
}

void UVisibilityComponent::RestoreUpperFloorsMaterials(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor || !OtherActor->Tags.Contains(FName("Player")))
    {
        return;
    }

    UBoxComponent* OverlappedBox = Cast<UBoxComponent>(OverlappedComponent);
    if (OverlappedBox && FloorMeshComponentsMap.Contains(OverlappedBox))
    {
        TArray<UStaticMeshComponent*> MeshComponents = FloorMeshComponentsMap[OverlappedBox];
        for (UStaticMeshComponent* MeshComp : MeshComponents)
        {
            if (MeshComp && OriginalMaterialsMap.Contains(MeshComp))
            {
                // Decrease reference count for transparency  
                int32& RefCount = TransparencyReferenceCount.FindOrAdd(MeshComp);
                RefCount--;
                if (RefCount <= 0) // Restore materials only if no other trigger requires transparency  
                {
                    TArray<UMaterialInterface*>& OriginalMaterials = OriginalMaterialsMap[MeshComp];
                    for (int32 i = 0; i < OriginalMaterials.Num(); ++i)
                    {
                        MeshComp->SetMaterial(i, OriginalMaterials[i]);
                    }
                    TransparencyReferenceCount.Remove(MeshComp); // Remove the key if not needed anymore  
                }
            }
        }
    }
}