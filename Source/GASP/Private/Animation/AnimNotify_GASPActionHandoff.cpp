// Ninja Bear Studio Inc., all rights reserved.
#include "Animation/AnimNotify_GASPActionHandoff.h"

#include "Components/GASPActionComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotify_GASPActionHandoff)

void UAnimNotify_GASPActionHandoff::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Actor = IsValid(MeshComp) ? MeshComp->GetOwner() : nullptr;
	UGASPActionComponent* Actions = IsValid(Actor) ? Actor->FindComponentByClass<UGASPActionComponent>() : nullptr;
	if (IsValid(Actions))
	{
		Actions->CompleteRoleForHandoff();
	}
	Super::Notify(MeshComp, Animation, EventReference);
}
