// Ninja Bear Studio Inc., all rights reserved.
#include "Components/GASPOverrideModeManager.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPOverrideModeManager)

UGASPOverrideModeManager::UGASPOverrideModeManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGASPOverrideModeManager::OnRegister()
{
	Super::OnRegister();

	AActor* Owner = GetOwner();
	if (IsValid(Owner))
	{
		SetTargetMesh(Owner->FindComponentByClass<USkeletalMeshComponent>());
	}
}

void UGASPOverrideModeManager::OnUnregister()
{
	SetTargetMesh(nullptr);
	Super::OnUnregister();
}

void UGASPOverrideModeManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LinkedOverrideLayers.Reset();
	SetTargetMesh(nullptr);
	ReleaseLayers();

	Super::EndPlay(EndPlayReason);
}

void UGASPOverrideModeManager::AddOverrideLayer(FGameplayTag OverrideTag, TSubclassOf<UAnimInstance> OverrideAnimClass)
{
	if (!OverrideTag.IsValid() || OverrideAnimClass == nullptr)
	{
		return;
	}

	const TSubclassOf<UAnimInstance>* ExistingLayer = LinkedOverrideLayers.Find(OverrideTag);
	if (ExistingLayer != nullptr && *ExistingLayer == OverrideAnimClass)
	{
		return;
	}

	const bool bAdded = ExistingLayer == nullptr;
	LinkedOverrideLayers.Add(OverrideTag, OverrideAnimClass);
	ReconcileLayers();

	if (bAdded)
	{
		OnOverrideLayerChanged.Broadcast(OverrideTag, true);
	}
}

void UGASPOverrideModeManager::RemoveOverrideLayer(FGameplayTag OverrideTag)
{
	if (LinkedOverrideLayers.Remove(OverrideTag) == 0)
	{
		return;
	}

	ReconcileLayers();
	OnOverrideLayerChanged.Broadcast(OverrideTag, false);
}

void UGASPOverrideModeManager::SetTargetMesh(USkeletalMeshComponent* Mesh)
{
	if (TargetMesh.Get() == Mesh)
	{
		return;
	}

	ReleaseLayers();
	USkeletalMeshComponent* PreviousMesh = TargetMesh.Get();
	if (IsValid(PreviousMesh))
	{
		PreviousMesh->OnAnimInitialized.RemoveDynamic(this, &ThisClass::HandleAnimInitialized);
	}

	TargetMesh = Mesh;
	if (IsValid(Mesh))
	{
		Mesh->OnAnimInitialized.AddUniqueDynamic(this, &ThisClass::HandleAnimInitialized);
	}

	ReconcileLayers();
}

void UGASPOverrideModeManager::SetOverrideLayers(const TMap<FGameplayTag, TSubclassOf<UAnimInstance>>& Layers, FGameplayTag PriorityTag)
{
	TArray<FGameplayTag> RemovedTags;
	TArray<FGameplayTag> AddedTags;
	for (const TPair<FGameplayTag, TSubclassOf<UAnimInstance>>& LayerRequest : LinkedOverrideLayers)
	{
		if (!Layers.Contains(LayerRequest.Key))
		{
			RemovedTags.Add(LayerRequest.Key);
		}
	}

	for (const TPair<FGameplayTag, TSubclassOf<UAnimInstance>>& LayerRequest : Layers)
	{
		const bool bValidRequest = LayerRequest.Key.IsValid() && LayerRequest.Value != nullptr;
		if (bValidRequest && !LinkedOverrideLayers.Contains(LayerRequest.Key))
		{
			AddedTags.Add(LayerRequest.Key);
		}
	}

	LinkedOverrideLayers.Reset();
	for (const TPair<FGameplayTag, TSubclassOf<UAnimInstance>>& LayerRequest : Layers)
	{
		if (LayerRequest.Key.IsValid() && LayerRequest.Value != nullptr)
		{
			LinkedOverrideLayers.Add(LayerRequest.Key, LayerRequest.Value);
		}
	}

	LastPriorityTag = PriorityTag;
	ReconcileLayers();

	for (const FGameplayTag RemovedTag : RemovedTags)
	{
		OnOverrideLayerChanged.Broadcast(RemovedTag, false);
	}

	for (const FGameplayTag AddedTag : AddedTags)
	{
		OnOverrideLayerChanged.Broadcast(AddedTag, true);
	}
}

void UGASPOverrideModeManager::ReconcileLayers(bool bForce)
{
	bReconcilePending = true;
	bForcePending |= bForce;
	if (bReconciling)
	{
		return;
	}

	TGuardValue<bool> ReconcileGuard(bReconciling, true);
	while (bReconcilePending)
	{
		bReconcilePending = false;
		const bool bReapply = bForcePending;
		bForcePending = false;

		const TArray<TSubclassOf<UAnimInstance>> DesiredLayers = BuildDesiredLayerStack();
		ApplyLayerStack(DesiredLayers, bReapply);
	}
}

TArray<TSubclassOf<UAnimInstance>> UGASPOverrideModeManager::BuildDesiredLayerStack() const
{
	TArray<FGameplayTag> RequestTags;
	LinkedOverrideLayers.GetKeys(RequestTags);
	RequestTags.Sort([this](FGameplayTag FirstTag, FGameplayTag SecondTag)
	{
		const bool bFirstHasPriority = FirstTag == LastPriorityTag;
		const bool bSecondHasPriority = SecondTag == LastPriorityTag;
		if (bFirstHasPriority != bSecondHasPriority)
		{
			return bSecondHasPriority;
		}

		return FirstTag.ToString() < SecondTag.ToString();
	});

	TArray<TSubclassOf<UAnimInstance>> DesiredLayers;
	for (const FGameplayTag RequestTag : RequestTags)
	{
		// Shared classes occupy their highest-priority position and are linked only once.
		const TSubclassOf<UAnimInstance> Layer = LinkedOverrideLayers.FindChecked(RequestTag);
		DesiredLayers.Remove(Layer);
		DesiredLayers.Add(Layer);
	}

	return DesiredLayers;
}

void UGASPOverrideModeManager::ApplyLayerStack(const TArray<TSubclassOf<UAnimInstance>>& DesiredLayers, bool bForce)
{
	UAnimInstance* Instance = ResolveAnimationInstance();
	if (AppliedInstance.Get() != Instance)
	{
		ReleaseLayers();
	}

	if (!IsValid(Instance))
	{
		return;
	}

	const bool bStackUnchanged = AppliedLayers == DesiredLayers && AppliedInstance.Get() == Instance;
	if (!bForce && bStackUnchanged)
	{
		return;
	}

	for (const TSubclassOf<UAnimInstance> Layer : AppliedLayers)
	{
		if (!DesiredLayers.Contains(Layer))
		{
			UnlinkLayer(Instance, Layer);
		}
	}

	// Reassert the ordered stack when it changes: removing an overriding class must restore
	// retained lower-priority layers. An unchanged stack makes no engine calls.
	//
	AppliedInstance = Instance;
	AppliedLayers = DesiredLayers;
	for (const TSubclassOf<UAnimInstance> Layer : DesiredLayers)
	{
		LinkLayer(Instance, Layer);
	}
}

void UGASPOverrideModeManager::ReleaseLayers()
{
	UAnimInstance* Instance = AppliedInstance.Get();
	if (IsValid(Instance))
	{
		for (const TSubclassOf<UAnimInstance> Layer : AppliedLayers)
		{
			UnlinkLayer(Instance, Layer);
		}
	}

	AppliedInstance.Reset();
	AppliedLayers.Reset();
}

UAnimInstance* UGASPOverrideModeManager::ResolveAnimationInstance() const
{
	USkeletalMeshComponent* Mesh = TargetMesh.Get();
	return IsValid(Mesh) ? Mesh->GetAnimInstance() : nullptr;
}

void UGASPOverrideModeManager::LinkLayer(UAnimInstance* Instance, TSubclassOf<UAnimInstance> Layer)
{
	checkf(IsValid(Instance), TEXT("Override manager %s requires a valid Animation Instance when linking layers."), *GetNameSafe(this));
	Instance->LinkAnimClassLayers(Layer);
}

void UGASPOverrideModeManager::UnlinkLayer(UAnimInstance* Instance, TSubclassOf<UAnimInstance> Layer)
{
	checkf(IsValid(Instance), TEXT("Override manager %s requires a valid Animation Instance when unlinking layers."), *GetNameSafe(this));
	Instance->UnlinkAnimClassLayers(Layer);
}

void UGASPOverrideModeManager::HandleAnimInitialized()
{
	ReconcileLayers(true);
}
