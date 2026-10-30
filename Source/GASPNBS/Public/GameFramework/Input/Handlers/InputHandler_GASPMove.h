// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "Input/Handlers/Character/InputHandler_Move.h"
#include "InputHandler_GASPMove.generated.h"

/**
 * Applies GASP's X-forward/Y-right input before Mover consumes and rotates it by controller yaw.
 * 
 * Uses Foundry's event dispatch and blocking checks. Mapping modifiers own the input dead zone;
 * no forward-reference rotation or gait scaling is applied by this adapter.
 */
UCLASS(meta = (DisplayName = "GASP: Move"))
class GASPNBS_API UInputHandler_GASPMove : public UInputHandler_Move
{

	GENERATED_BODY()

public:

	UInputHandler_GASPMove();

	// -- Begin InputHandler_Move implementation
	virtual bool CanMove_Implementation(const FFoundryInputHandlerContext& Context) const override;
	// -- End InputHandler_Move implementation

protected:

	// -- Begin FoundryInputHandler implementation
	virtual bool CanHandle_Implementation(const FFoundryInputHandlerContext& Context) const override;
	// -- End FoundryInputHandler implementation

	// -- Begin InputHandler_Move implementation
	virtual EFoundryInputHandlerResult Move_Implementation(const FFoundryInputHandlerContext& Context) const override;
	// -- End InputHandler_Move implementation

};
