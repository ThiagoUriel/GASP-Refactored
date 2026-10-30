// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "GameFramework/FoundryInventoryPresenceModifier.h"
#include "GASPOverlayPresenceModifier.generated.h"

class UGASPOverlayComponent;

struct FGASPOverlaySourceKey;

/**
 * Contributes overlay tags while a Foundry Presence is active on its local visual owner.
 *
 * Tags are combined with other sources before GASP evaluates its Chooser. Inventory and Item
 * identities keep each contribution independent, even when Items share this modifier instance.
 */
UCLASS(DisplayName = "GASP Overlay Presence Modifier")
class GASPNBS_API UGASPOverlayPresenceModifier : public UFoundryInventoryPresenceModifier
{

	GENERATED_BODY()

public:

	// -- Begin FoundryInventoryPresenceModifier implementation
	virtual bool CanApplyModifier_Implementation(const FInventoryPresenceContext& Context) const override;
	virtual void ApplyModifier_Implementation(const FInventoryPresenceContext& Context) const override;
	virtual void RemoveModifier_Implementation(const FInventoryPresenceContext& Context) const override;
	// -- End FoundryInventoryPresenceModifier implementation

protected:

	/** Tags evaluated with all other active sources by the character's Overlay Chooser. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASP|Overlay", meta = (Categories = "GASP.Overlay"))
	FGameplayTagContainer OverlayTags;

	/**
	 * Builds the stable identity shared by apply and remove for one Item Presence.
	 *
	 * @param Modifier	Modifier that owns the contribution.
	 * @param Context	Presence context supplying Inventory and Item identities.
	 * @return			Contribution key that distinguishes shared modifier instances by runtime identity.
	 */
	static FGASPOverlaySourceKey MakeSourceKey(const UGASPOverlayPresenceModifier* Modifier, const FInventoryPresenceContext& Context);

	/**
	 * Resolves the overlay component on the local visual owner.
	 *
	 * @param Context	Presence context whose owner receives the contribution.
	 * @return			Overlay component, or null when the visual owner does not provide one.
	 */
	virtual UGASPOverlayComponent* ResolveOverlayComponent(const FInventoryPresenceContext& Context) const;

};
