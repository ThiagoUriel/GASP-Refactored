// Ninja Bear Studio Inc., all rights reserved.
#include "GameFramework/Input/Handlers/InputHandler_GASPLook.h"

#include "GameFramework/GASPFoundryPawn.h"

UInputHandler_GASPLook::UInputHandler_GASPLook()
{
	MinimumMagnitudeToLook = 0.f;
}

bool UInputHandler_GASPLook::CanLook_Implementation(const FFoundryInputHandlerContext& Context) const
{
	const AGASPFoundryPawn* Pawn = Cast<AGASPFoundryPawn>(Context.Pawn);
	return IsValid(Pawn) 
		&& Pawn->IsLocallyControlled() 
		&& !Pawn->IsTwinStickInputEnabled()
		&& Super::CanLook_Implementation(Context);
}

EFoundryInputHandlerResult UInputHandler_GASPLook::Look_Implementation(const FFoundryInputHandlerContext& Context) const
{
	const FVector2D Value = Context.Value.Get<FVector2D>();
	FFoundryInputHandlerContext LookContext = Context;
	LookContext.Value = FInputActionValue(FVector2D(Value.X, -Value.Y));
	return Super::Look_Implementation(LookContext);
}
