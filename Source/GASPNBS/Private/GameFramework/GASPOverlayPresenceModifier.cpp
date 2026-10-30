// Ninja Bear Studio Inc., all rights reserved.
#include "GameFramework/GASPOverlayPresenceModifier.h"

#include "Components/GASPOverlayComponent.h"
#include "GameFramework/Actor.h"

bool UGASPOverlayPresenceModifier::CanApplyModifier_Implementation(const FInventoryPresenceContext& Context) const
{
	const bool bValidPresence = Super::CanApplyModifier_Implementation(Context) && Context.Inventory.IsValid() && Context.Item.IsValid();
	return bValidPresence && !OverlayTags.IsEmpty() && IsValid(ResolveOverlayComponent(Context));
}

void UGASPOverlayPresenceModifier::ApplyModifier_Implementation(const FInventoryPresenceContext& Context) const
{
	if (!CanApplyModifier(Context))
	{
		return;
	}

	UGASPOverlayComponent* Overlay = ResolveOverlayComponent(Context);
	if (IsValid(Overlay))
	{
		// Presence already runs locally for every observer. Register presentation intent without
		// changing the character's replicated manual overlays or creating a lazy ASC.
		//
		const FGASPOverlaySourceKey SourceKey = MakeSourceKey(this, Context);
		Overlay->SetSourceOverlayContribution(SourceKey, OverlayTags);
	}
}

void UGASPOverlayPresenceModifier::RemoveModifier_Implementation(const FInventoryPresenceContext& Context) const
{
	if (!Context.Inventory.IsValid() || !Context.Item.IsValid())
	{
		return;
	}

	UGASPOverlayComponent* Overlay = ResolveOverlayComponent(Context);
	if (IsValid(Overlay))
	{
		const FGASPOverlaySourceKey SourceKey = MakeSourceKey(this, Context);
		Overlay->RemoveSourceOverlayContribution(SourceKey);
	}
}

FGASPOverlaySourceKey UGASPOverlayPresenceModifier::MakeSourceKey(const UGASPOverlayPresenceModifier* Modifier, const FInventoryPresenceContext& Context)
{
	return {Modifier, NAME_None, Context.Inventory.Id, Context.Item.Id};
}

UGASPOverlayComponent* UGASPOverlayPresenceModifier::ResolveOverlayComponent(const FInventoryPresenceContext& Context) const
{
	return IsValid(Context.Owner) ? Context.Owner->FindComponentByClass<UGASPOverlayComponent>() : nullptr;
}
