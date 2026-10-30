// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "Components/GASPActionComponent.h"
#include "GameFramework/GASPFoundryPawn.h"
#include "GASPActionTestTypes.generated.h"

class UPoseSearchInteractionAsset;

/** Records the public Foundry outcome without changing the production Pawn or handler. */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPInteractionTestObserver : public UObject
{

	GENERATED_BODY()

public:

	FInteractionResult LastResult;
	int32 CompletionCount = 0;

	/** Receives the same terminal event used by game presentation and gameplay listeners. */
	UFUNCTION()
	void HandleFinished(UFoundryInteractionComponent* Component, FInteractionDescriptor Interaction, FInteractionResult Result);

};

/** Keeps performance selection controlled while exercising real ownership and Mover playback. */
UCLASS(Hidden, NotBlueprintable, NotBlueprintType)
class UGASPTestActionComponent : public UGASPActionComponent
{

	GENERATED_BODY()

public:

	UPROPERTY(Transient)
	TObjectPtr<UPoseSearchInteractionAsset> Performance;

protected:

	// -- Begin GASPActionComponent implementation
	virtual bool CanSelectTarget(FName ActionType, const AGASPCharacter* Source, const AGASPCharacter* Target) const override;
	virtual bool SelectAction(FName ActionType, AActor* Target, TArray<FGASPInteractionRep>& OutRoles) const override;
	// -- End GASPActionComponent implementation

};

/** Isolates physics assets while retaining the production Pawn's interaction lifecycle. */
UCLASS(Hidden, NotPlaceable, NotBlueprintable, NotBlueprintType)
class AGASPActionTestPawn : public AGASPFoundryPawn
{

	GENERATED_BODY()

public:

	AGASPActionTestPawn(const FObjectInitializer& ObjectInitializer);

};
