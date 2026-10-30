// Ninja Bear Studio Inc., all rights reserved.
#include "Components/GASPInteractionTargetComponent.h"

#include "GameFramework/GASPActionInteractionDefinition.h"

UGASPInteractionTargetComponent::UGASPInteractionTargetComponent()
{
	FInteractionTargetSlot Shove;
	Shove.SlotName = TEXT("Shove");
	Shove.Definitions.Add(CreateDefaultSubobject<UGASPShoveInteractionDefinition>(TEXT("ShoveDefinition")));
	Slots.Add(Shove);

	FInteractionTargetSlot Takedown;
	Takedown.SlotName = TEXT("Takedown");
	Takedown.Definitions.Add(CreateDefaultSubobject<UGASPTakedownInteractionDefinition>(TEXT("TakedownDefinition")));
	Slots.Add(Takedown);
}
