// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "Input/Handlers/Character/InputHandler_Look.h"
#include "InputHandler_GASPLook.generated.h"

/**
 * Reuses Foundry camera input with GASP's inverted pitch and twin-stick restriction.
 * 
 * Configure frame-time scaling separately for mouse deltas and rate-based gamepad input.
 * Existing GASP mappings need no additional pitch inversion modifier.
 */
UCLASS(meta = (DisplayName = "GASP: Look"))
class GASPNBS_API UInputHandler_GASPLook : public UInputHandler_Look
{

	GENERATED_BODY()

public:

	UInputHandler_GASPLook();

	// -- Begin InputHandler_Look implementation
	virtual bool CanLook_Implementation(const FFoundryInputHandlerContext& Context) const override;
	// -- End InputHandler_Look implementation

protected:

	// -- Begin InputHandler_Look implementation
	virtual EFoundryInputHandlerResult Look_Implementation(const FFoundryInputHandlerContext& Context) const override;
	// -- End InputHandler_Look implementation

};
