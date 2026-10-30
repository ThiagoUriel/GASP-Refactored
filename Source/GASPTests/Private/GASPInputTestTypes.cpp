// Ninja Bear Studio Inc., all rights reserved.
#include "GASPInputTestTypes.h"

AGASPInputTestPawn::AGASPInputTestPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bScanWithoutPlayer = false;
}

void AGASPInputTestPawn::AddControllerYawInput(const float Value)
{
	ReceivedYaw += Value;
}

void AGASPInputTestPawn::AddControllerPitchInput(const float Value)
{
	ReceivedPitch += Value;
}

bool AGASPInputTestPawn::TryTraversalJumpAction_Implementation(const FFoundryInputHandlerContext& Context)
{
	++TraversalAttempts;
	return bHandleTraversal || Super::TryTraversalJumpAction_Implementation(Context);
}

void AGASPInputTestPawn::SetTwinStickInputEnabled(const bool bEnabled)
{
	TwinStickMode = bEnabled;
}
