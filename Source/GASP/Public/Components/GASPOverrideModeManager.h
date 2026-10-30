// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GASPOverrideModeManager.generated.h"

class UAnimInstance;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateChanged, FGameplayTag, OldGameplayTag, FGameplayTag, NewGameplayTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameplayTagContainerChanged, FGameplayTagContainer, OldGameplayTagContainer, FGameplayTagContainer, NewGameplayTagContainer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOverrideLayerChanged, FGameplayTag, OverrideTag, bool, bIsActive);

/**
 * Applies tag-owned animation layer requests while sharing ownership of repeated classes.
 *
 * Requests survive missing or reinitialized Animation Instances. Classes are applied once in
 * deterministic tag order, with an optional priority tag last. An unchanged stack is left linked.
 */
UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GASP_API UGASPOverrideModeManager : public UActorComponent
{

	GENERATED_BODY()

public:

	/** Notifies listeners when a tag starts or stops requesting an override layer. */
	UPROPERTY(BlueprintAssignable, Category = "GASP|Override")
	FOnOverrideLayerChanged OnOverrideLayerChanged;

	UGASPOverrideModeManager();

	/**
	 * Registers or replaces one tag's layer request; invalid tags or classes are ignored.
	 *
	 * @param OverrideTag		Tag owning this request.
	 * @param OverrideAnimClass	Layer requested by the tag; shared classes remain linked once.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Override")
	void AddOverrideLayer(FGameplayTag OverrideTag, TSubclassOf<UAnimInstance> OverrideAnimClass);

	/**
	 * Releases a tag's request while preserving classes still requested by other tags.
	 *
	 * @param OverrideTag	Tag whose request should be removed; missing tags are ignored.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Override")
	void RemoveOverrideLayer(FGameplayTag OverrideTag);

	/**
	 * Changes the target mesh and releases layers on its previous Animation Instance.
	 *
	 * @param Mesh	New mesh, or null to detach; desired requests remain available for a later target.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Override")
	void SetTargetMesh(USkeletalMeshComponent* Mesh);

protected:

	/** Desired tag-owned layer requests, including requests waiting for an Animation Instance. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TMap<FGameplayTag, TSubclassOf<UAnimInstance>> LinkedOverrideLayers;

	// -- Begin ActorComponent implementation
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// -- End ActorComponent implementation

	/**
	 * Replaces all desired requests and reconciles their complete class stack.
	 *
	 * @param Layers		Complete tag-to-layer snapshot; invalid entries are ignored.
	 * @param PriorityTag	Tag whose class takes precedence by being applied last.
	 */
	void SetOverrideLayers(const TMap<FGameplayTag, TSubclassOf<UAnimInstance>>& Layers, FGameplayTag PriorityTag);

	/**
	 * Reconciles desired layers, deferring nested requests until the current update completes.
	 *
	 * @param bForce	Reapplies an unchanged stack after Animation Instance reinitialization.
	 */
	void ReconcileLayers(bool bForce = false);

	/**
	 * Builds a unique class stack in application order; subclasses may customize precedence.
	 */
	virtual TArray<TSubclassOf<UAnimInstance>> BuildDesiredLayerStack() const;

	/**
	 * Applies one stack update, releasing previous instance ownership when the target changes.
	 *
	 * @param DesiredLayers	Unique layer classes in application order.
	 * @param bForce		Reapplies an unchanged stack when true.
	 */
	void ApplyLayerStack(const TArray<TSubclassOf<UAnimInstance>>& DesiredLayers, bool bForce);

	/**
	 * Releases applied classes without discarding desired requests.
	 */
	void ReleaseLayers();

	/**
	 * Provides the current Animation Instance, or null while the target is unavailable.
	 */
	virtual UAnimInstance* ResolveAnimationInstance() const;

	/**
	 * Links a layer during reconciliation; the supplied Animation Instance is valid.
	 *
	 * @param Instance	Instance receiving the layer.
	 * @param Layer		Class to link at this position in the stack.
	 */
	virtual void LinkLayer(UAnimInstance* Instance, TSubclassOf<UAnimInstance> Layer);

	/**
	 * Unlinks a class whose applied ownership has ended.
	 *
	 * @param Instance	Valid instance from which the layer is released.
	 * @param Layer		Previously applied class to unlink.
	 */
	virtual void UnlinkLayer(UAnimInstance* Instance, TSubclassOf<UAnimInstance> Layer);

private:

	TWeakObjectPtr<USkeletalMeshComponent> TargetMesh;
	TWeakObjectPtr<UAnimInstance> AppliedInstance;
	FGameplayTag LastPriorityTag;
	bool bReconciling = false;
	bool bReconcilePending = false;
	bool bForcePending = false;

	UPROPERTY(Transient)
	TArray<TSubclassOf<UAnimInstance>> AppliedLayers;

	/** Reapplies pending requests after the target initializes its Animation Instance. */
	UFUNCTION()
	void HandleAnimInitialized();

};
