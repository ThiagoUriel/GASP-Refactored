// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "Actors/GASPCharacter.h"
#include "GASPCharacterExample.generated.h"

class UGameplayCameraComponent;
class UGASPActionComponent;

/**
 * Supplies the sample camera and input actions for GASP characters.
 */
UCLASS()
class GASPEXTRAS_API AGASPCharacterExample : public AGASPCharacter
{

	GENERATED_BODY()

public:

	static const FName ActionComponentName;

	AGASPCharacterExample(const FObjectInitializer& ObjectInitializer);

	// -- Begin Actor implementation
	virtual void PostInitializeComponents() override;
	// -- End Actor implementation

	// -- Begin Pawn implementation
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	// -- End Pawn implementation

	/**
	 * Provides the component that prepares and plays synchronized Mover actions.
	 */
	UFUNCTION(BlueprintPure, Category = "GASP|Actions")
	UGASPActionComponent* GetActionComponent() const { return ActionComponent; }

	/**
	 * Changes the desired gait while sprint input is held.
	 *
	 * @param bPressed	Whether the input is pressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Input|Actions")
	virtual void SprintAction(bool bPressed);

	/**
	 * Toggles between walking and running when sprinting is inactive.
	 *
	 * @param bPressed	Whether the input is pressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Input|Actions")
	virtual void WalkAction(bool bPressed);

	/**
	 * Toggles stance while grounded or sliding.
	 *
	 * @param bPressed	Whether the input is pressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Input|Actions")
	virtual void CrouchAction(bool bPressed);

	/**
	 * Selects aiming rotation while aim input is held.
	 *
	 * @param bPressed	Whether the input is pressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Input|Actions")
	virtual void AimAction(bool bPressed);

	/**
	 * Toggles the sample ragdoll action.
	 *
	 * @param bPressed	Whether the input is pressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Input|Actions")
	virtual void RagdollAction(bool bPressed);

	/**
	 * Toggles strafing rotation.
	 *
	 * @param bPressed	Whether the input is pressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Input|Actions")
	virtual void StrafeAction(bool bPressed);

private:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGameplayCameraComponent> GameplayCamera;

	/**
	 * Keeps the existing subobject name so sample Blueprint references remain valid.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGASPActionComponent> ActionComponent;

};
