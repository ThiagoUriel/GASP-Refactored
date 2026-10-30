// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "Input/Handlers/Character/InputHandler_Jump.h"
#include "InputHandler_GASPJump.generated.h"

/**
 * Gives the GASP Pawn's traversal interface first refusal, then requests its Mover jump behavior.
 * 
 * Defaults to Triggered input for GASP's Pressed + Released action. Normal release preserves the
 * Pawn's short jump pulse; cancellation and setup removal clear it. Conventional actions may
 * instead select Started/Completed for immediate release through the inherited completion hook.
 */
UCLASS(meta = (DisplayName = "GASP: Jump"))
class GASPNBS_API UInputHandler_GASPJump : public UInputHandler_Jump
{

	GENERATED_BODY()

public:

	UInputHandler_GASPJump();

	// -- Begin FoundryInputHandler implementation
	virtual void OnInputSetupRemoved_Implementation(const FFoundryInputHandlerContext& Context) const override;
	// -- End FoundryInputHandler implementation

	// -- Begin InputHandler_Jump implementation
	virtual bool CanJump_Implementation(const FFoundryInputHandlerContext& Context) const override;
	// -- End InputHandler_Jump implementation

protected:

	// -- Begin FoundryInputHandler implementation
	virtual EFoundryInputHandlerResult HandleTriggeredEvent_Implementation(const FFoundryInputHandlerContext& Context) const override;
	// -- End FoundryInputHandler implementation

	// -- Begin InputHandler_Jump implementation
	virtual void StartJump_Implementation(const FFoundryInputHandlerContext& Context) const override;
	virtual void StopJump_Implementation(const FFoundryInputHandlerContext& Context) const override;
	// -- End InputHandler_Jump implementation

};
