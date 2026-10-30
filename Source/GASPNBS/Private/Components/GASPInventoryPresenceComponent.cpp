// Ninja Bear Studio Inc., all rights reserved.
#include "Components/GASPInventoryPresenceComponent.h"

#include "Blueprint/FoundryGameplayAbilitiesFunctionLibrary.h"
#include "Components/GASPOverlayComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/AbilitySystemComponentProviderInterface.h"
#include "Misc/ScopeExit.h"
#include "Subsystems/FoundryInventorySubsystem.h"

void UGASPInventoryPresenceComponent::BeginPlay()
{
	Super::BeginPlay();

	BindAbilitySystemProvider();
	TryInitializeOwnerInventory();
}

void UGASPInventoryPresenceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindAbilitySystemProvider();
	Super::EndPlay(EndPlayReason);
}

void UGASPInventoryPresenceComponent::SynchronizeReplicatedPresences()
{
	UGASPOverlayComponent* Overlay = ResolveOverlayComponent();
	if (IsValid(Overlay))
	{
		Overlay->BeginOverlayUpdate();
	}

	ON_SCOPE_EXIT
	{
		if (IsValid(Overlay))
		{
			Overlay->EndOverlayUpdate();
		}
	};

	Super::SynchronizeReplicatedPresences();
}

UGASPOverlayComponent* UGASPInventoryPresenceComponent::ResolveOverlayComponent() const
{
	const AActor* Owner = GetOwner();
	return IsValid(Owner) ? Owner->FindComponentByClass<UGASPOverlayComponent>() : nullptr;
}

void UGASPInventoryPresenceComponent::BindAbilitySystemProvider()
{
	UnbindAbilitySystemProvider();

	// The persistent Inventory may exist before this Pawn receives its Player State. Observe
	// availability without creating a lazy ASC or waiting for an unrelated Item event.
	//
	AActor* ProviderActor = UFoundryGameplayAbilitiesFunctionLibrary::FindAbilitySystemProvider(GetOwner());
	IAbilitySystemComponentProviderInterface* Provider = Cast<IAbilitySystemComponentProviderInterface>(ProviderActor);
	if (Provider != nullptr)
	{
		AbilitySystemProvider = ProviderActor;
		AbilitySystemChangedHandle = Provider->OnAbilitySystemComponentChanged().AddUObject(this, &ThisClass::HandleAbilitySystemChanged);
	}
}

void UGASPInventoryPresenceComponent::UnbindAbilitySystemProvider()
{
	IAbilitySystemComponentProviderInterface* Provider = Cast<IAbilitySystemComponentProviderInterface>(AbilitySystemProvider.Get());
	if (Provider != nullptr)
	{
		Provider->OnAbilitySystemComponentChanged().Remove(AbilitySystemChangedHandle);
	}

	AbilitySystemChangedHandle.Reset();
	AbilitySystemProvider.Reset();
}

void UGASPInventoryPresenceComponent::TryInitializeOwnerInventory()
{
	const AActor* Owner = GetOwner();
	if (!bInitializeFromOwnerInventory || !IsValid(Owner) || !Owner->HasAuthority())
	{
		return;
	}

	const FInventoryHandle Inventory = ResolveOwnerInventory();
	if (Inventory.IsValid() && Inventory != GetInventoryHandle())
	{
		InitializePresence(Inventory);
	}
}

FInventoryHandle UGASPInventoryPresenceComponent::ResolveOwnerInventory() const
{
	const AActor* Owner = GetOwner();
	const UFoundryInventorySubsystem* Inventories = UFoundryInventorySubsystem::Get(this);
	if (!IsValid(Owner) || !IsValid(Inventories))
	{
		return FInventoryHandle();
	}

	FInventoryHandle Inventory = Inventories->FindInventoryByOwner(Owner);
	if (!Inventory.IsValid())
	{
		const APawn* Pawn = Cast<APawn>(Owner);
		if (IsValid(Pawn) && IsValid(Pawn->GetPlayerState()))
		{
			Inventory = Inventories->FindInventoryByOwner(Pawn->GetPlayerState());
		}
	}

	return Inventory;
}

void UGASPInventoryPresenceComponent::HandleAbilitySystemChanged(UAbilitySystemComponent* Previous, UAbilitySystemComponent* Current)
{
	TryInitializeOwnerInventory();
}
