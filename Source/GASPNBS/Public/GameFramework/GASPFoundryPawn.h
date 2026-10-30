// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "GASPCharacterExample.h"
#include "Interaction/GASPActionTypes.h"
#include "Interfaces/GASPInteractionInterface.h"
#include "Types/FInteractionCoreTypes.h"
#include "GASPFoundryPawn.generated.h"

class UFoundryInteractionComponent;
class UFoundryInteractionTargetComponent;
class USphereComponent;

/**
 * Keeps the sample camera and input behavior while Foundry owns interaction selection and execution.
 *
 * The performer component comes from Crucible. This Pawn adds authored target definitions and uses
 * the sample's named GASP Action Component for synchronized playback. It also owns cleanup when a
 * Foundry execution ends, including prepared actions that never reach Commit.
 */
UCLASS(Blueprintable)
class GASPNBS_API AGASPFoundryPawn : public AGASPCharacterExample, public IGASPInteractionInterface
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

	FGASPActionHandle InteractionActionHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFoundryInteractionTargetComponent> InteractionTargetComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> InteractionScan;

	UPROPERTY(VisibleAnywhere, Category = "Components")
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
