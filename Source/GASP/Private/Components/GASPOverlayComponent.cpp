// Ninja Bear Studio Inc., all rights reserved.
#include "Components/GASPOverlayComponent.h"

#include "Actors/GASPCharacter.h"
#include "Chooser.h"
#include "Settings/GASPCharacterSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPOverlayComponent)

void UGASPOverlayComponent::OnRegister()
{
	Super::OnRegister();
	RefreshOverlay();
}

void UGASPOverlayComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshOverlay();
}

void UGASPOverlayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	Contributions.Reset();
	EffectiveOverlayTags.Reset();
	bRefreshPending = false;
	UpdateDepth = 0;

	Super::EndPlay(EndPlayReason);
}

bool UGASPOverlayComponent::SetSourceOverlayContribution(const FGASPOverlaySourceKey& Key, const FGameplayTagContainer& Tags)
{
	if (!Key.Source.IsValid() || bEndingPlay)
	{
		return false;
	}

	if (Tags.IsEmpty())
	{
		return RemoveSourceOverlayContribution(Key);
	}

	const FGameplayTagContainer* ExistingTags = Contributions.Find(Key);
	if (ExistingTags != nullptr && *ExistingTags == Tags)
	{
		return false;
	}

	Contributions.Add(Key, Tags);
	RefreshOverlay();
	return true;
}

bool UGASPOverlayComponent::RemoveSourceOverlayContribution(const FGASPOverlaySourceKey& Key)
{
	// A teardown caller may still hold a source already marked for destruction.
	if (Contributions.Remove(Key) == 0)
	{
		return false;
	}

	RefreshOverlay();
	return true;
}

bool UGASPOverlayComponent::SetOverlayContribution(UObject* Source, FName Id, const FGameplayTagContainer& Tags)
{
	const FGASPOverlaySourceKey Key{Source, Id};
	return SetSourceOverlayContribution(Key, Tags);
}

bool UGASPOverlayComponent::RemoveOverlayContribution(UObject* Source, FName Id)
{
	if (Source == nullptr)
	{
		return false;
	}

	const FGASPOverlaySourceKey Key{Source, Id};
	return RemoveSourceOverlayContribution(Key);
}

FGameplayTagContainer UGASPOverlayComponent::GetOverlayMode() const
{
	return EffectiveOverlayTags;
}

void UGASPOverlayComponent::RefreshOverlay()
{
	if (bEndingPlay)
	{
		return;
	}

	bRefreshPending = true;
	if (bRefreshing || UpdateDepth > 0)
	{
		return;
	}

	TGuardValue<bool> RefreshGuard(bRefreshing, true);
	while (bRefreshPending && UpdateDepth == 0 && !bEndingPlay)
	{
		bRefreshPending = false;
		RefreshOverlayState();
	}
}

void UGASPOverlayComponent::BeginOverlayUpdate()
{
	++UpdateDepth;
}

void UGASPOverlayComponent::EndOverlayUpdate()
{
	if (bEndingPlay)
	{
		return;
	}

	if (!ensureMsgf(UpdateDepth > 0, TEXT("Overlay component %s requires a matching BeginOverlayUpdate before EndOverlayUpdate."), *GetNameSafe(this)))
	{
		return;
	}

	--UpdateDepth;
	if (UpdateDepth == 0 && bRefreshPending)
	{
		RefreshOverlay();
	}
}

void UGASPOverlayComponent::RefreshOverlayState()
{
	const FGameplayTagContainer PreviousTags = EffectiveOverlayTags;
	RefreshEffectiveOverlayTags();
	RefreshTargetMesh();

	// Existing Chooser bindings read character getters. Publish the complete effective tags before
	// evaluation so the character remains the context for both authored and contributed overlays.
	//
	const FGameplayTag PoseMode = ResolvePoseMode();
	const TMap<FGameplayTag, TSubclassOf<UAnimInstance>> DesiredLayers = BuildDesiredOverlayLayers(PoseMode);
	SetOverrideLayers(DesiredLayers, PoseMode);

	if (PreviousTags != EffectiveOverlayTags)
	{
		OnOverlayChanged.Broadcast(PreviousTags, EffectiveOverlayTags);
	}
}

void UGASPOverlayComponent::RefreshEffectiveOverlayTags()
{
	EffectiveOverlayTags = ResolveBaseOverlayMode();
	for (auto It = Contributions.CreateIterator(); It; ++It)
	{
		if (!It.Key().Source.IsValid())
		{
			It.RemoveCurrent();
			continue;
		}

		EffectiveOverlayTags.AppendTags(It.Value());
	}
}

void UGASPOverlayComponent::RefreshTargetMesh()
{
	const AGASPCharacter* Character = Cast<AGASPCharacter>(GetOwner());
	if (IsValid(Character))
	{
		SetTargetMesh(Character->GetMesh());
	}
}

TMap<FGameplayTag, TSubclassOf<UAnimInstance>> UGASPOverlayComponent::BuildDesiredOverlayLayers(FGameplayTag PoseMode) const
{
	TMap<FGameplayTag, TSubclassOf<UAnimInstance>> DesiredLayers;
	if (!EffectiveOverlayTags.IsEmpty())
	{
		const TSubclassOf<UAnimInstance> OverlayLayer = ResolveOverlayLayer();
		if (OverlayLayer != nullptr)
		{
			for (const FGameplayTag OverlayTag : EffectiveOverlayTags)
			{
				DesiredLayers.Add(OverlayTag, OverlayLayer);
			}
		}
	}

	if (PoseMode.IsValid())
	{
		const TSubclassOf<UAnimInstance> PoseLayer = ResolvePoseLayer();
		if (PoseLayer != nullptr)
		{
			DesiredLayers.Add(PoseMode, PoseLayer);
		}
	}

	return DesiredLayers;
}

FGameplayTagContainer UGASPOverlayComponent::ResolveBaseOverlayMode() const
{
	const AGASPCharacter* Character = Cast<AGASPCharacter>(GetOwner());
	return IsValid(Character) ? Character->GetBaseOverlayMode() : FGameplayTagContainer();
}

FGameplayTag UGASPOverlayComponent::ResolvePoseMode() const
{
	const AGASPCharacter* Character = Cast<AGASPCharacter>(GetOwner());
	return IsValid(Character) ? Character->GetPoseMode() : FGameplayTag();
}

TSubclassOf<UAnimInstance> UGASPOverlayComponent::ResolveOverlayLayer() const
{
	const AGASPCharacter* Character = Cast<AGASPCharacter>(GetOwner());
	if (!IsValid(Character))
	{
		return nullptr;
	}

	const UChooserTable* OverlayTable = Character->GetSettingsChecked()->OverlayTable.LoadSynchronous();
	return Character->GetLinkedAnimLayer(OverlayTable);
}

TSubclassOf<UAnimInstance> UGASPOverlayComponent::ResolvePoseLayer() const
{
	const AGASPCharacter* Character = Cast<AGASPCharacter>(GetOwner());
	if (!IsValid(Character))
	{
		return nullptr;
	}

	const UChooserTable* PosesTable = Character->GetSettingsChecked()->PosesTable.LoadSynchronous();
	return Character->GetLinkedAnimLayer(PosesTable);
}
