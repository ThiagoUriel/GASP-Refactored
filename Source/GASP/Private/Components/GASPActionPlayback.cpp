// Ninja Bear Studio Inc., all rights reserved.
#include "Components/GASPActionPlayback.h"

#include "Actors/GASPCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/GASPActionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "MoveLibrary/PlayMoverMontageCallbackProxy.h"
#include "MovementSet/GASPMoverComponent.h"
#include "DefaultMovementSet/LayeredMoves/AnimRootMotionLayeredMove.h"
#include "PoseSearch/PoseSearchInteractionAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPActionPlayback)

bool UGASPActionPlayback::Start(UGASPActionComponent* Owner, const FGASPActionHandle Handle,
	const FGASPInteractionRep& Role, const TArray<TObjectPtr<const UObject>>& Contexts, const float Elapsed)
{
	Action = Owner;
	ActionHandle = Handle;
	Initiator = Cast<AGASPCharacter>(Owner->GetOwner());
	Participant = Cast<AGASPCharacter>(Role.Actor);
	Animation = UGASPActionComponent::GetAnimContext(Role.Actor);
	const UPoseSearchInteractionAsset* Asset = Cast<UPoseSearchInteractionAsset>(Role.Result.SelectedAnim);
	Montage = IsValid(Asset) ? Cast<UAnimMontage>(Asset->GetAnimationAsset(Role.Result.Role)) : nullptr;
	if (!Participant.IsValid() || !Initiator.IsValid() || !Animation.IsValid() || !IsValid(Montage))
	{
		return false;
	}

	const float StartPosition = Role.Result.SelectedTime + Elapsed * Montage->RateScale;
	if (StartPosition >= Montage->GetPlayLength())
	{
		return false;
	}
	Proxy = UPlayMoverMontageCallbackProxy::CreateProxyObjectForPlayMoverMontage(
		Participant->GetMoverComponent(), Montage, 1.f, StartPosition, NAME_None, false);
	FAnimMontageInstance* Instance = Animation->GetActiveInstanceForMontage(Montage);
	if (!IsValid(Proxy) || Instance == nullptr)
	{
		return false;
	}

	MontageInstanceId = Instance->GetInstanceID();
	Proxy->OnCompleted.AddUniqueDynamic(this, &ThisClass::HandleCompleted);
	Proxy->OnInterrupted.AddUniqueDynamic(this, &ThisClass::HandleInterrupted);

	FPoseSearchBlueprintResult Result = Role.Result;
	Result.AnimContexts = Contexts;
	UGASPActionComponent* ParticipantActions = Participant->FindComponentByClass<UGASPActionComponent>();
	if (!IsValid(ParticipantActions) || !ParticipantActions->RegisterRolePlayback(Handle, Result, Owner))
	{
		Stop();
		return false;
	}
	bOwnsRoleResult = true;

	if (Participant != Initiator)
	{
		UCapsuleComponent* SourceCapsule = Initiator->GetCapsuleComponent();
		UCapsuleComponent* TargetCapsule = Participant->GetCapsuleComponent();
		bAddedSourceIgnore = !SourceCapsule->GetMoveIgnoreComponents().Contains(TargetCapsule);
		bAddedTargetIgnore = !TargetCapsule->GetMoveIgnoreComponents().Contains(SourceCapsule);
		if (bAddedSourceIgnore)
		{
			SourceCapsule->IgnoreComponentWhenMoving(TargetCapsule, true);
		}
		if (bAddedTargetIgnore)
		{
			TargetCapsule->IgnoreComponentWhenMoving(SourceCapsule, true);
		}

		USkeletalMeshComponent* SourceMesh = Initiator->GetMesh();
		USkeletalMeshComponent* TargetMesh = Participant->GetMesh();
		bAddedTickPrerequisite = !SourceMesh->PrimaryComponentTick.GetPrerequisites().ContainsByPredicate([TargetMesh](const FTickPrerequisite& Prerequisite)
		{
			return Prerequisite.Get() == &TargetMesh->PrimaryComponentTick;
		});
		if (bAddedTickPrerequisite)
		{
			SourceMesh->AddTickPrerequisiteComponent(TargetMesh);
		}
	}
	return true;
}

void UGASPActionPlayback::Stop()
{
	if (IsValid(Proxy))
	{
		Proxy->OnCompleted.RemoveDynamic(this, &ThisClass::HandleCompleted);
		Proxy->OnInterrupted.RemoveDynamic(this, &ThisClass::HandleInterrupted);
	}

	UAnimInstance* Instance = Animation.Get();
	FAnimMontageInstance* ActiveMontage = IsValid(Instance) ? Instance->GetActiveInstanceForMontage(Montage) : nullptr;
	if (ActiveMontage != nullptr && ActiveMontage->GetInstanceID() == MontageInstanceId)
	{
		StopOwnedRootMotion();
		Instance->Montage_Stop(0.1f, Montage);
	}

	if (Participant.IsValid() && bOwnsRoleResult)
	{
		if (UGASPActionComponent* ParticipantActions = Participant->FindComponentByClass<UGASPActionComponent>())
		{
			ParticipantActions->UnregisterRolePlayback(ActionHandle);
		}
	}
	if (Participant.IsValid() && Initiator.IsValid() && Participant != Initiator)
	{
		if (bAddedSourceIgnore)
		{
			Initiator->GetCapsuleComponent()->IgnoreComponentWhenMoving(Participant->GetCapsuleComponent(), false);
		}
		if (bAddedTargetIgnore)
		{
			Participant->GetCapsuleComponent()->IgnoreComponentWhenMoving(Initiator->GetCapsuleComponent(), false);
		}
		if (bAddedTickPrerequisite)
		{
			Initiator->GetMesh()->RemoveTickPrerequisiteComponent(Participant->GetMesh());
		}
	}

	bOwnsRoleResult = false;
	bAddedSourceIgnore = false;
	bAddedTargetIgnore = false;
	bAddedTickPrerequisite = false;
	MontageInstanceId = INDEX_NONE;
	Proxy = nullptr;
	Action.Reset();
}

bool UGASPActionPlayback::CompleteForHandoff(AActor* Actor)
{
	if (Participant != Actor || bReported)
	{
		return false;
	}
	StopOwnedRootMotion();
	ReportResult(true);
	return true;
}

void UGASPActionPlayback::StopOwnedRootMotion()
{
	if (!Participant.IsValid() || !Animation.IsValid() || !IsValid(Montage)
		|| !Montage->HasRootMotion() || MontageInstanceId == INDEX_NONE)
	{
		return;
	}
	const FAnimMontageInstance* Active = Animation->GetActiveInstanceForMontage(Montage);
	const bool bOwnsActiveMontage = Active != nullptr && Active->GetInstanceID() == MontageInstanceId;
	const bool bNoNewerMontage = Animation->GetCurrentActiveMontage() == nullptr;
	if (bOwnsActiveMontage || bNoNewerMontage)
	{
		// Mover's callback proxy does not end its layered move when the montage is interrupted.
		// A newer montage keeps its move; otherwise this role still owns the montage movement tag.
		//
		Participant->GetMoverComponent()->CancelFeaturesWithTag(Mover_AnimRootMotion_Montage, false);
	}
}

void UGASPActionPlayback::HandleCompleted(FName NotifyName)
{
	ReportResult(true);
}

void UGASPActionPlayback::HandleInterrupted(FName NotifyName)
{
	StopOwnedRootMotion();
	ReportResult(false);
}

void UGASPActionPlayback::ReportResult(const bool bSucceeded)
{
	if (bReported)
	{
		return;
	}
	bReported = true;
	if (UGASPActionComponent* Owner = Action.Get())
	{
		Owner->ReportPlaybackCompletion(ActionHandle, Participant.Get(), bSucceeded);
	}
}
