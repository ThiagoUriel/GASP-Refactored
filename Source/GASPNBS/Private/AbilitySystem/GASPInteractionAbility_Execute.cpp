// Ninja Bear Studio Inc., all rights reserved.
#include "AbilitySystem/GASPInteractionAbility_Execute.h"

#include "GASPNBSTags.h"

UGASPInteractionAbility_Execute::UGASPInteractionAbility_Execute(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	FInteractionEventQuery Shove;
	Shove.InteractionEvent = GASP::Interaction::ExecuteShove;
	Shove.InteractionQuery = FGameplayTagQuery::MakeQuery_MatchTag(GASP::Interaction::Shove);
	InteractionEventQueries.Add(Shove);

	FInteractionEventQuery Takedown;
	Takedown.InteractionEvent = GASP::Interaction::ExecuteTakedown;
	Takedown.InteractionQuery = FGameplayTagQuery::MakeQuery_MatchTag(GASP::Interaction::Takedown);
	InteractionEventQueries.Add(Takedown);
}
