// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_GASPActionHandoff.generated.h"

/**
 * Completes an action role before a Blueprint notify performs its planned gameplay transition.
 */
UCLASS(Blueprintable, DisplayName = "GASP Action Handoff")
class GASP_API UAnimNotify_GASPActionHandoff : public UAnimNotify
{

	GENERATED_BODY()

public:

	// -- Begin AnimNotify implementation
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	// -- End AnimNotify implementation

};
