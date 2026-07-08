// Ninja Bear Studio Inc., all rights reserved.
#include "GameFramework/GASPAbilitiesCharacter.h"

#include "AbilitySystemGlobals.h"
#include "TimerManager.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFramework/PlayerState.h"

FName AGASPAbilitiesCharacter::AbilitySystemComponentName = TEXT("AbilitySystemComponent");

AGASPAbilitiesCharacter::AGASPAbilitiesCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bReplicates = true;
	bInitializeAbilityComponentOnBeginPlay = true;
	NetPriority = 2.f;
	SetMinNetUpdateFrequency(11.f);
	SetNetUpdateFrequency(33.f);
	
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AbilityReplicationMode = EGameplayEffectReplicationMode::Minimal;
	
	CharacterAbilities = CreateOptionalDefaultSubobject<UAbilitySystemComponent>(AbilitySystemComponentName);
	if (IsValid(CharacterAbilities))
	{
		CharacterAbilities->SetIsReplicated(bReplicates);
		CharacterAbilities->SetReplicationMode(AbilityReplicationMode);	
	}
}

void AGASPAbilitiesCharacter::PostInitProperties()
{
	Super::PostInitProperties();
	
	if (IsValid(CharacterAbilities))
	{
		// Set the Replication Mode after properties are initialized but before components.
		// This way, once the Ability System Component initializes, it has the correct value.
		//
		CharacterAbilities->SetReplicationMode(AbilityReplicationMode);
	}	
}

void AGASPAbilitiesCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AGASPAbilitiesCharacter::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::BeginPlay();
	
	if (bInitializeAbilityComponentOnBeginPlay)
	{
		// This reinforces the ASC instance in this class, in case it was provided by a Game Feature.
		//
		// Doing this is useful as it makes this class compatible with both a gameplay feature and the
		// ASC interface, avoiding the component lookup. We'll always have the cached component.
		//
		CharacterAbilities = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(this);
		if (IsValid(CharacterAbilities))
		{
			SetupAbilitySystemComponent(this);
		}
	}
}

void AGASPAbilitiesCharacter::SetupAbilitySystemComponent(AActor* AbilitySystemOwner)
{
	if (IsValid(CharacterAbilities))
	{
		CharacterAbilities->InitAbilityActorInfo(AbilitySystemOwner, this);
	}
}

void AGASPAbilitiesCharacter::InitializeFromPlayerState()
{
	APlayerState* MyState = GetPlayerState<APlayerState>();
	if (!IsValid(MyState))
	{
		// We'll keep trying on next tick until the Player State replicates.
		// Return fast so nothing else will be done for now (including subclasses).
		//
		GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::InitializeFromPlayerState);
		return;
	}
	
	NetPriority = MyState->NetPriority;
	
	const float NewMinNetUpdateFrequency = MyState->GetMinNetUpdateFrequency();
	SetMinNetUpdateFrequency(NewMinNetUpdateFrequency);

	const float NewNetUpdateFrequency = MyState->GetNetUpdateFrequency(); 
	SetNetUpdateFrequency(NewNetUpdateFrequency);
	
	SetupAbilitySystemComponent(MyState);
}

void AGASPAbilitiesCharacter::ClearAbilitySystemComponent()
{
	if (IsValid(CharacterAbilities))
	{
		CharacterAbilities->ClearActorInfo();
	}	
}

UAbilitySystemComponent* AGASPAbilitiesCharacter::GetAbilitySystemComponent() const
{
	return CharacterAbilities;
}

void AGASPAbilitiesCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}