#pragma once

#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "Types/EnumTypes.h"
#include "Types/TagTypes.h"
#include "Types/StructTypes.h"
#include "GASPCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStateChanged, FGameplayTag, OldGameplayTag);

UCLASS()
class GASP_API AGASPCharacter : public ACharacter
{
	GENERATED_BODY()

	UFUNCTION(BlueprintSetter)
	void SetMovementMode(const FGameplayTag NewMovementMode, bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetMovementMode(const FGameplayTag NewMovementMode);

protected:
	UPROPERTY(EditAnywhere, Category="PoseSearchData|Choosers", BlueprintReadOnly)
	TObjectPtr<class UChooserTable> OverlayTable{nullptr};
	UPROPERTY(EditAnywhere, Category="PoseSearchData|Choosers", BlueprintReadOnly)
	TObjectPtr<UChooserTable> PosesTable{nullptr};

	UPROPERTY(BlueprintReadOnly, Transient)
	TObjectPtr<class UGASPCharacterMovementComponent> MovementComponent{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UMotionWarpingComponent> MotionWarpingComponent{};
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", Replicated)
	TObjectPtr<class UGASPTraversalComponent> TraversalComponent{};

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Transient)
	FGameplayTag DesiredGait{GaitTags::Run};

	UPROPERTY(BlueprintReadOnly, Transient)
	FGameplayTag Gait{GaitTags::Run};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_RotationMode, Transient)
	FGameplayTag RotationMode{RotationTags::Strafe};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_MovementState, Transient)
	FGameplayTag MovementState{MovementStateTags::Idle};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_MovementMode, Transient)
	FGameplayTag MovementMode{MovementModeTags::Grounded};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_StanceMode, Transient)
	FGameplayTag StanceMode{StanceTags::Standing};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_OverlayMode, Transient)
	FGameplayTag OverlayMode{OverlayModeTags::Default};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PoseMode, Transient)
	FGameplayTag PoseMode{PoseModeTags::Default};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_LocomotionAction, Transient)
	FGameplayTag LocomotionAction{FGameplayTag::EmptyTag};

	UPROPERTY(BlueprintReadOnly, Transient)
	FGameplayTag PreviousMovementMode{MovementModeTags::Grounded};

	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	FVector_NetQuantize ReplicatedAcceleration{ForceInit};
	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	FVector_NetQuantize RagdollTargetLocation{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Character", Transient)
	FRagdollingState RagdollingState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State|Character")
	TObjectPtr<UAnimMontage> GetUpMontageFront{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State|Character")
	TObjectPtr<UAnimMontage> GetUpMontageBack{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State|Character")
	uint8 bLimitInitialRagdollSpeed : 1{false};

	void SetReplicatedAcceleration(const FVector& NewAcceleration);

	UFUNCTION(BlueprintPure)
	UAnimMontage* SelectGetUpMontage(bool bRagdollFacingUpward);

	virtual void OnWalkingOffLedge_Implementation(const FVector& PreviousFloorImpactNormal,
	                                              const FVector& PreviousFloorContactNormal,
	                                              const FVector& PreviousLocation, float TimeDelta) override;

	/** Please add a function description */
	UFUNCTION(BlueprintPure, Category = "Traversal")
	struct FTraversalCheckInputs GetTraversalCheckInputs() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta=(ClampMin="0.0", ClampMax="1.0"))
	float AnalogMovementThreshold{.7f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	EAnalogStickBehaviorMode MovementStickMode{EAnalogStickBehaviorMode::FixedSingleGait};

	UFUNCTION(BlueprintPure, Category = "Input")
	bool HasFullMovementInput() const;

	UFUNCTION(BlueprintPure, Category = "Input")
	FVector2D GetMovementInputScaleValue(const FVector2D InVector) const;

public:
	
	static FName MotionWarpingComponentName;
	
	void RefreshGait();

	UFUNCTION(BlueprintCallable, Category="Traversal")
	FTraversalResult TryTraversalAction() const;
	UFUNCTION(BlueprintPure, Category="Traversal")
	bool IsDoingTraversal() const;

	UPROPERTY(BlueprintAssignable)
	FOnStateChanged OverlayModeChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged PoseModeChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged GaitChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged RotationModeChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged MovementStateChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged StanceModeChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged LocomotionActionChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged MovementModeChanged;

	UFUNCTION(BlueprintNativeEvent)
	void OnOverlayModeChanged(const FGameplayTag OldOverlayMode);
	UFUNCTION(BlueprintNativeEvent)
	void OnPoseModeChanged(const FGameplayTag OldPoseMode);

	void LinkAnimInstance(const UChooserTable* DataTable, const FGameplayTag OldState, const FGameplayTag State);


	// Sets default values for this character's properties
	explicit AGASPCharacter(const FObjectInitializer& ObjectInitializer);
	AGASPCharacter() = default;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void PostRegisterAllComponents() override;

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	/****************************
	 *		Movement States		*
	 ****************************/
	UFUNCTION(BlueprintCallable)
	void SetGait(const FGameplayTag NewGait, bool bForce = false);

	UFUNCTION(BlueprintCallable)
	void SetDesiredGait(const FGameplayTag NewGait, bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetDesiredGait(const FGameplayTag NewGait);

	UFUNCTION(BlueprintCallable)
	void SetRotationMode(const FGameplayTag NewRotationMode, const bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetRotationMode(const FGameplayTag NewRotationMode);

	UFUNCTION(BlueprintCallable)
	void SetMovementState(const FGameplayTag
	                      NewMovementState, const bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetMovementState(const FGameplayTag
		NewMovementState);

	UFUNCTION(BlueprintCallable)
	void SetStanceMode(const FGameplayTag NewStanceMode, const bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetStanceMode(const FGameplayTag NewStanceMode);

	UFUNCTION(BlueprintCallable)
	void SetOverlayMode(const FGameplayTag NewOverlayMode, const bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetOverlayMode(const FGameplayTag NewOverlayMode);

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

	template <typename T>
	T* GetTypedCharacterMovement() const;

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FVector GetReplicatedAcceleration() const
	{
		return ReplicatedAcceleration;
	}

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetOverlayMode() const
	{
		return OverlayMode;
	}

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetLocomotionAction() const
	{
		return LocomotionAction;
	}

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetGait() const
	{
		return Gait;
	}

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetRotationMode() const
	{
		return RotationMode;
	}

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetMovementMode() const
	{
		return MovementMode;
	}

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetMovementState() const
	{
		return MovementState;
	}

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetStanceMode() const
	{
		return StanceMode;
	}

	// Ragdolling
	bool IsRagdollingAllowedToStart() const;

	const FRagdollingState& GetRagdollingState() const
	{
		return RagdollingState;
	}

	UFUNCTION(BlueprintCallable, Category = "GASP|Character")
	void StartRagdolling();

private:
	UFUNCTION(Server, Reliable)
	void ServerStartRagdolling();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartRagdolling();

	void StartRagdollingImplementation();

	UFUNCTION()
	virtual void OnRep_OverlayMode(const FGameplayTag& OldOverlayMode);
	UFUNCTION()
	virtual void OnRep_PoseMode(const FGameplayTag& OldPoseMode);
	UFUNCTION()
	virtual void OnRep_Gait(const FGameplayTag& OldGait);
	UFUNCTION()
	virtual void OnRep_StanceMode(const FGameplayTag& OldStanceMode);
	UFUNCTION()
	virtual void OnRep_MovementMode(const FGameplayTag& OldMovementMode);
	UFUNCTION()
	virtual void OnRep_RotationMode(const FGameplayTag& OldRotationMode);
	UFUNCTION()
	virtual void OnRep_MovementState(const FGameplayTag& OldMovementState);
	UFUNCTION()
	virtual void OnRep_LocomotionAction(const FGameplayTag& OldLocomotionAction);

public:
	bool IsRagdollingAllowedToStop() const;

	UFUNCTION(BlueprintCallable, Category = "GASP|Character", Meta = (ReturnDisplayName = "Success"))
	bool StopRagdolling();

	UFUNCTION(BlueprintImplementableEvent)
	void OnStartRagdolling();
	UFUNCTION(BlueprintImplementableEvent)
	void OnStopRagdolling();

	UPROPERTY(BlueprintReadOnly)
	FGameplayTagContainer StateContainer;

private:
	UFUNCTION(Server, Reliable)
	void ServerStopRagdolling();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopRagdolling();

	void StopRagdollingImplementation();

	void SetRagdollTargetLocation(const FVector& NewTargetLocation);

	UFUNCTION(Server, Unreliable)
	void ServerSetRagdollTargetLocation(const FVector_NetQuantize& NewTargetLocation);

	void RefreshRagdolling(float DeltaTime);

	FVector RagdollTraceGround(bool& bGrounded) const;

	void ConstraintRagdollSpeed() const;
};

template <typename T>
T* AGASPCharacter::GetTypedCharacterMovement() const
{
	return static_cast<T*>(GetCharacterMovement());
}
