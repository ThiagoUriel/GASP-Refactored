// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "GameFramework/FoundryInteractionTargetComponent.h"
#include "GASPInteractionTargetComponent.generated.h"

/**
 * Offers the sample actions as ordinary, editable Foundry target definitions.
 */
UCLASS(Blueprintable, ClassGroup = (GASP), meta = (BlueprintSpawnableComponent))
class GASPNBS_API UGASPInteractionTargetComponent : public UFoundryInteractionTargetComponent
{

	GENERATED_BODY()

public:

	UGASPInteractionTargetComponent();

};
