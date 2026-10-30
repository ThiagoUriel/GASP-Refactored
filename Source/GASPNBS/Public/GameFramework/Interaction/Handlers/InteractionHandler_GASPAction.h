// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "GameFramework/FoundryInteractionHandler.h"
#include "InteractionHandler_GASPAction.generated.h"

/**
 * Bridges Foundry preparation and Commit to the performer's GASP Action Component.
 */
UCLASS(DisplayName = "GASP Action")
class GASPNBS_API UInteractionHandler_GASPAction : public UFoundryInteractionHandler
{

	GENERATED_BODY()

public:

	// -- Begin FoundryInteractionHandler implementation
	virtual FInteractionHandlerResult CanHandleInteractionEvent_Implementation(const FFoundryInteractionContext& Context) const override;
	virtual bool SupportsNotifications() const override { return false; }
	// -- End FoundryInteractionHandler implementation

	/**
	 * Sets the Chooser action name when authoring a native interaction definition.
	 */
	void SetActionType(FName NewActionType) { ActionType = NewActionType; }

protected:

	/**
	 * Chooser action selected when this Handler prepares an accepted Foundry interaction.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP|Actions")
	FName ActionType;

	// -- Begin FoundryInteractionHandler implementation
	virtual void ExecuteInteractionPreparation(const FFoundryInteractionContext& Context, FInteractionHandlerExecutionCallback CompletionCallback) const override;
	virtual void ExecuteInteractionCommit(const FFoundryInteractionContext& Context, FInteractionHandlerExecutionCallback CompletionCallback) const override;
	// -- End FoundryInteractionHandler implementation

};
