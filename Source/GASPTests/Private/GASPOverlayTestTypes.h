// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "Actors/GASPCharacter.h"
#include "Animation/AnimInstance.h"
#include "Components/FoundryAbilitySystemComponent.h"
#include "Components/GASPInventoryPresenceComponent.h"
#include "Components/GASPOverlayComponent.h"
#include "Components/GASPPhysicsControlComponent.h"
#include "Data/FoundryInventoryItemDataAsset.h"
#include "GameFramework/FoundryAbilityPlayerState.h"
#include "GameFramework/GASPOverlayPresenceModifier.h"
#include "GameFramework/Fragments/ItemFragment_Equipment.h"
#include "Utils/GASPLinkedAnimInstanceSet.h"
#include "GASPOverlayTestTypes.generated.h"

class UChooserTable;

/**
 * Identifies the single-source layer in overlay reconciliation scenarios.
 */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPTestLayerA : public UAnimInstance
{

	GENERATED_BODY()

};

/**
 * Identifies the combined-source or higher-priority layer in overlay scenarios.
 */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPTestLayerB : public UAnimInstance
{

	GENERATED_BODY()

};

/**
 * Records engine layer effects while exercising production source ownership and reconciliation.
 */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPTestOverlayComponent : public UGASPOverlayComponent
{

	GENERATED_BODY()

public:

	TArray<TSubclassOf<UAnimInstance>> LinkedLayers;
	TArray<TSubclassOf<UAnimInstance>> UnlinkedLayers;
	TSubclassOf<UAnimInstance> SingleOverlayLayer = UGASPTestLayerA::StaticClass();
	TSubclassOf<UAnimInstance> CombinedOverlayLayer = UGASPTestLayerB::StaticClass();
	TSubclassOf<UAnimInstance> PoseLayer;
	mutable int32 OverlayEvaluationCount = 0;
	int32 OverlayChangeCount = 0;
	bool bReenterOnChange = false;
	bool bUseConfiguredChooser = false;

	UPROPERTY(Transient)
	TObjectPtr<UAnimInstance> AnimationInstance;

	UPROPERTY(Transient)
	TObjectPtr<UObject> ReentrantSource;

	/**
	 * Begins recording effective overlay changes for the current scenario.
	 */
	void BindOverlayChanges();

	/**
	 * Clears recorded effects after the shared scenario setup has completed.
	 */
	void ResetRecordedChanges();

	/**
	 * Ends component gameplay so scenarios can verify teardown and rejected late contributions.
	 */
	void SimulateEndPlay();

protected:

	// -- Begin GASPOverrideModeManager implementation
	virtual UAnimInstance* ResolveAnimationInstance() const override;
	virtual void LinkLayer(UAnimInstance* Instance, TSubclassOf<UAnimInstance> Layer) override;
	virtual void UnlinkLayer(UAnimInstance* Instance, TSubclassOf<UAnimInstance> Layer) override;
	// -- End GASPOverrideModeManager implementation

	// -- Begin GASPOverlayComponent implementation
	virtual TSubclassOf<UAnimInstance> ResolveOverlayLayer() const override;
	virtual TSubclassOf<UAnimInstance> ResolvePoseLayer() const override;
	// -- End GASPOverlayComponent implementation

private:

	/** Records notifications and optionally registers one contribution from the listener. */
	UFUNCTION()
	void HandleOverlayChanged(FGameplayTagContainer PreviousTags, FGameplayTagContainer CurrentTags);

};

/**
 * Keeps presentation scenarios independent of authored physics assets and control simulation.
 */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPTestPhysicsControlComponent : public UGASPPhysicsControlComponent
{

	GENERATED_BODY()

protected:

	// -- Begin ActorComponent implementation
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// -- End ActorComponent implementation

};

/**
 * Supplies transient settings and exposes inherited Pawn lifecycle actions to overlay scenarios.
 *
 * Attribute replication and physics simulation are isolated so these presentation and owner/avatar
 * tests do not depend on authored Attribute Set profiles or physics assets.
 */
UCLASS(Hidden, NotPlaceable, NotBlueprintable, NotBlueprintType)
class AGASPTestCharacter : public AGASPCharacter
{

	GENERATED_BODY()

public:

	AGASPTestCharacter(const FObjectInitializer& ObjectInitializer);

	/**
	 * Starts the character's inherited Crucible death handling.
	 */
	void StartDeath();

	/**
	 * Sets the transient settings' overlay Chooser for the current scenario.
	 */
	void SetOverlayChooser(UChooserTable* Chooser);

	/**
	 * Checks whether death has disabled Mover input production.
	 */
	bool IsMovementDisabledByDeath() const;

	/**
	 * Assigns a Player State and applies its ASC owner/avatar handoff.
	 */
	void SetExternalAbilitySystemOwner(AFoundryAbilityPlayerState* State);

};

/**
 * Allows scenarios to author tags on a transient Presence modifier.
 */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPTestPresenceModifier : public UGASPOverlayPresenceModifier
{

	GENERATED_BODY()

public:

	/**
	 * Sets the overlay tags contributed while this modifier is active.
	 */
	void SetTags(const FGameplayTagContainer& Tags);

};

/**
 * Exercises owner/avatar handoff without loading an authored ASC profile.
 */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPTestAbilitySystemComponent : public UFoundryAbilitySystemComponent
{

	GENERATED_BODY()

public:

	// -- Begin AbilitySystemComponent implementation
	virtual void InitAbilityActorInfo(AActor* Owner, AActor* Avatar) override;
	// -- End AbilitySystemComponent implementation

};

/**
 * Supplies a persistent external ASC for Pawn association scenarios.
 */
UCLASS(Hidden, NotPlaceable, NotBlueprintable, NotBlueprintType)
class AGASPTestPlayerState : public AFoundryAbilityPlayerState
{

	GENERATED_BODY()

public:

	AGASPTestPlayerState(const FObjectInitializer& ObjectInitializer);

};

/**
 * Configures a transient Chooser result with an animation layer class.
 */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPTestLinkedSet : public UGASPLinkedAnimInstanceSet
{

	GENERATED_BODY()

public:

	/**
	 * Sets the layer selected when this linked set is returned by a Chooser.
	 */
	void SetLayer(TSubclassOf<UAnimInstance> Layer);

};

/**
 * Configures a transient Item's Equipped Presence for integration scenarios.
 */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPTestEquipmentFragment : public UItemFragment_Equipment
{

	GENERATED_BODY()

public:

	/**
	 * Sets the single modifier applied while the Item is equipped.
	 */
	void SetEquippedModifier(UFoundryInventoryPresenceModifier* Modifier);

};

/**
 * Rebuilds a transient Item definition after assigning its Equipment fragment.
 */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPTestItemDefinition : public UFoundryInventoryItemDataAsset
{

	GENERATED_BODY()

public:

	/**
	 * Sets the Equipment fragment and refreshes the definition's authored fragment cache.
	 */
	void SetEquipmentFragment(UItemFragment_Equipment* Fragment);

};

/**
 * Simulates Presence transport delivery without creating a private Inventory for an observer.
 */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPTestPresenceComponent : public UGASPInventoryPresenceComponent
{

	GENERATED_BODY()

public:

	/**
	 * Sets whether the component should discover an owner Inventory automatically.
	 */
	void SetAutomaticAssociation(bool bAutomatic);

	/**
	 * Replaces the received transport snapshot and invokes production Presence reconciliation.
	 */
	void ReceivePresences(const TArray<FInventoryPresenceContext>& Contexts);

};

/**
 * Supplies a concrete source identity whose lifetime can be controlled by overlay scenarios.
 */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPTestOverlaySource : public UObject
{

	GENERATED_BODY()

};
