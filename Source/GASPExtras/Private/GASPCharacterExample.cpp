// Ninja Bear Studio Inc., all rights reserved.
#include "GASPCharacterExample.h"

#include "Components/GASPActionComponent.h"
#include "Components/GASPTraversalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/GameplayCameraComponent.h"

const FName AGASPCharacterExample::ActionComponentName = TEXT("CharacterInteractionComponent");

AGASPCharacterExample::AGASPCharacterExample(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameplayCamera = CreateDefaultSubobject<UGameplayCameraComponent>(TEXT("GameplayCamera"));

	ActionComponent = CreateDefaultSubobject<UGASPActionComponent>(ActionComponentName);
	
	if (GetMesh())
	{
		GameplayCamera->SetupAttachment(GetMesh());
	}
}

void AGASPCharacterExample::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (IsValid(GameplayCamera) && IsValid(GetMesh()) && IsValid(GetMesh()->GetSkinnedAsset()))
	{
		// Blueprint mesh defaults are available now; raw native Pawns may still have no skeleton.
		GameplayCamera->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, TEXT("root"));
	}
}

void AGASPCharacterExample::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	APlayerController* PlayerController = Cast<APlayerController>(NewController);
	if (IsValid(PlayerController) && PlayerController->IsLocalController())
	{
		GameplayCamera->ActivateCameraForPlayerController(PlayerController);
	}
}

void AGASPCharacterExample::OnRep_Controller()
{
	Super::OnRep_Controller();
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (IsValid(PlayerController) && PlayerController->IsLocalController())
	{
		GameplayCamera->ActivateCameraForPlayerController(PlayerController);
	}
}

void AGASPCharacterExample::SprintAction(bool bPressed)
{
	auto* InputState{PlayerInputState.GetMutablePtr<FGASPInputState>()};
	if (bPressed)
	{
		InputState->DesiredGait = GaitTags::Sprint;
		if (GetStanceMode() == StanceTags::Standing)
		{
			InputState->DesiredStance = StanceTags::Standing;
		}
		return;
	}
	InputState->DesiredGait = GaitTags::Run;
}

void AGASPCharacterExample::WalkAction(bool bPressed)
{
	auto* InputState{PlayerInputState.GetMutablePtr<FGASPInputState>()};
	if (InputState)
	{
		if (InputState->DesiredGait != GaitTags::Sprint)
		{
			InputState->DesiredGait = InputState->DesiredGait != GaitTags::Walk ? GaitTags::Walk : GaitTags::Run;
		}
	}
}

void AGASPCharacterExample::CrouchAction(bool bPressed)
{
	if (GetMovementMode() == MovementModeTags::Grounded || GetMovementMode() == MovementModeTags::Slide)
	{
		auto* InputState{PlayerInputState.GetMutablePtr<FGASPInputState>()};
		if (InputState)
		{
			InputState->DesiredStance = GetStanceMode() == StanceTags::Crouching
				                            ? StanceTags::Standing
				                            : StanceTags::Crouching;
		}
	}
}

void AGASPCharacterExample::JumpAction(bool bPressed)
{
	if (GetMovementMode() == MovementModeTags::Ragdoll)
	{
		StopRagdolling();
		return;
	}

	auto* InputState{PlayerInputState.GetMutablePtr<FGASPInputState>()};
	if (bPressed && !IsDoingTraversal() && !GetMesh()->IsPlayingRootMotion())
	{
		if (const auto [bTraversalCheckFailed, bMontageSelectionFailed] = TryTraversalAction(); bTraversalCheckFailed ||
			bMontageSelectionFailed)
		{
			if (GetStanceMode() != StanceTags::Standing)
			{
				InputState->DesiredStance = StanceTags::Standing;
			}
			else
			{
				Jump();
				FTimerHandle TimerHandle;
				GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					StopJumping();
				}), .1f, false);
			}
		}
	}
}

void AGASPCharacterExample::AimAction(bool bPressed)
{
	auto* InputState{PlayerInputState.GetMutablePtr<FGASPInputState>()};
	if (InputState)
	{
		InputState->DesiredRotationMode = bPressed ? RotationTags::Aim : RotationTags::OrientToMovement;
	}
}

void AGASPCharacterExample::RagdollAction(bool bPressed)
{
	if (!IsRagdolling())
	{
		FMontageBlendSettings BlendSettings;
		BlendSettings.BlendMode = EMontageBlendMode::Inertialization;
		BlendSettings.Blend.BlendTime = .3f;

		StartRagdolling(true, BlendSettings, FGameplayTag::EmptyTag);
	}
	else
	{
		StopRagdolling();
	}
}

void AGASPCharacterExample::StrafeAction(bool bPressed)
{
	auto InputState{PlayerInputState.GetMutablePtr<FGASPInputState>()};
	if (MoverInputs_PostSim.RotationMode != RotationTags::Strafe)
	{
		InputState->DesiredRotationMode = RotationTags::Strafe;
	}
	else
	{
		InputState->DesiredRotationMode = RotationTags::OrientToMovement;
	}
}

void AGASPCharacterExample::MoveAction(const FVector2D& Value)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	AddMovementInput({Value.X, Value.Y, 0.f});
}

void AGASPCharacterExample::LookAction(const FVector2D& Value)
{
	if (!TwinStickMode)
	{
		AddControllerYawInput(Value.X);
		AddControllerPitchInput(-1 * Value.Y);
	}
}
