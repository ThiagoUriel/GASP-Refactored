// Ninja Bear Studio Inc., all rights reserved.
#include "GASPOverlayTestTypes.h"

#include "Chooser.h"
#include "Components/FoundryAttributeReplicationComponent.h"
#include "Settings/GASPCharacterSettings.h"
#include "Types/TagTypes.h"
#include "UObject/UnrealType.h"

AGASPTestCharacter::AGASPTestCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
		.DoNotCreateDefaultSubobject(InteractionComponentName)
		.SetDefaultSubobjectClass<UGASPTestOverlayComponent>(OverlayComponentName)
		.SetDefaultSubobjectClass<UGASPTestPhysicsControlComponent>(PhysicsControlComponentName))
{
	Settings = CreateDefaultSubobject<UGASPCharacterSettings>(TEXT("TestSettings"));
	AutoPossessAI = EAutoPossessAI::Disabled;
	RegisteredStimuliSources.Reset();

	// These scenarios exercise presentation and ASC handoff, without authored Attribute profiles.
	if (IsValid(AttributeReplicationComponent))
	{
		AttributeReplicationComponent->SetIsReplicated(false);
	}
}

AGASPTestPlayerState::AGASPTestPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGASPTestAbilitySystemComponent>(AbilitySystemComponentName))
{
}

UAnimInstance* UGASPTestOverlayComponent::ResolveAnimationInstance() const
{
	return AnimationInstance;
}

void UGASPTestOverlayComponent::LinkLayer(UAnimInstance* Instance, TSubclassOf<UAnimInstance> Layer)
{
	LinkedLayers.Add(Layer);
}

void UGASPTestOverlayComponent::UnlinkLayer(UAnimInstance* Instance, TSubclassOf<UAnimInstance> Layer)
{
	UnlinkedLayers.Add(Layer);
}

TSubclassOf<UAnimInstance> UGASPTestOverlayComponent::ResolveOverlayLayer() const
{
	++OverlayEvaluationCount;
	if (bUseConfiguredChooser)
	{
		return Super::ResolveOverlayLayer();
	}

	const FGameplayTagContainer EffectiveTags = GetOverlayMode();
	const bool bHasCombinedOverlay = EffectiveTags.HasTagExact(OverlayModeTags::Rifle) && EffectiveTags.HasTagExact(OverlayModeTags::Torch);
	return bHasCombinedOverlay ? CombinedOverlayLayer : SingleOverlayLayer;
}

TSubclassOf<UAnimInstance> UGASPTestOverlayComponent::ResolvePoseLayer() const
{
	return PoseLayer;
}

void UGASPTestOverlayComponent::BindOverlayChanges()
{
	OnOverlayChanged.AddUniqueDynamic(this, &ThisClass::HandleOverlayChanged);
}

void UGASPTestOverlayComponent::ResetRecordedChanges()
{
	LinkedLayers.Reset();
	UnlinkedLayers.Reset();
	OverlayEvaluationCount = 0;
	OverlayChangeCount = 0;
}

void UGASPTestOverlayComponent::SimulateEndPlay()
{
	if (!HasBegunPlay())
	{
		BeginPlay();
	}

	EndPlay(EEndPlayReason::Destroyed);
}

void UGASPTestOverlayComponent::HandleOverlayChanged(FGameplayTagContainer PreviousTags, FGameplayTagContainer CurrentTags)
{
	++OverlayChangeCount;
	if (bReenterOnChange)
	{
		bReenterOnChange = false;
		SetOverlayContribution(ReentrantSource, TEXT("Reentrant"), OverlayModeTags::Torch.GetTag().GetSingleTagContainer());
	}
}

void UGASPTestPhysicsControlComponent::BeginPlay()
{
	// Keep the engine component lifecycle without starting GASP's asset-backed control setup.
	UPhysicsControlComponent::BeginPlay();
}

void UGASPTestPhysicsControlComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UPhysicsControlComponent::EndPlay(EndPlayReason);
}

void AGASPTestCharacter::StartDeath()
{
	HandleDeathStarted();
}

void AGASPTestCharacter::SetOverlayChooser(UChooserTable* Chooser)
{
	Settings->OverlayTable = Chooser;
}

bool AGASPTestCharacter::IsMovementDisabledByDeath() const
{
	return bMovementDisabledByDeath;
}

void AGASPTestCharacter::SetExternalAbilitySystemOwner(AFoundryAbilityPlayerState* State)
{
	SetPlayerState(State);
	OnRep_PlayerState();
}

void UGASPTestPresenceModifier::SetTags(const FGameplayTagContainer& Tags)
{
	OverlayTags = Tags;
}

void UGASPTestAbilitySystemComponent::InitAbilityActorInfo(AActor* Owner, AActor* Avatar)
{
	UAbilitySystemComponent::InitAbilityActorInfo(Owner, Avatar);
}

void UGASPTestLinkedSet::SetLayer(TSubclassOf<UAnimInstance> Layer)
{
	LinkedAnimInstance = Layer;
}

void UGASPTestEquipmentFragment::SetEquippedModifier(UFoundryInventoryPresenceModifier* Modifier)
{
	EquippedPresence.Modifiers.Reset();
	EquippedPresence.Modifiers.Add(Modifier);
}

void UGASPTestItemDefinition::SetEquipmentFragment(UItemFragment_Equipment* Fragment)
{
	Fragments.Reset();
	Fragments.Add(Fragment);

	FPropertyChangedEvent ChangedFragments(FindFProperty<FProperty>(UFoundryInventoryItemDataAsset::StaticClass(), TEXT("Fragments")));
	PostEditChangeProperty(ChangedFragments);
}

void UGASPTestPresenceComponent::SetAutomaticAssociation(bool bAutomatic)
{
	bInitializeFromOwnerInventory = bAutomatic;
}

void UGASPTestPresenceComponent::ReceivePresences(const TArray<FInventoryPresenceContext>& Contexts)
{
	// This test-only accessor simulates replication into the component's owned snapshot. Keep
	// transport mutation here rather than adding a runtime setter exclusively for automation.
	//
	FInventoryPresenceReplicationList& Transport = const_cast<FInventoryPresenceReplicationList&>(GetReplicatedPresences());
	Transport.Reset();
	for (const FInventoryPresenceContext& Context : Contexts)
	{
		Transport.UpsertPresence(Context);
	}

	SynchronizeReplicatedPresences();
}
