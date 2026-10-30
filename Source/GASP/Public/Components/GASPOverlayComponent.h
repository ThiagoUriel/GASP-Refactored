// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "Components/GASPOverrideModeManager.h"
#include "GASPOverlayComponent.generated.h"

/**
 * Identifies one source contribution, optionally scoped to an Inventory and an Item.
 *
 * Reusing a key replaces its tags. Different runtime identities remain independent even when
 * they share the same source object. The source is weakly referenced and may be const.
 */
struct FGASPOverlaySourceKey
{

	TWeakObjectPtr<const UObject> Source;
	FName Id;
	FGuid Scope;
	FGuid RuntimeId;

	/**
	 * Checks whether both keys identify the same source contribution.
	 */
	bool operator==(const FGASPOverlaySourceKey& Other) const
	{
		return Source == Other.Source && Id == Other.Id && Scope == Other.Scope && RuntimeId == Other.RuntimeId;
	}

	/**
	 * Provides a hash for storing source identities in associative containers.
	 */
	friend uint32 GetTypeHash(const FGASPOverlaySourceKey& Key)
	{
		const uint32 SourceHash = HashCombine(GetTypeHash(Key.Source), GetTypeHash(Key.Id));
		const uint32 RuntimeHash = HashCombine(GetTypeHash(Key.Scope), GetTypeHash(Key.RuntimeId));
		return HashCombine(SourceHash, RuntimeHash);
	}
};

/**
 * Combines manual and source-owned overlay tags and resolves their animation layers through Choosers.
 *
 * Contributions are local presentation state and never send RPCs. GASP characters remain the
 * Chooser context. Subclasses can customize base tags, pose selection, and layer resolution.
 */
UCLASS(Blueprintable, ClassGroup = (GASP), meta = (BlueprintSpawnableComponent))
class GASP_API UGASPOverlayComponent : public UGASPOverrideModeManager
{

	GENERATED_BODY()

public:

	/** Notifies listeners after the complete effective tag state has changed. */
	UPROPERTY(BlueprintAssignable, Category = "GASP|Overlay")
	FOnGameplayTagContainerChanged OnOverlayChanged;

	/**
	 * Registers or replaces a source contribution using a native runtime identity.
	 *
	 * @param Key	Identifies the source and optional runtime scope.
	 * @param Tags	Tags contributed by this source; an empty container removes the contribution.
	 * @return		True when the registered contribution changed.
	 */
	bool SetSourceOverlayContribution(const FGASPOverlaySourceKey& Key, const FGameplayTagContainer& Tags);

	/**
	 * Removes one source contribution, including during source teardown.
	 *
	 * @param Key	Identifies the contribution to release.
	 * @return		True when a registered contribution was removed.
	 */
	bool RemoveSourceOverlayContribution(const FGASPOverlaySourceKey& Key);

	/**
	 * Registers or replaces a local source contribution without changing manual overlays.
	 *
	 * @param Source	Object owning this contribution; null or expired sources are rejected.
	 * @param Id		Distinguishes contributions owned by the same source.
	 * @param Tags		Contributed tags; an empty container releases this source and ID.
	 * @return			True when the registered contribution changed.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Overlay")
	bool SetOverlayContribution(UObject* Source, FName Id, const FGameplayTagContainer& Tags);

	/**
	 * Removes the local contribution identified by a source and ID.
	 *
	 * @param Source	Object that registered the contribution.
	 * @param Id		Contribution ID used during registration.
	 * @return			True when a registered contribution was removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Overlay")
	bool RemoveOverlayContribution(UObject* Source, FName Id);

	/**
	 * Provides the combined manual and contributed overlay tags.
	 */
	UFUNCTION(BlueprintPure, Category = "GASP|Overlay")
	FGameplayTagContainer GetOverlayMode() const;

	/**
	 * Reevaluates Chooser inputs and removes expired sources, or queues a refresh during a batch.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Overlay")
	void RefreshOverlay();

	/**
	 * Begins a nestable batch whose changes are resolved when the final matching update ends.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Overlay")
	void BeginOverlayUpdate();

	/**
	 * Ends a matching batch and resolves its complete contribution state once.
	 */
	UFUNCTION(BlueprintCallable, Category = "GASP|Overlay")
	void EndOverlayUpdate();

protected:

	// -- Begin ActorComponent implementation
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// -- End ActorComponent implementation

	/**
	 * Resolves the manual tags to combine with source contributions before Chooser evaluation.
	 */
	virtual FGameplayTagContainer ResolveBaseOverlayMode() const;

	/**
	 * Resolves the pose tag that takes precedence over overlay layers.
	 */
	virtual FGameplayTag ResolvePoseMode() const;

	/**
	 * Resolves the overlay layer using the complete published tag state.
	 */
	virtual TSubclassOf<UAnimInstance> ResolveOverlayLayer() const;

	/**
	 * Resolves the pose layer using the owner's current Chooser context.
	 */
	virtual TSubclassOf<UAnimInstance> ResolvePoseLayer() const;

	/**
	 * Refreshes the target mesh while preserving pending contributions.
	 */
	virtual void RefreshTargetMesh();

	/**
	 * Rebuilds effective tags and removes source identities that have expired.
	 */
	void RefreshEffectiveOverlayTags();

	/**
	 * Builds the desired layer requests after the effective tags have been published.
	 *
	 * @param PoseMode	Pose tag whose layer is applied last.
	 * @return			Complete overlay and pose requests for reconciliation.
	 */
	virtual TMap<FGameplayTag, TSubclassOf<UAnimInstance>> BuildDesiredOverlayLayers(FGameplayTag PoseMode) const;

	/**
	 * Applies one refresh and notifies listeners after layers and effective tags have been reconciled.
	 */
	void RefreshOverlayState();

private:

	TMap<FGASPOverlaySourceKey, FGameplayTagContainer> Contributions;
	int32 UpdateDepth = 0;
	bool bRefreshing = false;
	bool bRefreshPending = false;
	bool bEndingPlay = false;

	UPROPERTY(Transient)
	FGameplayTagContainer EffectiveOverlayTags;

};
