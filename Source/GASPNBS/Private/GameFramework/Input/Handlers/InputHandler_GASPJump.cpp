// Ninja Bear Studio Inc., all rights reserved.
#include "GameFramework/Input/Handlers/InputHandler_GASPJump.h"

#include "GameFramework/GASPFoundryPawn.h"

UInputHandler_GASPJump::UInputHandler_GASPJump()
{
	// Pressed and Released both emit Triggered; subscribing to Started as well would jump twice.
	TriggerEvents.Reset();
	TriggerEvents.Add(ETriggerEvent::Triggered);
	TriggerEvents.Add(ETriggerEvent::Canceled);
}

void UInputHandler_GASPJump::OnInputSetupRemoved_Implementation(const FFoundryInputHandlerContext& Context) const
{
	Super::OnInputSetupRemoved_Implementation(Context);
	StopJump(Context);
}

bool UInputHandler_GASPJump::CanJump_Implementation(const FFoundryInputHandlerContext& Context) const
{
	const AGASPFoundryPawn* Pawn = Cast<AGASPFoundryPawn>(Context.Pawn);
	return IsValid(Pawn) && Pawn->CanStartInputJump() && Super::CanJump_Implementation(Context);
}

EFoundryInputHandlerResult UInputHandler_GASPJump::HandleTriggeredEvent_Implementation(const FFoundryInputHandlerContext& Context) const
{
	if (Context.Value.Get<bool>())
	{
		return HandleStartedEvent(Context);
	}
	
	// A quick release must not erase the pulse before Mover has produced its next input command.
	return EFoundryInputHandlerResult::Continue;
}

void UInputHandler_GASPJump::StartJump_Implementation(const FFoundryInputHandlerContext& Context) const
{
	AGASPFoundryPawn* Pawn = Cast<AGASPFoundryPawn>(Context.Pawn);
	if (IsValid(Pawn))
	{
		Pawn->StartInputJump();
	}
}

void UInputHandler_GASPJump::StopJump_Implementation(const FFoundryInputHandlerContext& Context) const
{
	AGASPFoundryPawn* Pawn = Cast<AGASPFoundryPawn>(Context.Pawn);
	if (IsValid(Pawn))
	{
		Pawn->StopInputJump();
	}
}
