// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "GASPActionTestTypes.h"
#include "GASPInputTestTypes.generated.h"

/**
 * Observes camera requests and an optional custom traversal result while retaining real GASP
 * movement input, jump state, timer ownership and possession cleanup.
 */
UCLASS(Hidden, NotPlaceable, NotBlueprintable, NotBlueprintType)
class AGASPInputTestPawn : public AGASPActionTestPawn
{

	GENERATED_BODY()

public:

	float ReceivedYaw = 0.f;
	float ReceivedPitch = 0.f;
	int32 TraversalAttempts = 0;
	bool bHandleTraversal = false;

	AGASPInputTestPawn(const FObjectInitializer& ObjectInitializer);

	// -- Begin Pawn implementation
	virtual void AddControllerYawInput(float Value) override;
	virtual void AddControllerPitchInput(float Value) override;
	// -- End Pawn implementation

	// -- Begin TraversalMovementInputInterface implementation
	virtual bool TryTraversalJumpAction_Implementation(const FFoundryInputHandlerContext& Context) override;
	// -- End TraversalMovementInputInterface implementation

	/** Selects the runtime control scheme without depending on process-wide console settings. */
	void SetTwinStickInputEnabled(bool bEnabled);

};
