// Ninja Bear Studio Inc., all rights reserved.
#include "GameFramework/GASPActionInteractionDefinition.h"

#include "GASPNBSTags.h"
#include "GameFramework/Interaction/Handlers/InteractionHandler_GASPAction.h"

UGASPActionInteractionDefinition::UGASPActionInteractionDefinition()
{
	InteractionFlow = EInteractionFlow::Simple;
	bAllowFocusWhenUnavailable = false;
	bOverrideMaximumCommitExecutionTime = true;
	MaximumCommitExecutionTime = 30.f;
	InteractionPreparationHandler = CreateDefaultSubobject<UInteractionHandler_GASPAction>(TEXT("Preparation"));
	InteractionCommittedHandler = CreateDefaultSubobject<UInteractionHandler_GASPAction>(TEXT("Commit"));
}

void UGASPActionInteractionDefinition::ConfigureAction(const FGameplayTag Tag, const FName ActionType, const FText& Prompt, const int32 Rank)
{
	InteractionTag = Tag;
	DisplayText = Prompt;
	Priority = Rank;
	CastChecked<UInteractionHandler_GASPAction>(InteractionPreparationHandler)->SetActionType(ActionType);
	CastChecked<UInteractionHandler_GASPAction>(InteractionCommittedHandler)->SetActionType(ActionType);
}

UGASPShoveInteractionDefinition::UGASPShoveInteractionDefinition()
{
	ConfigureAction(GASP::Interaction::Shove, TEXT("Shove"), NSLOCTEXT("GASPInteraction", "Shove", "Shove"), 10);
}

UGASPTakedownInteractionDefinition::UGASPTakedownInteractionDefinition()
{
	ConfigureAction(GASP::Interaction::Takedown, TEXT("Takedown"), NSLOCTEXT("GASPInteraction", "Takedown", "Takedown"), 5);
}
