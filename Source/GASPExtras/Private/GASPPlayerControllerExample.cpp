// Ninja Bear Studio Inc., all rights reserved.
#include "GASPPlayerControllerExample.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GASPCharacterExample.h"

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

void AGASPPlayerControllerExample::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	PossessedPlayer = Cast<AGASPCharacterExample>(GetPawn());

	SetupInput();
}

void AGASPPlayerControllerExample::OnUnPossess()
{
	Super::OnUnPossess();
	PossessedPlayer = nullptr;
	SetupInput();
}

void AGASPPlayerControllerExample::OnRep_Owner()
{
	Super::OnRep_Owner();

	PossessedPlayer = Cast<AGASPCharacterExample>(GetPawn());

	SetupInput();
}

void AGASPPlayerControllerExample::SetupInput()
{
	UEnhancedInputLocalPlayerSubsystem* InputSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!IsValid(InputSystem))
	{
		return;
	}
	UInputMappingContext* Mapping = IsValid(PossessedPlayer) ? DefaultInputMapping.LoadSynchronous() : nullptr;
	if (IsValid(Mapping) && AppliedDefaultInputMapping == Mapping && InputSystem->HasMappingContext(Mapping))
	{
		return;
	}
	if (IsValid(AppliedDefaultInputMapping))
	{
		InputSystem->RemoveMappingContext(AppliedDefaultInputMapping);
		AppliedDefaultInputMapping = nullptr;
	}
	if (IsValid(Mapping) && !InputSystem->HasMappingContext(Mapping))
	{
		FModifyContextOptions Options;
		Options.bForceImmediately = true;
		InputSystem->AddMappingContext(Mapping, 1, Options);
		AppliedDefaultInputMapping = Mapping;
	}
}
