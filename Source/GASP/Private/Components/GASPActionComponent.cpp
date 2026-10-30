// Ninja Bear Studio Inc., all rights reserved.
#include "Components/GASPActionComponent.h"

#include "Chooser.h"
#include "ChooserFunctionLibrary.h"
#include "GASP.h"
#include "Actors/GASPCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/GASPActionPlayback.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "Types/TagTypes.h"
#include "Interfaces/GASPInteractionInterface.h"
#include "MovementSet/GASPMoverComponent.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "PoseSearch/PoseSearchInteractionAsset.h"
#include "PoseSearch/PoseSearchInteractionLibrary.h"
#include "Settings/GASPCharacterSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPActionComponent)

UGASPActionComponent::UGASPActionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UGASPActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelReservedAction();
	FinishAction(false);
	Super::EndPlay(EndPlayReason);
}

void UGASPActionComponent::OnUnregister()
{
	CancelReservedAction();
	FinishAction(false);
	Super::OnUnregister();
}

void UGASPActionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_None;
	Params.RepNotifyCondition = REPNOTIFY_OnChanged;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ActionState, Params);
}

void UGASPActionComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Actor references or animation instances can become available after the replication callback.
	if (ActionState.bPlaying && PresentedHandle != ActionState.Handle)
	{
		if (StartLocalPlayback())
		{
			SetComponentTickEnabled(false);
		}
	}
	else
	{
		SetComponentTickEnabled(false);
	}
}

bool UGASPActionComponent::CanPerformAction_Implementation(const FName ActionType, AActor* Target, const FGASPActionHandle Handle) const
{
	const AGASPCharacter* Source = Cast<AGASPCharacter>(GetOwner());
	const AGASPCharacter* Partner = Cast<AGASPCharacter>(Target);
	if (bFinishing || ActionType.IsNone() || !IsRegistered() || !IsValid(Source) || !IsValid(Partner)
		|| Source == Partner || Source->GetWorld() != Partner->GetWorld()
		|| Source->IsActorBeingDestroyed() || Partner->IsActorBeingDestroyed()
		|| Source->IsRagdolling() || Partner->IsRagdolling() || Source->IsDoingTraversal() || Partner->IsDoingTraversal())
	{
		return false;
	}

	const UGASPActionComponent* TargetAction = Partner->FindComponentByClass<UGASPActionComponent>();
	const bool bOwnsSource = !ReservationOwner.IsValid() || (ReservationOwner == this && ReservationHandle == Handle);
	const bool bOwnsTarget = IsValid(TargetAction) && TargetAction->IsRegistered()
		&& (!TargetAction->ReservationOwner.IsValid() || (TargetAction->ReservationOwner == this && TargetAction->ReservationHandle == Handle));
	const UAnimInstance* SourceAnimation = GetAnimContext(Source);
	const UAnimInstance* TargetAnimation = GetAnimContext(Target);
	return bOwnsSource && bOwnsTarget && IsValid(SourceAnimation) && IsValid(TargetAnimation)
		&& !SourceAnimation->IsSlotActive(FAnimSlotGroup::DefaultSlotName)
		&& !TargetAnimation->IsSlotActive(FAnimSlotGroup::DefaultSlotName)
		&& CanSelectTarget(ActionType, Source, Partner);
}

bool UGASPActionComponent::CanSelectTarget(const FName ActionType, const AGASPCharacter* Source, const AGASPCharacter* Target) const
{
	const FGASPInputState* Input = Source->PlayerInputState.GetPtr<FGASPInputState>();
	if (Source->GetMovementMode() != MovementModeTags::Grounded || Target->GetMovementMode() != MovementModeTags::Grounded
		|| (Source->GetMoverComponent()->GetRotationMode() == RotationTags::Aim
			|| (Input != nullptr && Input->DesiredRotationMode == RotationTags::Aim)))
	{
		return false;
	}

	const FVector Offset = Target->GetActorLocation() - Source->GetActorLocation();
	const float ForwardAlignment = FVector::DotProduct(Source->GetActorForwardVector().GetSafeNormal2D(), Offset.GetSafeNormal2D());
	const float MinimumAlignment = FMath::Cos(FMath::DegreesToRadians(TargetConeHalfAngle));
	if (Offset.SizeSquared() > FMath::Square(MaximumTargetDistance) || FMath::Abs(Offset.Z) > MaximumTargetHeightDifference
		|| ForwardAlignment < MinimumAlignment)
	{
		return false;
	}

	static constexpr float ProbeRadius = 20.f;
	static constexpr float ProbeHalfHeight = 60.f;
	static const FName CollisionProfile = TEXT("Pawn");
	FCollisionQueryParams Query(SCENE_QUERY_STAT(GASPActionReach), false, Source);
	Query.AddIgnoredActor(Target);
	const FCollisionShape Probe = FCollisionShape::MakeCapsule(ProbeRadius, ProbeHalfHeight);
	return !GetWorld()->SweepTestByProfile(Source->GetActorLocation(), Target->GetActorLocation(), FQuat::Identity, CollisionProfile, Probe, Query);
}

bool UGASPActionComponent::PrepareAction(const FGASPActionHandle Handle, const FName ActionType, AActor* Target)
{
	if (bPreparing || bCommitting || !Handle.IsValid() || !GetOwner()->HasAuthority() || PreparedHandle.IsValid())
	{
		return false;
	}

	TGuardValue<bool> PreparingGuard(bPreparing, true);
	if (!CanPerformAction(ActionType, Target, Handle))
	{
		return false;
	}

	TArray<FGASPInteractionRep> Roles;
	const bool bSelected = SelectAction(ActionType, Target, Roles);
	if (!bSelected || !AreRolesReady(Roles, Handle))
	{
		UE_LOGF(LogGASP, Verbose, "Could not prepare '%ls' from '%ls' to '%ls': selection returned %d role(s), selected: %d.",
			*ActionType.ToString(), *GetNameSafe(GetOwner()), *GetNameSafe(Target), Roles.Num(), bSelected);
		return false;
	}
	TSet<AActor*> Actors;
	for (const FGASPInteractionRep& Role : Roles)
	{
		Actors.Add(Role.Actor);
	}
	if (Actors.Num() != 2 || !Actors.Contains(GetOwner()) || !Actors.Contains(Target))
	{
		return false;
	}

	PreparedHandle = Handle;
	PreparedActionType = ActionType;
	PreparedTarget = Target;
	PreparedRoles = MoveTemp(Roles);
	for (const FGASPInteractionRep& Role : PreparedRoles)
	{
		UGASPActionComponent* ParticipantAction = Role.Actor->FindComponentByClass<UGASPActionComponent>();
		ParticipantAction->ReservationOwner = this;
		ParticipantAction->ReservationHandle = Handle;
		Role.Actor->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleParticipantEndPlay);
	}
	return true;
}

bool UGASPActionComponent::IsPreparedFor(const FGASPActionHandle Handle) const
{
	return Handle.IsValid() && PreparedHandle == Handle && !LocalPlaybackHandle.IsValid() && !bFinishing;
}

bool UGASPActionComponent::CommitAction(const FGASPActionHandle Handle, FGASPActionCompletion InCompletionCallback)
{
	if (bCommitting || !GetOwner()->HasAuthority() || !IsPreparedFor(Handle) || !InCompletionCallback.IsBound())
	{
		return false;
	}
	TGuardValue<bool> CommittingGuard(bCommitting, true);
	const bool bCanPerform = CanPerformAction(PreparedActionType, PreparedTarget.Get(), Handle);
	if (!bCanPerform || !IsPreparedFor(Handle) || !AreRolesReady(PreparedRoles, Handle))
	{
		return false;
	}

	CompletionCallback = MoveTemp(InCompletionCallback);
	ActionState.Handle = Handle;
	ActionState.Participants = PreparedRoles;
	ActionState.ServerStartTime = GetServerTime();
	ActionState.bPlaying = true;
	for (FGASPInteractionRep& Role : ActionState.Participants)
	{
		// Animation instances belong to each machine; send Actors and rebuild the contexts locally.
		Role.Result.AnimContexts.Reset();
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ActionState, this);
	GetOwner()->ForceNetUpdate();
	if (!StartLocalPlayback())
	{
		FinishAction(false);
	}
	return true;
}

void UGASPActionComponent::CancelAction(const FGASPActionHandle Handle)
{
	if (Handle.IsValid() && (PreparedHandle == Handle || LocalPlaybackHandle == Handle))
	{
		FinishAction(false);
	}
}

void UGASPActionComponent::CancelReservedAction()
{
	if (UGASPActionComponent* Owner = ReservationOwner.Get())
	{
		Owner->CancelAction(ReservationHandle);
	}
}

bool UGASPActionComponent::RegisterRolePlayback(const FGASPActionHandle Handle, const FPoseSearchBlueprintResult& Result, UGASPActionComponent* ExecutionOwner)
{
	if (!IsValid(ExecutionOwner) || !Handle.IsValid() || (LocalRoleHandle.IsValid() && LocalRoleHandle != Handle))
	{
		return false;
	}
	LocalRoleHandle = Handle;
	RoleExecutionOwner = ExecutionOwner;
	ActionResult = Result;
	return true;
}

void UGASPActionComponent::UnregisterRolePlayback(const FGASPActionHandle Handle)
{
	if (LocalRoleHandle == Handle)
	{
		LocalRoleHandle = FGASPActionHandle();
		RoleExecutionOwner.Reset();
		ActionResult = FPoseSearchBlueprintResult();
	}
}

void UGASPActionComponent::CompleteRoleForHandoff()
{
	if (UGASPActionComponent* Execution = RoleExecutionOwner.Get())
	{
		Execution->SignalRoleHandoff(LocalRoleHandle, GetOwner());
	}
}

void UGASPActionComponent::SignalRoleHandoff(const FGASPActionHandle Handle, AActor* Participant)
{
	if (!Handle.IsValid() || Handle != LocalPlaybackHandle || bFinishing)
	{
		return;
	}
	for (UGASPActionPlayback* Playback : Playbacks)
	{
		if (IsValid(Playback) && Playback->CompleteForHandoff(Participant))
		{
			return;
		}
	}
}

bool UGASPActionComponent::AreRolesReady(const TArray<FGASPInteractionRep>& Roles, const FGASPActionHandle Handle) const
{
	if (Roles.Num() != 2)
	{
		return false;
	}
	TSet<int32> RoleIndices;
	TSet<const AActor*> Participants;
	const UObject* Performance = Roles[0].Result.SelectedAnim;
	for (const FGASPInteractionRep& Role : Roles)
	{
		const AGASPCharacter* Actor = Cast<AGASPCharacter>(Role.Actor);
		const UPoseSearchInteractionAsset* Asset = Cast<UPoseSearchInteractionAsset>(Role.Result.SelectedAnim);
		const UAnimMontage* Montage = IsValid(Asset) ? Cast<UAnimMontage>(Asset->GetAnimationAsset(Role.Result.Role)) : nullptr;
		const UGASPActionComponent* ParticipantAction = IsValid(Actor) ? Actor->FindComponentByClass<UGASPActionComponent>() : nullptr;
		const bool bValidRole = IsValid(Asset) && Asset->GetNumRoles() == Roles.Num()
			&& Asset->Items.IsValidIndex(Role.Result.RoleIndex) && Asset->GetRole(Role.Result.RoleIndex) == Role.Result.Role
			&& !RoleIndices.Contains(Role.Result.RoleIndex) && Role.Result.SelectedAnim == Performance;
		if (!bValidRole || !IsValid(Actor) || Actor->GetWorld() != GetWorld() || Actor->IsActorBeingDestroyed()
			|| Participants.Contains(Actor) || !IsValid(Actor->GetMoverComponent())
			|| !IsValid(GetAnimContext(Actor)) || !IsValid(Montage) || !FMath::IsFinite(Montage->RateScale) || Montage->RateScale <= 0.f
			|| !FMath::IsFinite(Role.Result.SelectedTime) || Role.Result.SelectedTime < 0.f || Role.Result.SelectedTime >= Montage->GetPlayLength()
			|| !IsValid(ParticipantAction) || !ParticipantAction->IsRegistered())
		{
			return false;
		}
		if (ParticipantAction->ReservationOwner.IsValid()
			&& (ParticipantAction->ReservationOwner != this || ParticipantAction->ReservationHandle != Handle))
		{
			return false;
		}
		RoleIndices.Add(Role.Result.RoleIndex);
		Participants.Add(Actor);
	}
	return true;
}

bool UGASPActionComponent::StartLocalPlayback()
{
	const TArray<FGASPInteractionRep> Roles = ActionState.Participants;
	const FGASPActionHandle StartingHandle = ActionState.Handle;
	if (!StartingHandle.IsValid() || !AreRolesReady(Roles, StartingHandle))
	{
		return false;
	}
	float LongestRole = 0.f;
	for (const FGASPInteractionRep& Role : Roles)
	{
		const UGASPActionComponent* ParticipantAction = Role.Actor->FindComponentByClass<UGASPActionComponent>();
		if (ParticipantAction->LocalRoleHandle.IsValid() && ParticipantAction->LocalRoleHandle != StartingHandle)
		{
			return false;
		}
		const UPoseSearchInteractionAsset* Asset = CastChecked<UPoseSearchInteractionAsset>(Role.Result.SelectedAnim);
		const UAnimMontage* Montage = CastChecked<UAnimMontage>(Asset->GetAnimationAsset(Role.Result.Role));
		LongestRole = FMath::Max(LongestRole, (Montage->GetPlayLength() - Role.Result.SelectedTime) / Montage->RateScale);
	}
	const float Elapsed = FMath::Max(0.f, GetServerTime() - ActionState.ServerStartTime);
	if (Elapsed >= LongestRole)
	{
		// A late observer should not restart a performance that has already finished.
		PresentedHandle = ActionState.Handle;
		return true;
	}

	TArray<TObjectPtr<const UObject>> Contexts;
	Contexts.SetNum(Roles.Num());
	for (const FGASPInteractionRep& Role : Roles)
	{
		Contexts[Role.Result.RoleIndex] = GetAnimContext(Role.Actor);
		const UPoseSearchInteractionAsset* Asset = CastChecked<UPoseSearchInteractionAsset>(Role.Result.SelectedAnim);
		const UAnimMontage* Montage = CastChecked<UAnimMontage>(Asset->GetAnimationAsset(Role.Result.Role));
		const float EndTime = (Montage->GetPlayLength() - Role.Result.SelectedTime) / Montage->RateScale;
		if (Elapsed < EndTime)
		{
			PendingParticipants.Add(Role.Actor.Get());
		}
	}
	if (bFinishing || !ActionState.bPlaying || ActionState.Handle != StartingHandle)
	{
		return false;
	}
	LocalPlaybackHandle = StartingHandle;
	for (const FGASPInteractionRep& Role : Roles)
	{
		if (!PendingParticipants.Contains(Role.Actor.Get()))
		{
			continue;
		}
		UGASPActionPlayback* Playback = NewObject<UGASPActionPlayback>(this);
		Playbacks.Add(Playback);
		if (!Playback->Start(this, StartingHandle, Role, Contexts, Elapsed) || LocalPlaybackHandle != StartingHandle)
		{
			StopLocalPlayback();
			return false;
		}
	}
	PresentedHandle = StartingHandle;
	return true;
}

void UGASPActionComponent::ReportPlaybackCompletion(const FGASPActionHandle Handle, AActor* Participant, const bool bSucceeded)
{
	if (Handle != LocalPlaybackHandle || !PendingParticipants.Remove(Participant))
	{
		return;
	}
	const bool bAuthoritativeFailure = !bSucceeded && GetOwner()->HasAuthority();
	if (bAuthoritativeFailure || PendingParticipants.IsEmpty())
	{
		FinishAction(bSucceeded);
	}
}

void UGASPActionComponent::StopLocalPlayback()
{
	// Invalidate callbacks before stopping montages, since stopping can synchronously notify listeners.
	LocalPlaybackHandle = FGASPActionHandle();
	PendingParticipants.Reset();
	for (UGASPActionPlayback* Playback : Playbacks)
	{
		if (IsValid(Playback))
		{
			Playback->Stop();
		}
	}
	Playbacks.Reset();
	SetComponentTickEnabled(false);
}

void UGASPActionComponent::FinishAction(const bool bSucceeded)
{
	if (bFinishing)
	{
		return;
	}
	FGASPActionCompletion Callback = MoveTemp(CompletionCallback);
	CompletionCallback.Unbind();
	{
		TGuardValue<bool> FinishingGuard(bFinishing, true);
		StopLocalPlayback();
		for (const FGASPInteractionRep& Role : PreparedRoles)
		{
			if (!IsValid(Role.Actor))
			{
				continue;
			}
			Role.Actor->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleParticipantEndPlay);
			UGASPActionComponent* ParticipantAction = Role.Actor->FindComponentByClass<UGASPActionComponent>();
			if (IsValid(ParticipantAction) && ParticipantAction->ReservationOwner == this && ParticipantAction->ReservationHandle == PreparedHandle)
			{
				ParticipantAction->ReservationOwner.Reset();
				ParticipantAction->ReservationHandle = FGASPActionHandle();
			}
		}
		PreparedRoles.Reset();
		PreparedHandle = FGASPActionHandle();
		PreparedTarget.Reset();
		PreparedActionType = NAME_None;
		if (IsValid(GetOwner()) && GetOwner()->HasAuthority() && ActionState.bPlaying)
		{
			ActionState.bPlaying = false;
			ActionState.Participants.Reset();
			MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ActionState, this);
			GetOwner()->ForceNetUpdate();
		}
	}
	Callback.ExecuteIfBound(bSucceeded);
}

void UGASPActionComponent::OnRep_ActionState()
{
	if (ActionState.bPlaying && PresentedHandle == ActionState.Handle)
	{
		return;
	}
	StopLocalPlayback();
	if (ActionState.bPlaying)
	{
		const bool bStarted = StartLocalPlayback();
		SetComponentTickEnabled(!bStarted);
	}
}

float UGASPActionComponent::GetServerTime() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return 0.f;
	}
	const AGameStateBase* GameState = IsValid(World) ? World->GetGameState() : nullptr;
	return IsValid(GameState) ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
}

void UGASPActionComponent::HandleParticipantEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	FinishAction(false);
}

UAnimInstance* UGASPActionComponent::GetAnimContext(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	if (Actor->Implements<UGASPInteractionInterface>())
	{
		return IGASPInteractionInterface::Execute_GetInteractionAnimContext(Actor);
	}
	const USkeletalMeshComponent* Mesh = Actor->FindComponentByClass<USkeletalMeshComponent>();
	return IsValid(Mesh) ? Mesh->GetAnimInstance() : nullptr;
}

bool UGASPActionComponent::SelectAction(const FName ActionType, AActor* Target, TArray<FGASPInteractionRep>& OutRoles) const
{
	const AGASPCharacter* Source = Cast<AGASPCharacter>(GetOwner());
	const UGASPCharacterSettings* Settings = IsValid(Source) ? Source->GetSettingsChecked() : nullptr;
	UChooserTable* Table = IsValid(Settings) ? Settings->InteractionTable.LoadSynchronous() : nullptr;
	UAnimInstance* SourceAnimation = GetAnimContext(Source);
	UAnimInstance* TargetAnimation = GetAnimContext(Target);
	if (!IsValid(Table) || !IsValid(SourceAnimation) || !IsValid(TargetAnimation))
	{
		UE_LOGF(LogGASP, Verbose, "Action selection for '%ls' needs a Chooser and two animation instances. Source: '%ls', target: '%ls', Chooser: '%ls'.",
			*ActionType.ToString(), *GetNameSafe(Source), *GetNameSafe(Target), *GetNameSafe(Table));
		return false;
	}

	FCharacterInteractionInput Input;
	Input.InteractionType = ActionType;
	Input.Speed = Source->GetMoverComponent()->GetVelocity().Size2D();
	FCharacterInteractionOutput Output;
	FChooserEvaluationContext Context = UChooserFunctionLibrary::MakeChooserEvaluationContext();
	Context.AddStructParam(Input);
	Context.OutputArrays.Emplace(static_cast<uint32>(Context.Params.Num()));
	Context.AddStructParam(Output);
	const TArray<UObject*> Objects = UChooserFunctionLibrary::EvaluateObjectChooserBaseMulti(
		Context, UChooserFunctionLibrary::MakeEvaluateChooser(Table), UPoseSearchDatabase::StaticClass());

	TArray<FPoseSearchMotionMatchMultiQuery> Queries;
	const TArray<FInstancedStruct>& Outputs = Context.OutputArrays[0].OutputValues;
	for (int32 Index = 0; Index < Objects.Num(); ++Index)
	{
		if (!Outputs.IsValidIndex(Index))
		{
			continue;
		}
		const FCharacterInteractionOutput& Roles = Outputs[Index].Get<FCharacterInteractionOutput>();
		FPoseSearchMotionMatchMultiQuery& Query = Queries.AddDefaulted_GetRef();
		Query.Database = Cast<UPoseSearchDatabase>(Objects[Index]);
		Query.AnimContextsRoles.Add({ SourceAnimation, Roles.InitiatorRoles });
		Query.AnimContextsRoles.Add({ TargetAnimation, Roles.TargetRoles });
	}
	static const FName PoseHistoryName = TEXT("PoseHistory");
	TArray<FPoseSearchBlueprintResult> Results;
	UPoseSearchInteractionLibrary::MotionMatchMulti(Queries, PoseHistoryName, {}, Results);
	UE_LOGF(LogGASP, Verbose, "Action '%ls' from '%ls' to '%ls' evaluated Chooser '%ls': %d database(s), %d query(s), %d selected role(s).",
		*ActionType.ToString(), *GetNameSafe(Source), *GetNameSafe(Target), *GetNameSafe(Table), Objects.Num(), Queries.Num(), Results.Num());
	for (const FPoseSearchBlueprintResult& Result : Results)
	{
		FGASPInteractionRep& Role = OutRoles.AddDefaulted_GetRef();
		Role.Actor = const_cast<AActor*>(UPoseSearchLibrary::GetActor(Result));
		Role.Result = Result;
	}
	return !OutRoles.IsEmpty();
}
