// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Interaction/GASPInteractionTypes.h"
#include "GASPActionTypes.generated.h"

/**
 * Identifies one action, so a delayed response cannot affect a later attempt.
 */
USTRUCT(BlueprintType)
struct GASP_API FGASPActionHandle
{

	GENERATED_BODY()

public:

	/**
	 * Creates an action identity from the execution that owns it.
	 *
	 * @param ExecutionId	Identity assigned by the caller to this attempt.
	 * @return				Handle carrying that identity; an empty GUID produces an invalid handle.
	 */
	static FGASPActionHandle FromExecution(const FGuid& ExecutionId)
	{
		FGASPActionHandle Handle;
		Handle.Id = ExecutionId;
		return Handle;
	}

	/**
	 * Indicates whether this handle identifies an action.
	 */
	bool IsValid() const { return Id.IsValid(); }
	bool operator==(const FGASPActionHandle& Other) const { return Id == Other.Id; }
	bool operator!=(const FGASPActionHandle& Other) const { return !(*this == Other); }

private:

	UPROPERTY()
	FGuid Id;

};

/**
 * Shares a selected performance with every connection that can see its initiator.
 */
USTRUCT()
struct FGASPActionState
{

	GENERATED_BODY()

	UPROPERTY()
	FGASPActionHandle Handle;

	UPROPERTY()
	TArray<FGASPInteractionRep> Participants;

	UPROPERTY()
	float ServerStartTime = 0.f;

	UPROPERTY()
	bool bPlaying = false;

};

DECLARE_DELEGATE_OneParam(FGASPActionCompletion, bool);
