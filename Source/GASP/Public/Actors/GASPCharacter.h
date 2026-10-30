#pragma once

#include "GameplayTagContainer.h"
#include "GameFramework/CruciblePawn.h"
#include "Components/GASPOverrideModeManager.h"
#include "Interfaces/GASPTargetedActor.h"
#include "MoverSimulationTypes.h"
#include "Tasks/TaskDataTypes.h"
#include "Types/EnumTypes.h"
#include "Types/MovementTypes.h"
#include "Types/StructTypes.h"
#include "Types/TagTypes.h"
#include "GameplayTaskOwnerInterface.h"
#include "GASPCharacter.generated.h"

class UAnimInstance;
class UCapsuleComponent;
class UCharacterTask_Ragdoll;
class UChooserTable;
class UGASPCharacterSettings;
class UGASPMoverComponent;
class UGASPOverlayComponent;
class UGASPOverrideModeManager;
class UGASPPhysicsControlComponent;
class UGASPTraversalComponent;
class UMotionWarpingComponent;
class UNavMoverComponent;
class USkeletalMeshComponent;

struct FMontageBlendSettings;
struct FTraversalCheckInputs;

enum class EStanceMode : uint8;

UCLASS()
class GASP_API AGASPCharacter : public ACruciblePawn, public IMoverInputProducerInterface, public IGASPTargetedActor,
                                public IGameplayTagAssetInterface, public IGameplayTaskOwnerInterface
{
	GENERATED_BODY()

	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh;
	/** Vertical capsule used by Mover for movement collision. */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UGASPPhysicsControlComponent> PhysicsControlComponent;
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UGASPOverrideModeManager> OverrideModeManager;

	UPROPERTY(EditDefaultsOnly, BlueprintGetter=GetPoseMode, ReplicatedUsing=OnRep_PoseMode, Transient)
	FGameplayTag PoseMode{PoseModeTags::Default};
	UPROPERTY(BlueprintGetter=GetLocomotionAction, ReplicatedUsing=OnRep_LocomotionAction, Transient)
	FGameplayTag LocomotionAction{FGameplayTag::EmptyTag};

	UPROPERTY(BlueprintGetter=GetOverlayMode, BlueprintSetter=SetOverlayMode,
		ReplicatedUsing=OnRep_OverlayMode, Transient)
	FGameplayTagContainer OverlayTagContainer{};

protected:

	/** Suppresses locomotion input after the Crucible death flow begins. */
	bool bMovementDisabledByDeath = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UGASPCharacterSettings> Settings;

	UPROPERTY(BlueprintReadOnly)
	float DebugAngle{0.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGASPTraversalComponent> TraversalComponent{};

	UPROPERTY(BlueprintReadOnly)
	FGASPMoverInputs MoverInputs_PostSim{};

	UFUNCTION()
	void OnBasedMovementApplied(const FTransform& TransformDelta, const FMoverTimeStep& TimeStep);

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;

protected:
	UFUNCTION()
	virtual void OnMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName);

	// Cached to restore the mesh transform after ragdoll without assuming default values.
	FTransform MeshRelativeTransformCache{FTransform::Identity};
	FTransform BasedMovementDelta{FTransform::Identity};
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	FRotator LastControlRotation{FRotator::ZeroRotator};

	UFUNCTION(BlueprintPure, Category = "Traversal")
	FTraversalCheckInputs GetTraversalCheckInputs() const;

	UFUNCTION(BlueprintPure, Category = "Input")
	bool HasFullMovementInput() const;

	void GetMovementDirectionAndOffset(EMovementDirection& MovementDirection, float& RotationOffset);

	UPROPERTY(BlueprintReadOnly)
	float ControlRotationRate{0.f};
	UPROPERTY(BlueprintReadOnly)
	uint8 TwinStickMode : 1{false};
	UPROPERTY(BlueprintReadOnly)
	FRotator TwinStickAimRotation{FRotator::ZeroRotator};

	virtual void RefreshFloorValues();
	virtual void RefreshControlRotationRate(const float DeltaTime);
	virtual void RefreshTwinStickMode();
	virtual void RefreshMoverState();

	// -- Begin CruciblePawn implementation
	virtual void HandleDeathStarted() override;
	// -- End CruciblePawn implementation

	virtual FGameplayTagContainer BP_GetOwnedGameplayTags() const override;

public:
	FTransform GetMeshRelativeTransformCache() const { return MeshRelativeTransformCache; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input",
		meta = (BaseStruct = "/Script/GASP.GASPInputState"))
	FInstancedStruct PlayerInputState{FGASPInputState::StaticStruct()};

	UPROPERTY(BlueprintReadOnly, Category=Character)
	uint8 bJustPressedJump : 1;

	UFUNCTION(BlueprintPure)
	const FGASPMoverInputs& GetMoverState() const;

	UFUNCTION(BlueprintPure)
	virtual FVector GetMovementInputVector();

	UFUNCTION(BlueprintPure)
	virtual FVector GetOrientationIntent();

	UFUNCTION(BlueprintPure)
	virtual FRotator GetAimingRotation();

	UFUNCTION(BlueprintPure)
	virtual FGameplayTag GetAllowedRotationMode();
	UFUNCTION(BlueprintPure)
	virtual FGameplayTag GetAllowedGait();

	/** Returns Mesh subobject **/
	inline USkeletalMeshComponent* GetMesh() const { return Mesh; }

	/** Name of the MeshComponent. Use this name if you want to prevent creation of the component (with ObjectInitializer.DoNotCreateDefaultSubobject). */
	static FName MeshComponentName;

	inline UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }
	static FName CapsuleComponentName;

	UFUNCTION(BlueprintPure, Category = Mover)
	UGASPMoverComponent* GetMoverComponent() const { return CharacterMotionComponent; }

	UFUNCTION(BlueprintPure, Category = Mover)
	UNavMoverComponent* GetNavMoverComponent() const { return NavMoverComponent; }

	UFUNCTION(BlueprintPure, Category = Warping)
	UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }

	UFUNCTION(BlueprintPure, Category = Physics)
	UGASPPhysicsControlComponent* GetPhysicsControlComponent() const { return PhysicsControlComponent; }

	static FName MotionWarpingComponentName;
	static FName CharacterMotionComponentName;
	static FName NavMoverComponentName;
	static FName PhysicsControlComponentName;
	/** Stable subobject name used to select an Overlay Component subclass during construction. */
	static FName OverlayComponentName;

	/**
	 * Provides the component that combines overlay contributions and resolves their layers.
	 */
	UFUNCTION(BlueprintPure, Category = "GASP|Overlay")
	UGASPOverlayComponent* GetOverlayComponent() const;

	/**
	 * Provides authored and replicated manual overlays, excluding local source contributions.
	 */
	FGameplayTagContainer GetBaseOverlayMode() const;

	//~ Begin INavAgentInterface Interface
	virtual FVector GetNavAgentLocation() const override;
	//~ End INavAgentInterface Interface

	virtual void UpdateNavigationRelevance() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D TwinStickAimDirection{FVector2D::ZeroVector};

	UFUNCTION(BlueprintPure)
	UGASPCharacterSettings* GetSettings() const
	{
		return Settings;
	}

	/**
	 * Settings accessor that is safe to dereference. Settings is an unset EditAnywhere property
	 * by default, and PostInitializeComponents runs before the BeginPlay ensure that guards it,
	 * so every internal read goes through here and falls back to the class defaults.
	 */
	const UGASPCharacterSettings* GetSettingsChecked() const;

	UFUNCTION(BlueprintPure)
	FTransform GetBasedMovementDelta() const { return BasedMovementDelta; };

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;

protected:
	UPROPERTY(Category = Movement, VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGASPMoverComponent> CharacterMotionComponent;

	/** Holds functionality for nav movement data and functions */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Nav Movement")
	TObjectPtr<UNavMoverComponent> NavMoverComponent;

public:
	UPROPERTY(BlueprintReadOnly)
	FGASPMoverInputs MoverInputs_PreSim{};

	UFUNCTION(BlueprintCallable, Category="Traversal")
	FTraversalResult TryTraversalAction() const;
	UFUNCTION(BlueprintPure, Category="Traversal")
	bool IsDoingTraversal() const;

	UPROPERTY(BlueprintAssignable)
	FOnGameplayTagContainerChanged OverlayContainerChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged PoseModeChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged StanceModeChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged LocomotionActionChanged;

	UFUNCTION()
	virtual void OnOverlayModeChanged(const FGameplayTagContainer OldOverlayMode,
	                                  const FGameplayTagContainer NewOverlayMode);
	UFUNCTION()
	virtual void OnPoseModeChanged(const FGameplayTag OldPoseMode, const FGameplayTag NewPoseMode);
	UFUNCTION()
	virtual void OnMovementStateChanged(const FGameplayTag OldMovementState, const FGameplayTag NewMovementState);

	/**
	 * Resolves an animation layer with this character as the Chooser context.
	 *
	 * @param DataTable	Chooser to evaluate; null tables produce no layer.
	 * @return			Layer selected from a linked animation set, or null when no set is selected.
	 */
	virtual TSubclassOf<UAnimInstance> GetLinkedAnimLayer(const UChooserTable* DataTable) const;

	explicit AGASPCharacter(const FObjectInitializer& ObjectInitializer);
	AGASPCharacter() = default;

	virtual void Tick(float DeltaTime) override;

	/****************************
	 *		Movement States		*
	 ****************************/

	UFUNCTION(BlueprintCallable)
	void SetOverlayMode(const FGameplayTagContainer NewOverlayMode);
	UFUNCTION(Server, Reliable)
	void Server_SetOverlayMode(const FGameplayTagContainer NewOverlayMode);

	UFUNCTION(BlueprintCallable)
	void SetPoseMode(const FGameplayTag NewPoseMode, const bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetPoseMode(const FGameplayTag NewPoseMode);

	UFUNCTION(BlueprintCallable)
	void SetLocomotionAction(const FGameplayTag NewLocomotionAction, const bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetLocomotionAction(const FGameplayTag NewLocomotionAction);

	UFUNCTION(BlueprintPure)
	virtual bool CanSprint();

	UFUNCTION(BlueprintCallable, Category=Character)
	virtual void Jump();

	UFUNCTION(BlueprintCallable, Category=Character)
	virtual void StopJumping();

	/**
	 * Provides the complete manual and contributed overlay tags used by animation Choosers.
	 */
	UFUNCTION(BlueprintPure)
	FGameplayTagContainer GetOverlayMode() const;

	UFUNCTION(BlueprintPure)
	FGameplayTag GetPoseMode() const
	{
		return PoseMode;
	}

	UFUNCTION(BlueprintPure)
	FGameplayTag GetLocomotionAction() const
	{
		return LocomotionAction;
	}

	UFUNCTION(BlueprintPure)
	FGameplayTag GetMovementMode() const;

	UFUNCTION(BlueprintPure)
	FGameplayTag GetStanceMode() const;

	UFUNCTION(BlueprintPure)
	UGASPTraversalComponent* GetTraversalComponent() const
	{
		return TraversalComponent;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated)
	FGameplayTagContainer GameplayTags;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_TaskStates)
	FInstancedStructCollection TaskStates{};

protected:

	/**
	 * Forwards effective overlay changes to character listeners after component reconciliation.
	 *
	 * @param OldTags	Previously effective overlay tags.
	 * @param NewTags	Current combined manual and contributed tags.
	 */
	UFUNCTION()
	virtual void HandleOverlayComponentChanged(FGameplayTagContainer OldTags, FGameplayTagContainer NewTags);

private:

	UFUNCTION()
	virtual void OnRep_OverlayMode(const FGameplayTagContainer& OldOverlayMode);
	UFUNCTION()
	virtual void OnRep_PoseMode(const FGameplayTag& OldPoseMode);
	UFUNCTION()
	virtual void OnRep_LocomotionAction(const FGameplayTag& OldLocomotionAction);
	UFUNCTION()
	virtual void OnRep_TaskStates(const FInstancedStructCollection& OldTaskStates);

	// Ragdoll
public:
	UFUNCTION(BlueprintPure, Category = "GASP|Ragdoll")
	bool IsRagdolling() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Replicated)
	TObjectPtr<UCharacterTask_Ragdoll> RagdollTask;

	UFUNCTION(BlueprintCallable, Category = "GASP|Character", meta = (AutoCreateRefTerm = "InjuryState"))
	void StartRagdolling(const bool bStopActiveMontages, const FMontageBlendSettings& BlendSettings,
	                     const FGameplayTag& InjuryState);

	UFUNCTION(BlueprintCallable, Category = "GASP|Character")
	bool StopRagdolling();

	UFUNCTION(BlueprintImplementableEvent)
	void OnStartRagdolling();
	UFUNCTION(BlueprintImplementableEvent)
	void OnStopRagdolling();

private:
	UFUNCTION(Server, Reliable)
	void ServerStartRagdolling(const bool bStopActiveMontages, const FMontageBlendSettings& BlendSettings,
	                           const FGameplayTag& InjuryState);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartRagdolling(const bool bStopActiveMontages, const FMontageBlendSettings& BlendSettings,
	                              const FGameplayTag& InjuryState);

	void StartRagdollingImplementation(const bool bStopActiveMontages, const FMontageBlendSettings& BlendSettings,
	                                   const FGameplayTag& InjuryState);

	UFUNCTION(Server, Reliable)
	void ServerStopRagdolling();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopRagdolling();

	void StopRagdollingImplementation();
	void TryPlayGetUpMontage();

	// Gameplay tasks
public:
	/** Finds tasks component for given GameplayTask, Task.GetGameplayTasksComponent() may not be initialized at this point! */
	virtual UGameplayTasksComponent* GetGameplayTasksComponent(const UGameplayTask& Task) const override;

	/** Get owner of a task or default one when task is null */
	virtual AActor* GetGameplayTaskOwner(const UGameplayTask* Task) const override;

	/** Get "body" of task's owner / default, having location in world (e.g. Owner = AIController, Avatar = Pawn) */
	virtual AActor* GetGameplayTaskAvatar(const UGameplayTask* Task) const override
	{
		return GetGameplayTaskOwner(Task);
	}

	/** Get default priority for running a task */
	virtual uint8 GetGameplayTaskDefaultPriority() const override { return FGameplayTasks::DefaultPriority; }

	/** Notify called after GameplayTask finishes initialization (not active yet) */
	virtual void OnGameplayTaskInitialized(UGameplayTask& Task) override;

	/** Notify called after GameplayTask changes state to Active (initial activation or resuming) */
	virtual void OnGameplayTaskActivated(UGameplayTask& Task) override;

	/** Notify called after GameplayTask changes state from Active (finishing or pausing) */
	virtual void OnGameplayTaskDeactivated(UGameplayTask& Task) override;
};
