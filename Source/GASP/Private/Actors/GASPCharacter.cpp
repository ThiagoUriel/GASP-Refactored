#include "Actors/GASPCharacter.h"
#include "ChooserFunctionLibrary.h"
#include "MotionWarpingComponent.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "Components/GASPCharacterMovementComponent.h"
#include "Components/GASPTraversalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/NetConnection.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Utils/GASPLinkedAnimInstanceSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPCharacter)

FName AGASPCharacter::MotionWarpingComponentName = TEXT("MotionWarping");

// Sets default values
AGASPCharacter::AGASPCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGASPCharacterMovementComponent>(CharacterMovementComponentName))
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationRoll = bUseControllerRotationPitch = bUseControllerRotationYaw = false;

	SetReplicates(true);
	SetReplicatingMovement(true);

	if (GetMesh())
	{
		GetMesh()->SetRelativeRotation_Direct({0.f, -90.f, 0.f});
		GetMesh()->SetRelativeLocation_Direct({0.f, 0.f, -90.f});
	}
	
	MovementComponent = Cast<UGASPCharacterMovementComponent>(GetCharacterMovement());
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(MotionWarpingComponentName);
	TraversalComponent = CreateDefaultSubobject<UGASPTraversalComponent>(TEXT("TraversalComponent"));
}

// Called when the game starts or when spawned
void AGASPCharacter::BeginPlay()
{
	Super::BeginPlay();

	check(MovementComponent)

	OverlayModeChanged.AddDynamic(this, &ThisClass::OnOverlayModeChanged);
	PoseModeChanged.AddDynamic(this, &ThisClass::OnPoseModeChanged);

	SetGait(DesiredGait, true);
	SetRotationMode(RotationMode, true);
	SetOverlayMode(OverlayMode, true);
	SetPoseMode(PoseMode, true);
	SetLocomotionAction(FGameplayTag::EmptyTag, true);
	SetMovementMode(MovementMode, true);
	SetStanceMode(StanceMode, true);
}

void AGASPCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

// Called every frame
void AGASPCharacter::Tick(float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AGASPCharacter::Tick"),
	                            STAT_AGASPCharacter_Tick, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	Super::Tick(DeltaTime);

	if (GetLocalRole() >= ROLE_AutonomousProxy)
	{
		SetReplicatedAcceleration(MovementComponent->GetCurrentAcceleration());
	}

	const bool IsMoving = !MovementComponent->Velocity.IsNearlyZero(.1f) && !ReplicatedAcceleration.IsNearlyZero(10.f);
	SetMovementState(IsMoving ? MovementStateTags::Moving : MovementStateTags::Idle);

	if (LocomotionAction == LocomotionActionTags::Ragdoll)
	{
		RefreshRagdolling(DeltaTime);
	}

	if (MovementMode == MovementModeTags::Grounded)
	{
		if (StanceMode != StanceTags::Crouching)
		{
			RefreshGait();
		}
	}
}

void AGASPCharacter::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	MovementComponent = Cast<UGASPCharacterMovementComponent>(GetCharacterMovement());
}

void AGASPCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	SetStanceMode(StanceTags::Crouching);
}

void AGASPCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	SetStanceMode(StanceTags::Standing);
}

void AGASPCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	GetMesh()->AddTickPrerequisiteActor(this);

	MovementComponent = Cast<UGASPCharacterMovementComponent>(GetCharacterMovement());
}

void AGASPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	// Replicate to everyone except owner
	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredGait, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RotationMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MovementMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MovementState, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, StanceMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, LocomotionAction, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedAcceleration, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, TraversalComponent, Parameters);

	// Replicate to everyone
	Parameters.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, OverlayMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PoseMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RagdollTargetLocation, Parameters);
}

void AGASPCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode)
{
	if (MovementComponent)
	{
		// what to do with traversal?
		switch (MovementComponent->MovementMode)
		{
		case MOVE_Walking:
		case MOVE_NavWalking:
		// traversal modes and root motion	
		case MOVE_Flying:
		case MOVE_None:
			SetMovementMode(MovementModeTags::Grounded);
			break;
		case MOVE_Falling:
			SetMovementMode(MovementModeTags::InAir);
			break;
		default:
			SetMovementMode(FGameplayTag::EmptyTag);
		}
	}

	Super::OnMovementModeChanged(PrevMovementMode, PrevCustomMode);
}

void AGASPCharacter::SetGait(const FGameplayTag NewGait, const bool bForce)
{
	if (!ensure(MovementComponent))
	{
		return;
	}

	if (NewGait != Gait || bForce)
	{
		const auto OldGait{Gait};
		Gait = NewGait;
		MovementComponent->SetGait(NewGait);

		GaitChanged.Broadcast(OldGait);
	}
}

void AGASPCharacter::SetDesiredGait(FGameplayTag NewGait, bool bForce)
{
	if (NewGait != Gait || bForce)
	{
		DesiredGait = NewGait;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredGait, this);

		if (GetLocalRole() == ROLE_AutonomousProxy && IsValid(GetNetConnection()))
		{
			Server_SetDesiredGait(NewGait);
		}
	}
}

void AGASPCharacter::SetRotationMode(const FGameplayTag NewRotationMode, const bool bForce)
{
	if (NewRotationMode != RotationMode || bForce)
	{
		const auto OldRotationMode{RotationMode};
		RotationMode = NewRotationMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RotationMode, this);

		MovementComponent->SetRotationMode(NewRotationMode);

		if (GetLocalRole() == ROLE_AutonomousProxy && IsValid(GetNetConnection()))
		{
			Server_SetRotationMode(NewRotationMode);
		}

		RotationModeChanged.Broadcast(OldRotationMode);
	}
}

void AGASPCharacter::SetMovementMode(const FGameplayTag NewMovementMode, const bool bForce)
{
	if (NewMovementMode != MovementMode || bForce)
	{
		auto OldMovementMode{MovementMode};
		MovementMode = NewMovementMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MovementMode, this);
		if (GetLocalRole() == ROLE_AutonomousProxy && IsValid(GetNetConnection()))
		{
			Server_SetMovementMode(NewMovementMode);
		}

		MovementModeChanged.Broadcast(OldMovementMode);
	}
}

void AGASPCharacter::Server_SetMovementMode_Implementation(const FGameplayTag NewMovementMode)
{
	SetMovementMode(NewMovementMode);
}

void AGASPCharacter::Server_SetRotationMode_Implementation(const FGameplayTag NewRotationMode)
{
	SetRotationMode(NewRotationMode);
}

void AGASPCharacter::Server_SetDesiredGait_Implementation(const FGameplayTag NewGait)
{
	SetDesiredGait(NewGait);
}

void AGASPCharacter::SetMovementState(const FGameplayTag NewMovementState, const bool bForce)
{
	if (!ensure(MovementComponent))
	{
		return;
	}

	if (NewMovementState != MovementState || bForce)
	{
		const auto OldMovementState{MovementState};
		MovementState = NewMovementState;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MovementState, this);

		if (GetLocalRole() == ROLE_AutonomousProxy && IsValid(GetNetConnection()))
		{
			Server_SetMovementState(NewMovementState);
		}
		MovementStateChanged.Broadcast(OldMovementState);
	}
}

void AGASPCharacter::SetStanceMode(const FGameplayTag NewStanceMode, const bool bForce)
{
	if (StanceMode != NewStanceMode || bForce)
	{
		const auto OldStanceMode{StanceMode};
		StanceMode = NewStanceMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StanceMode, this);

		MovementComponent->SetStance(NewStanceMode);

		if (GetLocalRole() == ROLE_AutonomousProxy && IsValid(GetNetConnection()))
		{
			Server_SetStanceMode(NewStanceMode);
		}
		StanceModeChanged.Broadcast(OldStanceMode);
	}
}

void AGASPCharacter::Server_SetStanceMode_Implementation(const FGameplayTag NewStanceMode)
{
	SetStanceMode(NewStanceMode);
}

void AGASPCharacter::Server_SetMovementState_Implementation(const FGameplayTag NewMovementState)
{
	SetMovementState(NewMovementState);
}

bool AGASPCharacter::CanSprint()
{
	if (RotationMode == RotationTags::OrientToMovement)
	{
		return true;
	}
	if (RotationMode == RotationTags::Aim)
	{
		return false;
	}

	const FVector ViewDirection{GetActorForwardVector()};

	const float Dot = FVector::DotProduct(ReplicatedAcceleration.GetSafeNormal2D(), ViewDirection.GetSafeNormal2D());

	return Dot > FMath::Cos(FMath::DegreesToRadians(50.f));
}

void AGASPCharacter::SetOverlayMode(const FGameplayTag NewOverlayMode, const bool bForce)
{
	if (NewOverlayMode != OverlayMode || bForce)
	{
		const auto OldOverlayMode{OverlayMode};
		OverlayMode = NewOverlayMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, OverlayMode, this);
		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetOverlayMode(NewOverlayMode);
		}
		OverlayModeChanged.Broadcast(OldOverlayMode);
	}
}

void AGASPCharacter::SetPoseMode(const FGameplayTag NewPoseMode, const bool bForce)
{
	if (NewPoseMode != PoseMode || bForce)
	{
		const auto OldOverlayMode{PoseMode};
		PoseMode = NewPoseMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, OverlayMode, this);
		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetOverlayMode(NewPoseMode);
		}
		PoseModeChanged.Broadcast(OldOverlayMode);
	}
}

void AGASPCharacter::Server_SetPoseMode_Implementation(const FGameplayTag NewPoseMode)
{
	SetPoseMode(NewPoseMode);
}

void AGASPCharacter::SetLocomotionAction(const FGameplayTag NewLocomotionAction, const bool bForce)
{
	if (NewLocomotionAction != LocomotionAction || bForce)
	{
		const auto OldLocomotionAction{LocomotionAction};
		LocomotionAction = NewLocomotionAction;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LocomotionAction, this);

		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetLocomotionAction(NewLocomotionAction);
		}

		LocomotionActionChanged.Broadcast(OldLocomotionAction);
	}
}

void AGASPCharacter::Server_SetLocomotionAction_Implementation(const FGameplayTag NewLocomotionAction)
{
	SetLocomotionAction(NewLocomotionAction);
}

void AGASPCharacter::Server_SetOverlayMode_Implementation(const FGameplayTag NewOverlayMode)
{
	SetOverlayMode(NewOverlayMode);
}

void AGASPCharacter::SetReplicatedAcceleration(const FVector& NewAcceleration)
{
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, ReplicatedAcceleration, NewAcceleration, this);
}

void AGASPCharacter::OnWalkingOffLedge_Implementation(const FVector& PreviousFloorImpactNormal,
                                                      const FVector& PreviousFloorContactNormal,
                                                      const FVector& PreviousLocation, float TimeDelta)
{
	Super::OnWalkingOffLedge_Implementation(PreviousFloorImpactNormal, PreviousFloorContactNormal, PreviousLocation,
	                                        TimeDelta);
	UnCrouch();
}

bool AGASPCharacter::HasFullMovementInput() const
{
	if (MovementStickMode == EAnalogStickBehaviorMode::FixedWalkRun || MovementStickMode ==
		EAnalogStickBehaviorMode::VariableWalkRun)
	{
		return GetPendingMovementInputVector().Size2D() >= AnalogMovementThreshold;
	}

	return true;
}

FVector2D AGASPCharacter::GetMovementInputScaleValue(const FVector2D InVector) const
{
	return MovementStickMode > EAnalogStickBehaviorMode::FixedWalkRun ? InVector : InVector.GetSafeNormal();
}

void AGASPCharacter::RefreshGait()
{
	FGameplayTag NewGait{DesiredGait};

	if (DesiredGait == GaitTags::Sprint && CanSprint())
	{
		NewGait = HasFullMovementInput() ? GaitTags::Sprint : GaitTags::Run;
	}
	else if (DesiredGait == GaitTags::Walk)
	{
		NewGait = GaitTags::Walk;
	}
	else if (DesiredGait == GaitTags::Sprint || DesiredGait == GaitTags::Run)
	{
		NewGait = HasFullMovementInput() ? GaitTags::Run : GaitTags::Walk;
	}

	SetGait(NewGait);
}

FTraversalResult AGASPCharacter::TryTraversalAction() const
{
	if (IsValid(TraversalComponent))
	{
		return TraversalComponent->TryTraversalAction(GetTraversalCheckInputs());
	}

	return {true, false};
}

bool AGASPCharacter::IsDoingTraversal() const
{
	return IsValid(TraversalComponent) && TraversalComponent->IsDoingTraversal();
}

FTraversalCheckInputs AGASPCharacter::GetTraversalCheckInputs() const
{
	const FVector ForwardVector{GetActorForwardVector()};
	if (MovementMode == MovementModeTags::InAir)
	{
		return {
			ForwardVector, 75.f, FVector::ZeroVector,
			{0.f, 0.f, 50.f}, 30.f, 86.f
		};
	}

	const FVector RotationVector = GetActorRotation().UnrotateVector(MovementComponent->Velocity);
	const float ClampedDistance = FMath::GetMappedRangeValueClamped<float, float>(
		{0.f, 500.f}, {75.f, 350.f}, RotationVector.X);

	return {
		ForwardVector, ClampedDistance, FVector::ZeroVector,
		FVector::ZeroVector, 30.f, 60.f
	};
}

void AGASPCharacter::LinkAnimInstance(const UChooserTable* DataTable, const FGameplayTag OldState,
                                      const FGameplayTag State)
{
	if (!DataTable)
	{
		return;
	}

	StateContainer.RemoveTag(OldState);
	StateContainer.AddLeafTag(State);

	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!IsValid(MeshComponent))
	{
		return;
	}
	const UGASPLinkedAnimInstanceSet* DataAsset{
		static_cast<UGASPLinkedAnimInstanceSet*>(
			UChooserFunctionLibrary::EvaluateChooser(this, DataTable, UGASPLinkedAnimInstanceSet::StaticClass()))
	};

	if (IsValid(DataAsset))
	{
		MeshComponent->LinkAnimClassLayers(DataAsset->GetAnimInstance());
	}
}

void AGASPCharacter::OnPoseModeChanged_Implementation(const FGameplayTag OldPoseMode)
{
	LinkAnimInstance(PosesTable, OldPoseMode, PoseMode);
}

void AGASPCharacter::OnOverlayModeChanged_Implementation(const FGameplayTag OldOverlayMode)
{
	LinkAnimInstance(OverlayTable, OldOverlayMode, OverlayMode);
}

void AGASPCharacter::OnRep_OverlayMode(const FGameplayTag& OldOverlayMode)
{
	OverlayModeChanged.Broadcast(OldOverlayMode);
}

void AGASPCharacter::OnRep_PoseMode(const FGameplayTag& OldPoseMode)
{
	PoseModeChanged.Broadcast(OldPoseMode);
}

void AGASPCharacter::OnRep_Gait(const FGameplayTag& OldGait)
{
	GaitChanged.Broadcast(OldGait);
}

void AGASPCharacter::OnRep_StanceMode(const FGameplayTag& OldStanceMode)
{
	StanceModeChanged.Broadcast(OldStanceMode);
}

void AGASPCharacter::OnRep_MovementMode(const FGameplayTag& OldMovementMode)
{
	MovementModeChanged.Broadcast(OldMovementMode);
}

void AGASPCharacter::OnRep_RotationMode(const FGameplayTag& OldRotationMode)
{
	RotationModeChanged.Broadcast(OldRotationMode);
}

void AGASPCharacter::OnRep_MovementState(const FGameplayTag
	& OldMovementState)
{
	MovementStateChanged.Broadcast(OldMovementState);
}

void AGASPCharacter::OnRep_LocomotionAction(const FGameplayTag& OldLocomotionAction)
{
	LocomotionActionChanged.Broadcast(OldLocomotionAction);
}
