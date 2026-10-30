// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "GameFramework/FoundryInteractionDefinition.h"
#include "GASPActionInteractionDefinition.generated.h"

/**
 * Supplies optional preparation and deferred Commit for synchronized GASP performances.
 */
UCLASS(Abstract)
class GASPNBS_API UGASPActionInteractionDefinition : public UFoundryInteractionDefinition
{

	GENERATED_BODY()

public:

	UGASPActionInteractionDefinition();

protected:

	/**
	 * Configures a native action definition and its two stateless Handlers.
	 *
	 * @param Tag			Foundry action tag offered by the target.
	 * @param ActionType	Chooser name used by GASP to select a performance.
	 * @param Prompt		Text shown for the action.
	 * @param Rank			Priority relative to other eligible actions.
	 */
	void ConfigureAction(FGameplayTag Tag, FName ActionType, const FText& Prompt, int32 Rank);

};

/**
 * Offers the sample's Shove performance through Foundry.
 */
UCLASS(DisplayName = "GASP Shove")
class GASPNBS_API UGASPShoveInteractionDefinition : public UGASPActionInteractionDefinition
{

	GENERATED_BODY()

public:

	UGASPShoveInteractionDefinition();

};

/**
 * Offers the sample's Takedown performance through Foundry.
 */
UCLASS(DisplayName = "GASP Takedown")
class GASPNBS_API UGASPTakedownInteractionDefinition : public UGASPActionInteractionDefinition
{

	GENERATED_BODY()

public:

	UGASPTakedownInteractionDefinition();

};
