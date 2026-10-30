// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "PoseSearch/PoseSearchResult.h"

#include "GASPInteractionTypes.generated.h"

class AActor;
class UAnimMontage;
class UPoseSearchDatabase;
class UPoseSearchInteractionAsset;

/** Chooser input retaining the sample action names and movement speed. */
USTRUCT(BlueprintType)
struct GASP_API FCharacterInteractionInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	FName InteractionType{NAME_None};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	float Speed{0.f};
};

/** Chooser output row: the roles each side of the interaction is allowed to take. */
USTRUCT(BlueprintType)
struct GASP_API FCharacterInteractionOutput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	TArray<FName> InitiatorRoles{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	TArray<FName> TargetRoles{};
};

/**
 * Describes one participant and its selected animation role.
 *
 * Action Components share the selected result with observers, omitting its animation instances.
 * Each observer uses Actor to rebuild its local contexts before publishing the role to animation
 * Blueprints and motion-warping notifies. Read that local result through GetInteractionResult.
 */
USTRUCT(BlueprintType)
struct GASP_API FGASPInteractionRep
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GASP")
	TObjectPtr<AActor> Actor{nullptr};

	UPROPERTY(BlueprintReadOnly, Category = "GASP")
	FPoseSearchBlueprintResult Result{};
};
