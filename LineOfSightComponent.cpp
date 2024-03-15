#include "LineOfSightComponent.h"      
#include "Engine/World.h"      
#include "GameFramework/PlayerController.h"      
#include "DrawDebugHelpers.h"      
#include "Components/PrimitiveComponent.h"    
#include "GameFramework/Character.h"      
#include "Engine/Engine.h"    
#include "IEnemyVisibilityInterface.h"  
#include "GhostActor.h"
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
    bIsLineOfSightEnabled = true;
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
    // Check if the component is enabled before performing the cone trace  
    if (!bIsLineOfSightEnabled)
    {
        return;
    }

    TSet<AActor*> CurrentlyVisibleEnemies;
    PerformVisibilityCheck(CurrentlyVisibleEnemies);
    UpdateVisibilityStates(CurrentlyVisibleEnemies);
    UpdateGhostActors(CurrentlyVisibleEnemies);
    VisibleEnemies = CurrentlyVisibleEnemies;
}

void ULineOfSightComponent::PerformVisibilityCheck(TSet<AActor*>& OutCurrentlyVisibleEnemies)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
    {
        return;
    }

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
                if (HitActor && HitActor->ActorHasTag(VisibleActorTag))
                {
                    if (HitActor->GetClass()->ImplementsInterface(UEnemyVisibilityInterface::StaticClass()))
                    {
                        OutCurrentlyVisibleEnemies.Add(HitActor);
                        IEnemyVisibilityInterface::Execute_SetVisible(HitActor, true);
                        UE_LOG(LogLineOfSightComponent, Log, TEXT("Actor %s is now VISIBLE"), *HitActor->GetName());
                    }
                    else
                    {
                        UE_LOG(LogLineOfSightComponent, Warning, TEXT("Actor %s does not implement IEnemyVisibilityInterface"), *HitActor->GetName());
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
}

void ULineOfSightComponent::UpdateVisibilityStates(const TSet<AActor*>& CurrentlyVisibleEnemies)
{
    // Actors that were visible but are no longer visible  
    for (AActor* PreviouslyVisibleEnemy : VisibleEnemies)
    {
        if (!CurrentlyVisibleEnemies.Contains(PreviouslyVisibleEnemy))
        {
            HandleActorVisibilityChange(PreviouslyVisibleEnemy, false);
        }
    }

    // Actors that are now visible but were not before  
    for (AActor* Actor : CurrentlyVisibleEnemies)
    {
        if (!VisibleEnemies.Contains(Actor))
        {
            HandleActorVisibilityChange(Actor, true);
        }
    }
}

void ULineOfSightComponent::HandleActorVisibilityChange(AActor* Actor, bool bIsNowVisible)
{
    // Check if the component is enabled before handling visibility changes  
    if (!bIsLineOfSightEnabled)
    {
        return;
    }

    if (bIsNowVisible)
    {
        IEnemyVisibilityInterface::Execute_SetVisible(Actor, true);
        UE_LOG(LogLineOfSightComponent, Log, TEXT("Actor %s is now VISIBLE"), *Actor->GetName());
        DestroyGhostActor(Actor); // Destroy ghost actor if the real actor is now visible  
    }
    else
    {
        IEnemyVisibilityInterface::Execute_SetVisible(Actor, false);
        UE_LOG(LogLineOfSightComponent, Log, TEXT("Actor %s is now HIDDEN"), *Actor->GetName());
        if (!GhostActorsMap.Contains(Actor))
        {
            SpawnGhostActor(Actor); // Spawn ghost actor if the real actor is now hidden  
        }
    }
}

void ULineOfSightComponent::SpawnGhostActor(AActor* EnemyActor)
{
    USkeletalMeshComponent* EnemySkeletalMesh = Cast<USkeletalMeshComponent>(EnemyActor->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
    if (EnemySkeletalMesh && EnemySkeletalMesh->SkeletalMesh)
    {
        AGhostActor* GhostActor = GetWorld()->SpawnActor<AGhostActor>(AGhostActor::StaticClass(), EnemySkeletalMesh->GetComponentTransform());
        UPoseableMeshComponent* GhostPoseableMesh = NewObject<UPoseableMeshComponent>(GhostActor);
        if (GhostPoseableMesh)
        {
            GhostActor->AddInstanceComponent(GhostPoseableMesh);
            GhostPoseableMesh->RegisterComponent();
            GhostPoseableMesh->SetSkeletalMesh(EnemySkeletalMesh->SkeletalMesh);
            GhostPoseableMesh->SetWorldTransform(EnemySkeletalMesh->GetComponentTransform());
            // Assume GhostMaterial is a valid UMaterialInterface*  
            GhostPoseableMesh->SetMaterial(0, GhostMaterial);
            GhostPoseableMesh->SetMaterial(1, GhostMaterial);
            GhostPoseableMesh->SetVisibility(true);
            GhostPoseableMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

            // Copy the pose from the enemy's skeletal mesh to the ghost's poseable mesh  
            const int32 NumBones = EnemySkeletalMesh->GetNumBones();
            for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
            {
                const FName BoneName = EnemySkeletalMesh->GetBoneName(BoneIndex);
                const FTransform BoneTransform = EnemySkeletalMesh->GetBoneTransform(BoneIndex);
                GhostPoseableMesh->SetBoneTransformByName(BoneName, BoneTransform, EBoneSpaces::WorldSpace);
            }

            FGhostInfo NewGhostInfo;
            NewGhostInfo.GhostActor = GhostActor;
            NewGhostInfo.TimeWhenHidden = GetWorld()->GetTimeSeconds();
            GhostActorsMap.Add(EnemyActor, NewGhostInfo);
        }
    }
}

void ULineOfSightComponent::UpdateGhostActors(const TSet<AActor*>& CurrentlyVisibleEnemies)
{
    // Check if the component is enabled before updating ghost actors  
    if (!bIsLineOfSightEnabled)
    {
        // If the component has been disabled, ensure all ghost actors are destroyed  
        for (auto& GhostPair : GhostActorsMap)
        {
            if (GhostPair.Value.GhostActor)
            {
                GhostPair.Value.GhostActor->Destroy();
            }
        }
        GhostActorsMap.Empty();
        return;
    }

    TArray<AActor*> GhostsToRemove;

    // Check if any ghosts should be destroyed because their corresponding enemy is visible again or the timer has expired  
    for (const auto& GhostPair : GhostActorsMap)
    {
        AActor* EnemyActor = GhostPair.Key;
        FGhostInfo GhostInfo = GhostPair.Value;

        if (CurrentlyVisibleEnemies.Contains(EnemyActor) || GetWorld()->GetTimeSeconds() - GhostInfo.TimeWhenHidden >= 2.0f)
        {
            GhostsToRemove.Add(EnemyActor);
        }
    }

    // Destroy ghosts that are no longer needed  
    for (AActor* EnemyActor : GhostsToRemove)
    {
        DestroyGhostActor(EnemyActor);
    }
}

void ULineOfSightComponent::DestroyGhostActor(AActor* EnemyActor)
{
    FGhostInfo* GhostInfo = GhostActorsMap.Find(EnemyActor);
    if (GhostInfo && GhostInfo->GhostActor)
    {
        GhostInfo->GhostActor->Destroy();
        GhostActorsMap.Remove(EnemyActor);
    }
}

void ULineOfSightComponent::DisableLineOfSightAndCleanup()
{
    // Disable the line of sight functionality  
    bIsLineOfSightEnabled = false;

    // Destroy all ghost actors  
    for (auto& GhostPair : GhostActorsMap)
    {
        if (GhostPair.Value.GhostActor)
        {
            GhostPair.Value.GhostActor->Destroy();
        }
    }
    GhostActorsMap.Empty(); // Clear the map since we've destroyed all ghost actors  
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
