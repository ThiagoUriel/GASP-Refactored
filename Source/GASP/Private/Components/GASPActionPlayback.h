// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Interaction/GASPActionTypes.h"
#include "UObject/Object.h"
#include "GASPActionPlayback.generated.h"

class AGASPCharacter;
class UAnimInstance;
class UAnimMontage;
class UGASPActionComponent;
class UPlayMoverMontageCallbackProxy;

/**
 * Owns one role's Mover playback and the exact temporary changes made for that role.
 */
UCLASS()
class UGASPActionPlayback : public UObject
{

	GENERATED_BODY()

public:

	/**
	 * Starts one role and installs its completion listeners without replacing Mover's delegates.
	 *
	 * @param Owner	Action receiving the role's final result.
	 * @param Handle	Execution that owns the role.
	 * @param Role		Selected Actor, montage and pose-search role.
	 * @param Contexts	Local animation contexts shared by all roles.
	 * @param Elapsed	Time since authority started playback.
	 * @return			Whether this role started successfully.
	 */
	bool Start(UGASPActionComponent* Owner, FGASPActionHandle Handle, const FGASPInteractionRep& Role, const TArray<TObjectPtr<const UObject>>& Contexts, float Elapsed);

	/**
	 * Removes only this role's listeners, role result, collision ignores and tick prerequisite.
	 */
	void Stop();

	/**
	 * Completes a matching role before its authored gameplay transition stops the montage.
	 *
	 * @param Actor	Participant requesting the handoff.
	 * @return			Whether this playback accepted the handoff for its unfinished role.
	 */
	bool CompleteForHandoff(AActor* Actor);

private:

	TWeakObjectPtr<UGASPActionComponent> Action;
	TWeakObjectPtr<AGASPCharacter> Initiator;
	TWeakObjectPtr<AGASPCharacter> Participant;
	TWeakObjectPtr<UAnimInstance> Animation;
	FGASPActionHandle ActionHandle;
	int32 MontageInstanceId = INDEX_NONE;
	bool bAddedSourceIgnore = false;
	bool bAddedTargetIgnore = false;
	bool bAddedTickPrerequisite = false;
	bool bOwnsRoleResult = false;
	bool bReported = false;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(Transient)
	TObjectPtr<UPlayMoverMontageCallbackProxy> Proxy;

	/**
	 * Reports natural completion once; blend-out alone does not complete the role.
	 */
	UFUNCTION()
	void HandleCompleted(FName NotifyName);

	/**
	 * Reports interruption once, allowing the owning action to stop the other roles.
	 */
	UFUNCTION()
	void HandleInterrupted(FName NotifyName);

	/**
	 * Cancels this role's montage move while no newer montage has taken ownership.
	 */
	void StopOwnedRootMotion();

	/**
	 * Claims completion before calling the action, because that call may stop this playback.
	 */
	void ReportResult(bool bSucceeded);

};
