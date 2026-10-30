// Ninja Bear Studio Inc., all rights reserved.
#include "GameFramework/Input/Handlers/InputHandler_GASPMove.h"

#include "GameFramework/GASPFoundryPawn.h"

UInputHandler_GASPMove::UInputHandler_GASPMove()
{
	MinimumMagnitudeToMove = 0.f;
}

bool UInputHandler_GASPMove::CanMove_Implementation(const FFoundryInputHandlerContext& Context) const
{
	const AGASPFoundryPawn* Pawn = Cast<AGASPFoundryPawn>(Context.Pawn);
	return IsValid(Pawn) 
		&& Pawn->IsLocallyControlled() 
		&& Super::CanMove_Implementation(Context);
}

bool UInputHandler_GASPMove::CanHandle_Implementation(const FFoundryInputHandlerContext& Context) const
{
	// This adapter deliberately bypasses Move's unused forward-reference provider requirement.
	return UFoundryInputHandler::CanHandle_Implementation(Context);
}

EFoundryInputHandlerResult UInputHandler_GASPMove::Move_Implementation(const FFoundryInputHandlerContext& Context) const
{
	if (CanMove(Context))
	{
		// GASP applies controller yaw when consuming this vector; rotating here would apply it twice.
		const FVector2D Value = Context.Value.Get<FVector2D>();
		Context.Pawn->AddMovementInput(FVector(Value.X, Value.Y, 0.f));
	}
	
	return EFoundryInputHandlerResult::Continue;
}
