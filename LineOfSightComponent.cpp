#include "LineOfSightComponent.h"      
#include "Engine/World.h"      
#include "GameFramework/PlayerController.h"      
#include "DrawDebugHelpers.h"      
#include "Components/PrimitiveComponent.h"    
#include "GameFramework/Character.h"      
#include "Engine/Engine.h"    
#include "IEnemyVisibilityInterface.h"  
#include "GhostActor.h"
#include "Animation/AnimInstance.h"  
#include "Components/PoseableMeshComponent.h"
#include "Logging/LogMacros.h"  

// Define the log category for LineOfSightComponent    
DEFINE_LOG_CATEGORY(LogLineOfSightComponent);


ULineOfSightComponent::ULineOfSightComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    // Initialize default values      
    TraceDistance = 4000.0f;
    ConeAngleHorizontal = 180.0f;
    ConeAngleVertical = 35.0f;
    NumberOfTraces = 50;
    bDrawDebug = false;
}

void ULineOfSightComponent::BeginPlay()
{
    Super::BeginPlay();
    // Set up the timer to call TimerPerformConeTrace every 0.2 seconds (or whatever interval you prefer)      
    GetWorld()->GetTimerManager().SetTimer(VisibilityCheckTimerHandle, this, &ULineOfSightComponent::PerformConeTrace, 0.2f, true);
}

void ULineOfSightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void ULineOfSightComponent::PerformConeTrace()
{
    TSet<AActor*> CurrentlyVisibleEnemies;

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
        return;

    FVector EyeLocation;
    FRotator EyeRotation;
    if (USkeletalMeshComponent* MeshComp = OwnerCharacter->GetMesh())
    {
        EyeLocation = MeshComp->GetSocketLocation("EyeSocket");
        EyeRotation = MeshComp->GetSocketRotation("EyeSocket");
    }
    else
    {
        EyeLocation = OwnerCharacter->GetActorLocation();
        EyeRotation = OwnerCharacter->GetActorRotation();
    }

    FVector StartTrace = EyeLocation;
    FVector EndTrace;

    // Calculate the number of traces for horizontal and vertical directions    
    int32 NumberOfHorizontalTraces = FMath::CeilToInt(FMath::Sqrt(NumberOfTraces * (ConeAngleHorizontal / ConeAngleVertical)));
    int32 NumberOfVerticalTraces = FMath::Max(3, NumberOfTraces / NumberOfHorizontalTraces); // At least 3 vertical traces    

    float DeltaHorizontalAngle = ConeAngleHorizontal / NumberOfHorizontalTraces;
    float DeltaVerticalAngle = ConeAngleVertical / NumberOfVerticalTraces;

    for (int32 i = 0; i < NumberOfHorizontalTraces; ++i)
    {
        for (int32 j = 0; j < NumberOfVerticalTraces; ++j)
        {
            float CurrentHorizontalAngle = -ConeAngleHorizontal / 2 + i * DeltaHorizontalAngle;
            float CurrentVerticalAngle = -ConeAngleVertical / 2 + j * DeltaVerticalAngle;

            FRotator TraceRotation = EyeRotation + FRotator(CurrentVerticalAngle, CurrentHorizontalAngle, 0);
            FVector TraceDirection = TraceRotation.Vector();
            EndTrace = StartTrace + TraceDirection * TraceDistance;

            FHitResult HitResult;
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(OwnerCharacter);
            QueryParams.bReturnPhysicalMaterial = false;

            bool bHit = GetWorld()->LineTraceSingleByChannel(
                HitResult,
                StartTrace,
                EndTrace,
                ECC_GameTraceChannel1,
                QueryParams
            );

            if (bHit)
            {
                AActor* HitActor = HitResult.GetActor();
                if (HitActor)
                {
                    if (HitActor && HitActor->ActorHasTag(VisibleActorTag))
                    {
                        if (HitActor->GetClass()->ImplementsInterface(UEnemyVisibilityInterface::StaticClass()))
                        {
                            CurrentlyVisibleEnemies.Add(HitActor);
                            IEnemyVisibilityInterface::Execute_SetVisible(HitActor, true);
                            // Log that the actor is now visible    
                            UE_LOG(LogLineOfSightComponent, Log, TEXT("Actor %s is now VISIBLE"), *HitActor->GetName());
                        }
                        else
                        {
                            // Log that the actor does not implement the interface    
                            UE_LOG(LogLineOfSightComponent, Warning, TEXT("Actor %s does not implement IEnemyVisibilityInterface"), *HitActor->GetName());
                        }
                    }
                }
            }
            // Debug drawing for line traces    
            if (bDrawDebug)
            {
                DrawDebugLine(GetWorld(), StartTrace, EndTrace, bHit ? FColor::Red : FColor::Green, false, 0.2f, 0, 1.0f);
            }
        }
    }

    // Hide enemies that were previously visible but now are not      
    for (AActor* PreviouslyVisibleEnemy : VisibleEnemies)
    {
        if (!CurrentlyVisibleEnemies.Contains(PreviouslyVisibleEnemy))
        {
            if (PreviouslyVisibleEnemy->GetClass()->ImplementsInterface(UEnemyVisibilityInterface::StaticClass()))
            {
                IEnemyVisibilityInterface::Execute_SetVisible(PreviouslyVisibleEnemy, false);
                // Log that the actor is now hidden      
                UE_LOG(LogLineOfSightComponent, Log, TEXT("Actor %s is now HIDDEN"), *PreviouslyVisibleEnemy->GetName());

                // Spawn the ghost actor at the last known position of the enemy          
                if (!GhostActorsMap.Contains(PreviouslyVisibleEnemy))
                {
                    USkeletalMeshComponent* EnemySkeletalMesh = Cast<USkeletalMeshComponent>(PreviouslyVisibleEnemy->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
                    if (EnemySkeletalMesh && EnemySkeletalMesh->SkeletalMesh)
                    {
                        // Create a new actor for the ghost    
                        AGhostActor* GhostActor = GetWorld()->SpawnActor<AGhostActor>(AGhostActor::StaticClass(), EnemySkeletalMesh->GetComponentTransform());

                        // Create a new poseable mesh component for the ghost actor    
                        UPoseableMeshComponent* GhostPoseableMesh = NewObject<UPoseableMeshComponent>(GhostActor);
                        if (GhostPoseableMesh)
                        {
                            GhostActor->AddInstanceComponent(GhostPoseableMesh);
                            GhostPoseableMesh->RegisterComponent();
                            GhostPoseableMesh->SetSkeletalMesh(EnemySkeletalMesh->SkeletalMesh);
                            GhostPoseableMesh->SetWorldTransform(EnemySkeletalMesh->GetComponentTransform());
                            GhostPoseableMesh->SetMaterial(0, GhostMaterial); // Set the ghost material    
                            GhostPoseableMesh->SetMaterial(1, GhostMaterial); // Set the ghost material    

                            // Disable physics and collision    
                            GhostPoseableMesh->SetSimulatePhysics(false);
                            GhostPoseableMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                            GhostPoseableMesh->SetVisibility(true);

                            // Copy the pose from the enemy's skeletal mesh to the ghost's poseable mesh  
                            const int32 NumBones = EnemySkeletalMesh->GetNumBones();
                            for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
                            {
                                const FName BoneName = EnemySkeletalMesh->GetBoneName(BoneIndex);
                                const FTransform BoneTransform = EnemySkeletalMesh->GetBoneTransform(BoneIndex);

                                GhostPoseableMesh->SetBoneTransformByName(BoneName, BoneTransform, EBoneSpaces::WorldSpace);
                            }

                            // Set up the ghost info and timer to destroy the ghost    
                            FGhostInfo NewGhostInfo;
                            NewGhostInfo.GhostActor = GhostActor;
                            NewGhostInfo.TimeWhenHidden = GetWorld()->GetTimeSeconds(); // Set the time when the enemy was hidden    
                            GhostActorsMap.Add(PreviouslyVisibleEnemy, NewGhostInfo);
                        }
                    }
                }
            }

                // Check if the ghost should be destroyed because the enemy is visible again or the timer has expired  
                FGhostInfo* GhostInfo = GhostActorsMap.Find(PreviouslyVisibleEnemy);
                if (GhostInfo && GhostInfo->GhostActor)
                {
                    // Check if the enemy is now visible again or if 2 seconds have passed  
                    if (CurrentlyVisibleEnemies.Contains(PreviouslyVisibleEnemy) || GetWorld()->GetTimeSeconds() - GhostInfo->TimeWhenHidden >= 2.0f)
                    {
                        DestroyGhostActor(PreviouslyVisibleEnemy);
                    }
                }
            }
            else
            {
                // The enemy is currently visible, so destroy its ghost if it exists  
                DestroyGhostActor(PreviouslyVisibleEnemy);
            }
        }

        // Update the list of visible enemies  
        VisibleEnemies = CurrentlyVisibleEnemies;
    }

    void ULineOfSightComponent::DestroyGhostActor(AActor* EnemyActor)
    {
        FGhostInfo* GhostInfo = GhostActorsMap.Find(EnemyActor);
        if (GhostInfo)
        {
            if (GhostInfo->GhostActor)
            {
                // Destroy the ghost actor  
                GhostInfo->GhostActor->Destroy();
            }
            GhostActorsMap.Remove(EnemyActor);
        }
    }

void ULineOfSightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)  
{  
    Super::EndPlay(EndPlayReason);  
    // Clear the timer to ensure it doesn't try to call back to a destroyed component  
    if (GetWorld())  
    {  
        GetWorld()->GetTimerManager().ClearTimer(VisibilityCheckTimerHandle);  
    }  
  
    // Destroy all ghost actors to clean up  
    for (auto& GhostPair : GhostActorsMap)  
    {  
        if (GhostPair.Value.GhostActor)  
        {  
            GhostPair.Value.GhostActor->Destroy();  
        }  
    }  
    GhostActorsMap.Empty(); // Clear the map since we've destroyed all ghost actors  
}  