// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "AbilitySystem/Abilities/InteractionAbility_Execute.h"
#include "GASPInteractionAbility_Execute.generated.h"

/**
 * Maps the sample input actions to Foundry's authoritative selection queries.
 */
UCLASS()
class GASPNBS_API UGASPInteractionAbility_Execute : public UInteractionAbility_Execute
{

	GENERATED_BODY()

public:

	UGASPInteractionAbility_Execute(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};
