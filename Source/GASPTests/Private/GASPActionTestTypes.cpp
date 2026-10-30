// Ninja Bear Studio Inc., all rights reserved.
#include "GASPActionTestTypes.h"

#include "GASPOverlayTestTypes.h"
#include "Components/FoundryAttributeReplicationComponent.h"
#include "PoseSearch/PoseSearchInteractionAsset.h"
#include "Settings/GASPCharacterSettings.h"
#include "GameFramework/GameplayCameraComponent.h"

void UGASPInteractionTestObserver::HandleFinished(UFoundryInteractionComponent* Component, FInteractionDescriptor Interaction, FInteractionResult Result)
{
	LastResult = Result;
	++CompletionCount;
}

AGASPActionTestPawn::AGASPActionTestPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
		.SetDefaultSubobjectClass<UGASPTestActionComponent>(ActionComponentName)
		.SetDefaultSubobjectClass<UGASPTestPhysicsControlComponent>(PhysicsControlComponentName))
{
	Settings = CreateDefaultSubobject<UGASPCharacterSettings>(TEXT("TestSettings"));
	AutoPossessAI = EAutoPossessAI::Disabled;
	bScanWithoutPlayer = true;
	RegisteredStimuliSources.Reset();
	PrimaryActorTick.bCanEverTick = false;
	if (UGameplayCameraComponent* Camera = Cast<UGameplayCameraComponent>(GetDefaultSubobjectByName(TEXT("GameplayCamera"))))
	{
		Camera->SetAutoActivate(false);
	}
	if (IsValid(AttributeReplicationComponent))
	{
		AttributeReplicationComponent->SetIsReplicated(false);
	}
}

bool UGASPTestActionComponent::SelectAction(FName ActionType, AActor* Target, TArray<FGASPInteractionRep>& OutRoles) const
{
	if (!IsValid(Performance) || Performance->Items.Num() != 2)
	{
		return false;
	}
	for (int32 Index = 0; Index < Performance->Items.Num(); ++Index)
	{
		FGASPInteractionRep& Role = OutRoles.AddDefaulted_GetRef();
		Role.Actor = Index == 0 ? GetOwner() : Target;
		Role.Result.SelectedAnim = Performance;
		Role.Result.bIsInteraction = true;
		Role.Result.RoleIndex = Index;
		Role.Result.Role = Performance->Items[Index].Role;
		Role.Result.ActorRootTransforms.Add(GetOwner()->GetActorTransform());
		Role.Result.ActorRootTransforms.Add(Target->GetActorTransform());
		Role.Result.ActorRootBoneTransforms = Role.Result.ActorRootTransforms;
	}
	return true;
}

bool UGASPTestActionComponent::CanSelectTarget(FName ActionType, const AGASPCharacter* Source, const AGASPCharacter* Target) const
{
	// Geometry and grounded movement are covered by the authored-sample scenario, not this playback fixture.
	return true;
}
