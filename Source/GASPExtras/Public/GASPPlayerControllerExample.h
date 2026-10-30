// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "GameFramework/PlayerController.h"
#include "GASPPlayerControllerExample.generated.h"

class AGASPCharacterExample;
class UInputMappingContext;

UCLASS()
class GASPEXTRAS_API AGASPPlayerControllerExample : public APlayerController
{
	GENERATED_BODY()

public:
	AGASPPlayerControllerExample() = default;

	UFUNCTION(BlueprintPure)
	AGASPCharacterExample* GetPossessedPlayer();
	UFUNCTION(BlueprintPure)
	TSoftObjectPtr<UInputMappingContext> GetDefaultInputMapping();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void OnRep_Pawn() override;
	virtual void OnRep_Owner() override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, BlueprintGetter=GetPossessedPlayer)
	TObjectPtr<AGASPCharacterExample> PossessedPlayer{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintGetter=GetDefaultInputMapping)
	TSoftObjectPtr<UInputMappingContext> DefaultInputMapping{};

	/** Updates only the optional legacy context owned by this Controller. Empty defaults use Pawn-provided setups. */
	void SetupInput();

private:

	/** Context installed by this Controller; externally registered contexts are never claimed or removed. */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> AppliedDefaultInputMapping;
};
