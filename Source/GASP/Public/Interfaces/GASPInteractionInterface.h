// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "PoseSearch/PoseSearchResult.h"
#include "UObject/Interface.h"
#include "GASPInteractionInterface.generated.h"

class UAnimInstance;

UINTERFACE(BlueprintType, MinimalAPI)
class UGASPInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Gives animation Blueprints and notifies access to a participant's local synchronized action.
 *
 * Implementations provide their current animation instance and forward the role result from their
 * Action Component. Foundry owns target discovery; this interface is only the animation bridge.
 */
class GASP_API IGASPInteractionInterface
{
	GENERATED_BODY()

public:
	/**
	 * Provides the animation instance used for this participant's role selection and playback.
	 *
	 * @return	Current local instance, or null while an animated participant is unavailable.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	UAnimInstance* GetInteractionAnimContext() const;

	/**
	 * Provides the local role consumed by the animation graph and motion-warping notifies.
	 *
	 * @return	Current role result, or an empty result when no role is playing.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	FPoseSearchBlueprintResult GetInteractionResult() const;
};
