// Ninja Bear Studio Inc., all rights reserved.
#include "GameFramework/Interaction/Handlers/InteractionHandler_GASPAction.h"

#include "Components/GASPActionComponent.h"
#include "GameFramework/GASPFoundryPawn.h"

FInteractionHandlerResult UInteractionHandler_GASPAction::CanHandleInteractionEvent_Implementation(const FFoundryInteractionContext& Context) const
{
	FInteractionHandlerResult Result = Super::CanHandleInteractionEvent_Implementation(Context);
	const AGASPFoundryPawn* Pawn = Cast<AGASPFoundryPawn>(Context.InstigatorActor);
	const UGASPActionComponent* Actions = IsValid(Pawn) ? Pawn->GetActionComponent() : nullptr;
	const FGASPActionHandle Handle = FGASPActionHandle::FromExecution(Context.ExecutionId);
	Result.bCanHandle &= IsValid(Actions) && Actions->CanPerformAction(ActionType, Context.Interaction.TargetActor, Handle);
	return Result;
}

void UInteractionHandler_GASPAction::ExecuteInteractionPreparation(const FFoundryInteractionContext& Context, FInteractionHandlerExecutionCallback CompletionCallback) const
{
	AGASPFoundryPawn* Pawn = Cast<AGASPFoundryPawn>(Context.InstigatorActor);
	UGASPActionComponent* Actions = IsValid(Pawn) ? Pawn->GetActionComponent() : nullptr;
	const FGASPActionHandle Handle = FGASPActionHandle::FromExecution(Context.ExecutionId);
	const bool bPrepared = IsValid(Actions) && Pawn->TrackInteractionAction(Handle)
		&& Actions->PrepareAction(Handle, ActionType, Context.Interaction.TargetActor);
	const FInteractionHandlerExecutionResult Result = bPrepared
		? FInteractionHandlerExecutionResult::Succeeded()
		: FInteractionHandlerExecutionResult::Failed(TEXT("No ready GASP performance is available for the selected target."));
	CompletionCallback.ExecuteIfBound(Result);
}

void UInteractionHandler_GASPAction::ExecuteInteractionCommit(const FFoundryInteractionContext& Context, FInteractionHandlerExecutionCallback CompletionCallback) const
{
	AGASPFoundryPawn* Pawn = Cast<AGASPFoundryPawn>(Context.InstigatorActor);
	UGASPActionComponent* Actions = IsValid(Pawn) ? Pawn->GetActionComponent() : nullptr;
	const FGASPActionHandle Handle = FGASPActionHandle::FromExecution(Context.ExecutionId);
	FGASPActionCompletion ActionCompletion;
	ActionCompletion.BindLambda([CompletionCallback, Cancellation = Context.ExecutionCancellation](const bool bSucceeded)
	{
		if (Cancellation.IsValid() && Cancellation->bCancelled)
		{
			return;
		}
		const FInteractionHandlerExecutionResult Result = bSucceeded
			? FInteractionHandlerExecutionResult::Succeeded()
			: FInteractionHandlerExecutionResult::Failed(TEXT("The GASP performance was interrupted."));
		CompletionCallback.ExecuteIfBound(Result);
	});
	const bool bAccepted = IsValid(Actions) && Actions->CommitAction(Handle, MoveTemp(ActionCompletion));
	if (!bAccepted)
	{
		CompletionCallback.ExecuteIfBound(FInteractionHandlerExecutionResult::Failed(TEXT("The prepared GASP action is no longer ready to Commit.")));
	}
}
