// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Actors/GASPCharacter.h"
#include "GASPAbilitiesCharacter.generated.h"

class UAbilitySystemComponent;

/**
 * GASP Character that includes an Ability System Component.
 *
 * The Ability System Component can be modified by subclasses, using a different type, such as the
 * Ninja GAS version, or even fully cancelled, for scenarios where the ASC lives in the Player State.
 * 
 * This character is also a modular character, meaning it will broadcast Modular Framework events
 * when the Begin Play lifecycle event happens. Watching this event is a way to grant default attribute 
 * sets and abilities for a character.
 */
UCLASS(Abstract)
class GASPABILITIES_API AGASPAbilitiesCharacter : public AGASPCharacter, public IAbilitySystemInterface
{
	
	GENERATED_BODY()

public:
	
	/** 
	 * Name for the ASC Component, if initialized by the class. 
	 * 
	 * Can be used by C++ subclasses to override the Ability System Component type,
	 * or even cancel the instantiation of the component in the constructor. 
	 */
	static FName AbilitySystemComponentName;
	
	AGASPAbilitiesCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// -- Begin Character implementation
	virtual void PostInitProperties() override;
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// -- End Character implementation	
	
	// -- Begin Ability System implementation
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// -- End Ability System implementation
	
protected:

	/** 
	 * Allows subclasses to skip the Ability System Component initialization.
	 * 
	 * This is most likely the case on player-oriented subclasses, where the Ability System
	 * Component lives in the Player State, so no initialization should happen here.
	 */
	bool bInitializeAbilityComponentOnBeginPlay;	
	
	/**
	 * Sets how the Ability System component will replicate data to clients.
	 *
	 * - Full:		Single Player, every Gameplay Effect is replicated to every client.
	 * 
	 * - Mixed:		Player Actors in Multiplayer, Gameplay Effects are replicated to owning client.
	 *				Gameplay Tags and Cues will replicate to everyone.
	 *				
	 * - Minimal	AI Actors in Multiplayer, Gameplay Effects are never replicated.
	 *				Gameplay Tags and Cues will replicate to everyone.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability System")
	EGameplayEffectReplicationMode AbilityReplicationMode;	
	
	/**
	 * Initializes the ability system component using the source as an avatar.
	 *
	 * @param AbilitySystemOwner
	 *		Actor that owns the Ability System Component. It may be the same character
	 *		or an external source such as the Player State.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NBS|GASP|Character")
	virtual void SetupAbilitySystemComponent(AActor* AbilitySystemOwner);
	
	/**
	 * Clears the ability system component, most likely due to the character being destroyed.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NBS|GASP|Character")
	virtual void ClearAbilitySystemComponent();

	/**
	 * Timer-friendly function to initialize features from the Player State.
	 * By default, retrieves a copy of the ASC, but can be used for other components too.
	 */
	UFUNCTION()
	virtual void InitializeFromPlayerState();
	
private:

	/** The Ability System Component managed by this character class. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess))
	TObjectPtr<UAbilitySystemComponent> CharacterAbilities;
	
};
