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

void AGASPFoundryPawn::HandleDeathStarted()
{
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
