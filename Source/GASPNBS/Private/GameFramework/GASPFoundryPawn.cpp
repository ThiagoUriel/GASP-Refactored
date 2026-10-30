// Ninja Bear Studio Inc., all rights reserved.
#include "GameFramework/GASPFoundryPawn.h"

#include "GASPNBSTags.h"
#include "FoundryInteractionTags.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/FoundryInteractionComponent.h"
#include "Components/GASPActionComponent.h"
#include "Components/GASPInteractionTargetComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/FoundryInputSetupDataAsset.h"
#include "Components/GASPTraversalComponent.h"
#include "TimerManager.h"
#include "Types/TagTypes.h"
#include "Types/FoundryInteractionCollisions.h"

const FName AGASPFoundryPawn::InteractionTargetComponentName = TEXT("InteractionTargetComponent");

AGASPFoundryPawn::AGASPFoundryPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	AbilitySystemProfileTag = GASP::Interaction::AbilityProfile;
	InteractionTargetComponent = CreateDefaultSubobject<UGASPInteractionTargetComponent>(InteractionTargetComponentName);
	InteractionScan = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionScan"));
	InteractionScan->SetupAttachment(GetCapsuleComponent());
	InteractionScan->InitSphereRadius(400.f);
	InteractionScan->ComponentTags.Add(NBS::Interaction::Components::InteractableScan.GetTag().GetTagName());
	InteractionScan->SetCollisionProfileName(NBS::Interaction::Collision::InteractionScanner);
	InteractionScan->SetGenerateOverlapEvents(true);

	InteractionTargetCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionTargetCollision"));
	InteractionTargetCollision->SetupAttachment(GetCapsuleComponent());
	InteractionTargetCollision->InitSphereRadius(60.f);
	InteractionTargetCollision->SetCollisionProfileName(NBS::Interaction::Collision::InteractionTarget);
	InteractionTargetCollision->SetGenerateOverlapEvents(true);
}

void AGASPFoundryPawn::BeginPlay()
{
	Super::BeginPlay();
	if (UFoundryInteractionComponent* Performer = GetInteractionComponent())
	{
		Performer->OnInteractionFinished.AddUniqueDynamic(this, &ThisClass::HandleInteractionFinished);
	}
	
	RefreshInteractionScanning();
}

void AGASPFoundryPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopInputJump();
	
	if (UFoundryInteractionComponent* Performer = GetInteractionComponent())
	{
		Performer->CancelInteraction();
		Performer->OnInteractionFinished.RemoveDynamic(this, &ThisClass::HandleInteractionFinished);
	}
	
	if (UGASPActionComponent* Actions = GetActionComponent())
	{
		Actions->CancelReservedAction();
	}
	
	Super::EndPlay(EndPlayReason);
}

void AGASPFoundryPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	RefreshInteractionScanning();
}

void AGASPFoundryPawn::UnPossessed()
{
	StopInputJump();
	Super::UnPossessed();
	RefreshInteractionScanning();
}

UAnimInstance* AGASPFoundryPawn::GetInteractionAnimContext_Implementation() const
{
	return IsValid(GetMesh()) ? GetMesh()->GetAnimInstance() : nullptr;
}

FPoseSearchBlueprintResult AGASPFoundryPawn::GetInteractionResult_Implementation() const
{
	const UGASPActionComponent* Actions = GetActionComponent();
	return IsValid(Actions) ? Actions->GetActionResult() : FPoseSearchBlueprintResult();
}

bool AGASPFoundryPawn::TryTraversalJumpAction_Implementation(const FFoundryInputHandlerContext& Context)
{
	if (Context.Pawn != this || !Context.Value.Get<bool>() || !CanStartInputJump() || IsRagdolling())
	{
		return false;
	}

	// Retain the Pawn's movement-dependent trace policy and the component's execution path.
	const FTraversalResult Result = TryTraversalAction();
	return !Result.bTraversalCheckFailed && !Result.bMontageSelectionFailed;
}

TArray<UFoundryInputSetupDataAsset*> AGASPFoundryPawn::GetInputSetups_Implementation() const
{
	return InputSetups;
}

void AGASPFoundryPawn::HandleDeathStarted()
{
	StopInputJump();
	if (UFoundryInteractionComponent* Performer = GetInteractionComponent())
	{
		Performer->CancelInteraction();
	}
	
	if (UGASPActionComponent* Actions = GetActionComponent())
	{
		Actions->CancelReservedAction();
	}
	
	Super::HandleDeathStarted();
}

bool AGASPFoundryPawn::CanStartInputJump() const
{
	return IsLocallyControlled() && !bMovementDisabledByDeath
		&& (IsRagdolling() || (!IsDoingTraversal() && IsValid(GetMesh()) && !GetMesh()->IsPlayingRootMotion()));
}

void AGASPFoundryPawn::StartInputJump()
{
	if (!CanStartInputJump())
	{
		return;
	}

	if (IsRagdolling())
	{
		StopRagdolling();
		return;
	}

	FGASPInputState* InputState = PlayerInputState.GetMutablePtr<FGASPInputState>();
	if (!InputState)
	{
		return;
	}
	if (GetStanceMode() != StanceTags::Standing)
	{
		InputState->DesiredStance = StanceTags::Standing;
		return;
	}

	Jump();
	GetWorldTimerManager().SetTimer(InputJumpTimer, this, &ThisClass::StopInputJump, 0.1f, false);
}

void AGASPFoundryPawn::StopInputJump()
{
	GetWorldTimerManager().ClearTimer(InputJumpTimer);
	StopJumping();
}

void AGASPFoundryPawn::RequestAction(const FName ActionType)
{
	if (!HasAuthority())
	{
		Server_RequestAction(ActionType);
		return;
	}
	
	FGameplayTag Event;
	if (ActionType == TEXT("Shove"))
	{
		Event = GASP::Interaction::ExecuteShove;
	}
	else if (ActionType == TEXT("Takedown"))
	{
		Event = GASP::Interaction::ExecuteTakedown;
	}
	
	if (!Event.IsValid())
	{
		return;
	}
	
	UAbilitySystemComponent* Abilities = GetAbilitySystemComponent();
	if (IsValid(Abilities))
	{
		FGameplayEventData Payload;
		Payload.EventTag = Event;
		Payload.Instigator = this;
		Abilities->HandleGameplayEvent(Event, &Payload);
	}
}

bool AGASPFoundryPawn::TrackInteractionAction(const FGASPActionHandle Handle)
{
	const UFoundryInteractionComponent* Performer = GetInteractionComponent();
	const FGASPActionHandle Active = IsValid(Performer)
		? FGASPActionHandle::FromExecution(Performer->GetActiveInteractionContext().ExecutionId)
		: FGASPActionHandle();
	
	if (!Handle.IsValid() || Handle != Active)
	{
		return false;
	}
	
	InteractionActionHandle = Handle;
	return true;
}

void AGASPFoundryPawn::RefreshInteractionScanning()
{
	if (!HasAuthority())
	{
		return;
	}
	
	const bool bShouldScan = IsPlayerControlled() || bScanWithoutPlayer;
	if (bShouldScan)
	{
		EnsureAbilitySystemComponent();
	}
	
	const ECollisionEnabled::Type Collision = bShouldScan ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision;
	InteractionScan->SetCollisionEnabled(Collision);
}

void AGASPFoundryPawn::Server_RequestAction_Implementation(const FName ActionType)
{
	RequestAction(ActionType);
}

void AGASPFoundryPawn::HandleInteractionFinished(UFoundryInteractionComponent* Component, FInteractionDescriptor Interaction, FInteractionResult Result)
{
	const FGASPActionHandle Handle = InteractionActionHandle;
	InteractionActionHandle = FGASPActionHandle();
	if (UGASPActionComponent* Actions = GetActionComponent())
	{
		Actions->CancelAction(Handle);
	}
}
