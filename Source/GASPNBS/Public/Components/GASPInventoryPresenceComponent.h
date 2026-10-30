// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "Components/FoundryInventoryPresenceComponent.h"
#include "GASPInventoryPresenceComponent.generated.h"

class UAbilitySystemComponent;
class UGASPOverlayComponent;

/**
 * Batches each Foundry Presence reconciliation into one GASP overlay update.
 *
 * Automatic association also observes ASC availability so a persistent Player State Inventory
 * can be discovered after the Pawn begins play. Explicit Inventory association remains supported.
 */
UCLASS(Blueprintable, ClassGroup = (GASP), meta = (BlueprintSpawnableComponent))
class GASPNBS_API UGASPInventoryPresenceComponent : public UFoundryInventoryPresenceComponent
{

	GENERATED_BODY()

public:

	// -- Begin ActorComponent implementation
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// -- End ActorComponent implementation

	// -- Begin FoundryInventoryPresenceComponent implementation
	virtual void SynchronizeReplicatedPresences() override;
	// -- End FoundryInventoryPresenceComponent implementation

protected:

	/**
	 * Provides the local overlay component whose updates should be batched during reconciliation.
	 */
	virtual UGASPOverlayComponent* ResolveOverlayComponent() const;

	/**
	 * Binds to the owner's current ASC provider without creating a lazy ASC.
	 */
	void BindAbilitySystemProvider();

	/**
	 * Removes the availability binding from the exact provider that created it.
	 */
	void UnbindAbilitySystemProvider();

	/**
	 * Attempts automatic association on authority when an owner Inventory becomes available.
	 */
	virtual void TryInitializeOwnerInventory();

	/**
	 * Provides an Inventory owned by the visual Actor or its Player State, when registered.
	 */
	virtual FInventoryHandle ResolveOwnerInventory() const;

	/**
	 * Retries automatic Inventory association after the ASC provider changes availability.
	 *
	 * @param Previous	Previously available ASC, or null.
	 * @param Current	Currently available ASC, or null.
	 */
	virtual void HandleAbilitySystemChanged(UAbilitySystemComponent* Previous, UAbilitySystemComponent* Current);

private:

	FDelegateHandle AbilitySystemChangedHandle;
	TWeakObjectPtr<AActor> AbilitySystemProvider;

};
