#include "GASPPlayerControllerExample.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GASPCharacterExample.h"
#include "Engine/LocalPlayer.h"

TSoftObjectPtr<UInputMappingContext> AGASPPlayerControllerExample::GetDefaultInputMapping()
{
	return DefaultInputMapping;
}

AGASPCharacterExample* AGASPPlayerControllerExample::GetPossessedPlayer()
{
	return PossessedPlayer;
}

void AGASPPlayerControllerExample::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	PossessedPlayer = Cast<AGASPCharacterExample>(InPawn);

	SetupInput();
}

void AGASPPlayerControllerExample::OnUnPossess()
{
	PossessedPlayer = nullptr;
	Super::OnUnPossess();
}

void AGASPPlayerControllerExample::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	PossessedPlayer = Cast<AGASPCharacterExample>(GetPawn());

	SetupInput();
}

void AGASPPlayerControllerExample::OnRep_Owner()
{
	Super::OnRep_Owner();

	PossessedPlayer = Cast<AGASPCharacterExample>(GetOwner());

	SetupInput();
}

void AGASPPlayerControllerExample::SetupInput() const
{
	if (!IsValid(PossessedPlayer))
	{
		return;
	}
	// Get the Enhanced Input Local Player Subsystem from the Local Player related to our Player Controller.
	if (UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// PawnClientRestart can run more than once in an Actor's lifetime, so start by clearing out any leftover
		// mappings.
		InputSystem->ClearAllMappings();

		FModifyContextOptions Options;
		Options.bForceImmediately = true;

		// Add each mapping context, along with their priority values. Higher values outprioritize lower values.
		InputSystem->AddMappingContext(DefaultInputMapping.LoadSynchronous(), 1, Options);
	}
}
