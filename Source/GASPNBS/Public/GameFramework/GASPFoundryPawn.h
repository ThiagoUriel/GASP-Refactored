// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "GASPCharacterExample.h"
#include "Interaction/GASPActionTypes.h"
#include "Interfaces/GASPInteractionInterface.h"
#include "Interfaces/InputSetupProviderInterface.h"
#include "Interfaces/TraversalMovementInputInterface.h"
#include "Types/FInteractionCoreTypes.h"
#include "GASPFoundryPawn.generated.h"

class UFoundryInteractionComponent;
class UFoundryInteractionTargetComponent;
class UFoundryInputSetupDataAsset;
class USphereComponent;

/**
 * Keeps the sample camera and input behavior while Foundry owns interaction selection and execution.
 *
 * The performer component comes from Crucible. This Pawn adds authored target definitions and uses
 * the sample's named GASP Action Component for synchronized playback. It also owns cleanup when a
 * Foundry execution ends, including prepared actions that never reach Commit.
 */
UCLASS(Blueprintable)
class GASPNBS_API AGASPFoundryPawn : public AGASPCharacterExample, public IGASPInteractionInterface, public ITraversalMovementInputInterface, public IInputSetupProviderInterface
{

	GENERATED_BODY()

public:

	static const FName InteractionTargetComponentName;

	AGASPFoundryPawn(const FObjectInitializer& ObjectInitializer);

	// -- Begin Actor implementation
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// -- End Actor implementation

	// -- Begin Pawn implementation
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	// -- End Pawn implementation

	// -- Begin GASPInteractionInterface implementation
	virtual UAnimInstance* GetInteractionAnimContext_Implementation() const override;
	virtual FPoseSearchBlueprintResult GetInteractionResult_Implementation() const override;
	// -- End GASPInteractionInterface implementation

	// -- Begin TraversalMovementInputInterface implementation
	virtual bool TryTraversalJumpAction_Implementation(const FFoundryInputHandlerContext& Context) override;
	// -- End TraversalMovementInputInterface implementation

	// -- Begin InputSetupProviderInterface implementation
	virtual TArray<UFoundryInputSetupDataAsset*> GetInputSetups_Implementation() const override;
	// -- End InputSetupProviderInterface implementation

	/** Reports whether GASP currently uses its separate twin-stick aim direction. */
	UFUNCTION(BlueprintPure, Category = "GASP|Input")
	bool IsTwinStickInputEnabled() const { return TwinStickMode; }

	/** Checks local ownership and movement restrictions before accepting a jump request. */
	UFUNCTION(BlueprintPure, Category = "GASP|Input")
	bool CanStartInputJump() const;

	/**
	 * Handles the non-traversal jump request: recover, stand, or issue a short Mover jump pulse.
	 * The Pawn owns the reset timer so shared Input Handlers retain no player state.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Input")
	void StartInputJump();

	/** Releases the Mover jump pulse and cancels its pending reset. */
	UFUNCTION(BlueprintCallable, Category = "GASP|Input")
	void StopInputJump();

	/**
	 * Provides this Pawn's authored target definitions and default GASP action Handlers.
	 */
	UFUNCTION(BlueprintPure, Category = "GASP|Interaction")
	UFoundryInteractionTargetComponent* GetInteractionTargetComponent() const { return InteractionTargetComponent; }

	/**
	 * Routes a known sample input action through Foundry's granted Execute Ability.
	 *
	 * @param ActionType	Shove or Takedown; authority chooses the target from its current scanner state.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Interaction")
	void RequestAction(FName ActionType);

	/**
	 * Associates a GASP action with the current accepted Foundry execution for cleanup.
	 *
	 * @param Handle	Action corresponding to the performer's current execution identity.
	 * @return			Whether the current Foundry execution owns this identity.
	 */
	bool TrackInteractionAction(FGASPActionHandle Handle);

protected:

	/** Setups registered by the Local Player while this Pawn is possessed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASP|Input")
	TArray<TObjectPtr<UFoundryInputSetupDataAsset>> InputSetups;

	/** Allows an AI-controlled Pawn to discover interactions in addition to offering its target slots. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	bool bScanWithoutPlayer = false;

	// -- Begin CruciblePawn implementation
	virtual void HandleDeathStarted() override;
	// -- End CruciblePawn implementation

	/**
	 * Enables discovery for players and opted-in AI, including possession changes after BeginPlay.
	 *
	 * AI subclasses can enable bScanWithoutPlayer in their defaults. Target slots remain available
	 * regardless of whether this Pawn is currently scanning for its own interactions.
	 */
	void RefreshInteractionScanning();

private:

	/** Reset owned by this Pawn, independent of the lifetime of its Input Setup. */
	FTimerHandle InputJumpTimer;

	FGASPActionHandle InteractionActionHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFoundryInteractionTargetComponent> InteractionTargetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> InteractionScan;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> InteractionTargetCollision;

	/**
	 * Accepts only known action names; clients never supply a target or pose-search result.
	 */
	UFUNCTION(Server, Reliable)
	void Server_RequestAction(FName ActionType);

	/**
	 * Releases only the GASP action associated with the execution that just ended.
	 */
	UFUNCTION()
	void HandleInteractionFinished(UFoundryInteractionComponent* Component, FInteractionDescriptor Interaction, FInteractionResult Result);

};
