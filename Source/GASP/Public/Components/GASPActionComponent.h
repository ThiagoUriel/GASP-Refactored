// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/GASPActionTypes.h"
#include "GASPActionComponent.generated.h"

class AGASPCharacter;
class UAnimInstance;
class UGASPActionPlayback;

/**
 * Selects and plays synchronized Mover actions for an explicitly selected target.
 *
 * Authority prepares the performance and reserves its participants before Commit starts playback.
 * The caller owns that prepared action until completion or cancellation. Playback is replicated to
 * observers independently of any interaction UI. This component does not discover or rank targets.
 */
UCLASS(Blueprintable, ClassGroup = (GASP), meta = (BlueprintSpawnableComponent))
class GASP_API UGASPActionComponent : public UActorComponent
{

	GENERATED_BODY()

public:

	UGASPActionComponent();

	// -- Begin ActorComponent implementation
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnregister() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// -- End ActorComponent implementation

	/**
	 * Checks feasibility without selecting a performance or taking participant ownership.
	 *
	 * Overrides may add gameplay requirements and should call Super to preserve participant checks.
	 * Focus queries use an empty handle; prepared execution may pass its existing handle.
	 *
	 * @param ActionType	Chooser action name requested by the caller.
	 * @param Target		Explicitly selected partner.
	 * @param Handle		Existing execution, or an empty handle for a feasibility query.
	 * @return				Whether both participants can currently take part.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "GASP|Actions")
	bool CanPerformAction(FName ActionType, AActor* Target, FGASPActionHandle Handle) const;

	/**
	 * Selects a performance and reserves every role without starting animation.
	 *
	 * Call on authority with a fresh execution identity. On success, Commit or Cancel must follow.
	 * Rejected preparation takes no ownership and performs no animation.
	 *
	 * @param Handle		Execution that will own the selected performance.
	 * @param ActionType	Chooser action name requested by the caller.
	 * @param Target		Explicitly selected partner, retained throughout selection.
	 * @return				Whether preparation succeeded and the participants are reserved.
	 */
	bool PrepareAction(FGASPActionHandle Handle, FName ActionType, AActor* Target);

	/**
	 * Starts a prepared performance and waits for every selected role to finish.
	 *
	 * @param Handle				Prepared execution to consume.
	 * @param CompletionCallback	Receives success or interruption once on the game thread.
	 * @return						Whether Commit was accepted; rejection does not invoke the callback.
	 */
	bool CommitAction(FGASPActionHandle Handle, FGASPActionCompletion CompletionCallback);

	/**
	 * Releases an action only when its identity matches the current execution.
	 *
	 * @param Handle	Execution to cancel; stale identities have no effect.
	 */
	void CancelAction(FGASPActionHandle Handle);

	/**
	 * Cancels the action using this Actor, including an action initiated by another participant.
	 */
	void CancelReservedAction();

	/**
	 * Indicates whether this component has prepared the supplied execution.
	 *
	 * @param Handle	Execution whose preparation is being checked.
	 * @return			Whether this execution can still be submitted to Commit.
	 */
	bool IsPreparedFor(FGASPActionHandle Handle) const;

	/**
	 * Indicates whether an action has reserved this Actor.
	 */
	bool IsReserved() const { return ReservationOwner.IsValid(); }

	/**
	 * Provides this Actor's locally reconstructed role for animation and motion-warping consumers.
	 */
	UFUNCTION(BlueprintPure, Category = "GASP|Actions")
	FPoseSearchBlueprintResult GetActionResult() const { return ActionResult; }

	/**
	 * Publishes one role's local animation context while its playback owns this Actor.
	 *
	 * @param Handle			Execution publishing the role.
	 * @param Result			Locally reconstructed role, including animation contexts.
	 * @param ExecutionOwner	Component coordinating the complete performance.
	 * @return					Whether no other execution already owns this Actor's role.
	 */
	bool RegisterRolePlayback(FGASPActionHandle Handle, const FPoseSearchBlueprintResult& Result, UGASPActionComponent* ExecutionOwner);

	/**
	 * Releases a role only when it belongs to the supplied execution.
	 *
	 * @param Handle	Execution releasing its local role.
	 */
	void UnregisterRolePlayback(FGASPActionHandle Handle);

	/**
	 * Finishes this Actor's role before an authored animation hands control to another gameplay action.
	 *
	 * Use from a completion notify before a planned transition such as ragdoll. External montage
	 * interruptions remain failures. An Actor without a current role has nothing to complete.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Actions")
	void CompleteRoleForHandoff();

	/**
	 * Resolves an Actor's animation context through the sample interface or its skeletal mesh.
	 *
	 * @param Actor	Actor whose animation will participate.
	 * @return			Current animation instance, or null when unavailable.
	 */
	static UAnimInstance* GetAnimContext(const AActor* Actor);

	/**
	 * Receives one playback's result, ignoring duplicates and responses for older actions.
	 *
	 * @param Handle		Action that created the playback.
	 * @param Participant	Actor whose role ended.
	 * @param bSucceeded	Whether its montage completed without interruption.
	 */
	void ReportPlaybackCompletion(FGASPActionHandle Handle, AActor* Participant, bool bSucceeded);

protected:

	/**
	 * Maximum distance at which the sample offers a synchronized action.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASP|Actions", meta = (ClampMin = 0.f, Units = "cm"))
	float MaximumTargetDistance = 200.f;

	/**
	 * Maximum height difference accepted by the sample's paired animations.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASP|Actions", meta = (ClampMin = 0.f, Units = "cm"))
	float MaximumTargetHeightDifference = 50.f;

	/**
	 * Half-angle of the forward cone in which a target can be selected.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GASP|Actions", meta = (ClampMin = 0.f, ClampMax = 180.f, Units = "deg"))
	float TargetConeHalfAngle = 60.f;

	/**
	 * Applies the sample's movement, aiming, reach and obstruction requirements.
	 *
	 * Override to support another action's gameplay requirements. Participant ownership and animation
	 * availability remain enforced by the caller, independently of this selection policy.
	 *
	 * @param ActionType	Requested Chooser action name.
	 * @param Source		Validated initiator.
	 * @param Target		Validated partner.
	 * @return				Whether gameplay and spatial requirements allow the selected pair.
	 */
	virtual bool CanSelectTarget(FName ActionType, const AGASPCharacter* Source, const AGASPCharacter* Target) const;

	/**
	 * Selects a performance for this initiator and the caller's exact target.
	 *
	 * Override to adapt selection while retaining the caller's participant choice. The returned roles
	 * must each have a valid montage and animation context; preparation validates them before claiming.
	 *
	 * @param ActionType	Chooser action name requested by the caller.
	 * @param Target		Partner selected by the caller.
	 * @param OutRoles		Selected roles, including the initiator and target.
	 * @return				Whether a complete performance was selected.
	 */
	virtual bool SelectAction(FName ActionType, AActor* Target, TArray<FGASPInteractionRep>& OutRoles) const;

private:

	TWeakObjectPtr<UGASPActionComponent> ReservationOwner;
	TWeakObjectPtr<UGASPActionComponent> RoleExecutionOwner;
	FGASPActionHandle ReservationHandle;
	FGASPActionHandle PreparedHandle;
	FGASPActionHandle LocalPlaybackHandle;
	FGASPActionHandle PresentedHandle;
	FName PreparedActionType;
	TWeakObjectPtr<AActor> PreparedTarget;
	TSet<TWeakObjectPtr<AActor>> PendingParticipants;
	FGASPActionCompletion CompletionCallback;
	bool bFinishing = false;
	bool bPreparing = false;
	bool bCommitting = false;
	FGASPActionHandle LocalRoleHandle;

	/**
	 * Animation instances are local resources and must never be replicated through Pawn task state.
	 */
	UPROPERTY(Transient)
	FPoseSearchBlueprintResult ActionResult;

	UPROPERTY(Transient)
	TArray<FGASPInteractionRep> PreparedRoles;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGASPActionPlayback>> Playbacks;

	UPROPERTY(ReplicatedUsing = OnRep_ActionState)
	FGASPActionState ActionState;

	/**
	 * Routes a matching participant's authored handoff to its owned playback.
	 */
	void SignalRoleHandoff(FGASPActionHandle Handle, AActor* Participant);

	/**
	 * Validates the selected roles without reserving or starting any participant.
	 */
	bool AreRolesReady(const TArray<FGASPInteractionRep>& Roles, FGASPActionHandle Handle) const;

	/**
	 * Starts every role locally, reconstructing animation contexts omitted from replication.
	 */
	bool StartLocalPlayback();

	/**
	 * Releases owned playbacks, reservations and subscriptions before reporting the outcome.
	 */
	void FinishAction(bool bSucceeded);

	/**
	 * Removes local animation and presentation work while retaining the replicated record.
	 */
	void StopLocalPlayback();

	/**
	 * Provides synchronized server time for observers that receive a performance after it starts.
	 */
	float GetServerTime() const;

	/**
	 * Applies the latest authority-selected performance on observers.
	 */
	UFUNCTION()
	void OnRep_ActionState();

	/**
	 * Cancels when any reserved participant leaves play.
	 */
	UFUNCTION()
	void HandleParticipantEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

};
