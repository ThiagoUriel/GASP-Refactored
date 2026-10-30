// Ninja Bear Studio Inc., all rights reserved.
#include "CoreMinimal.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Common/Fixtures/FoundryCoreTickerFixture.h"
#include "Common/Fixtures/FoundryGameInstanceFixture.h"
#include "Data/FoundryInputSetupDataAsset.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GASPFoundryPawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "MovementSet/GASPMoverComponent.h"
#include "Subsystems/FoundryInputManagerSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FGASPInputMigrationSpec, "NinjaBearStudio.GASP.InputMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FoundryTestFixtures::FGameInstanceFixture> Fixture;
	FoundryTestFixtures::FCoreTickerFixture Tickers;
	TStrongObjectPtr<UGameViewportClient> Viewport;
	TStrongObjectPtr<ULocalPlayer> LocalPlayer;
	TStrongObjectPtr<UInputMappingContext> IndependentContext;
	AGASPFoundryPawn* Pawn = nullptr;
	APlayerController* Controller = nullptr;
	UEnhancedInputComponent* Input = nullptr;
	UEnhancedPlayerInput* PlayerInput = nullptr;
	UEnhancedInputLocalPlayerSubsystem* EnhancedInput = nullptr;
	UInputMappingContext* MainContext = nullptr;
	UInputMappingContext* UIContext = nullptr;
	UInputAction* Move = nullptr;
	UInputAction* Look = nullptr;
	UInputAction* Jump = nullptr;
	bool bPlayerAdded = false;
	bool bReady = false;

	/** Creates the saved controller/Pawn family with a real Local Player and registered input subsystems. */
	bool CreateScenario(bool bUseGameAssets);

	/** Releases Local Player subsystems before destroying the transient World and viewport. */
	void ReleaseScenario();

	/** Counts physical Enhanced Input delegates for one action/event to detect duplicate legacy bindings. */
	int32 CountBindings(const UInputAction* Action, ETriggerEvent Event) const;
END_DEFINE_SPEC(FGASPInputMigrationSpec)

bool FGASPInputMigrationSpec::CreateScenario(const bool bUseGameAssets)
{
	Fixture = MakeUnique<FoundryTestFixtures::FGameInstanceFixture>();
	if (!TestTrue(TEXT("Migration scenario has a Game Instance"), Fixture->HasGameInstance()))
	{
		return false;
	}
	UGameInstance* GameInstance = Fixture->GetGameInstance();
	FWorldContext* WorldContext = GameInstance->GetWorldContext();
	if (!TestNotNull(TEXT("Game Instance has its World context"), WorldContext))
	{
		return false;
	}
	Viewport.Reset(NewObject<UGameViewportClient>(GEngine, GEngine->GameViewportClientClass));
	WorldContext->OwningGameInstance = GameInstance;
	WorldContext->GameViewport = Viewport.Get();
	Viewport->Init(*WorldContext, GameInstance, false);

	const TCHAR* ControllerPath = bUseGameAssets ? TEXT("/Game/Blueprints/Player/PC_Player.PC_Player_C") : TEXT("/GASP/GASPExtras/Blueprints/BP_GASP_PlayerController.BP_GASP_PlayerController_C");
	const TCHAR* PawnPath = bUseGameAssets ? TEXT("/Game/Blueprints/Characters/CPB_Player.CPB_Player_C") : TEXT("/GASP/GASPExtras/Blueprints/CBP_SandboxCharacter.CBP_SandboxCharacter_C");
	UClass* ControllerClass = LoadClass<APlayerController>(nullptr, ControllerPath);
	UClass* PawnClass = LoadClass<AGASPFoundryPawn>(nullptr, PawnPath);
	if (!TestNotNull(TEXT("Saved controller class loads"), ControllerClass) || !TestNotNull(TEXT("Saved Pawn class loads"), PawnClass))
	{
		return false;
	}
	Controller = Fixture->GetWorld()->SpawnActor<APlayerController>(ControllerClass);
	Pawn = Fixture->GetWorld()->SpawnActor<AGASPFoundryPawn>(PawnClass);
	if (!TestNotNull(TEXT("Saved controller spawns"), Controller) || !TestNotNull(TEXT("Saved Pawn spawns"), Pawn))
	{
		return false;
	}
	Pawn->SetActorTickEnabled(false);
	Pawn->GetMoverComponent()->SetComponentTickEnabled(false);
	LocalPlayer.Reset(NewObject<ULocalPlayer>(GEngine));
	Controller->SetPlayer(LocalPlayer.Get());
	LocalPlayer->PlayerAdded(Viewport.Get(), 0);
	bPlayerAdded = true;
	LocalPlayer->ForEachSubsystem<ULocalPlayerSubsystem>([this](ULocalPlayerSubsystem* Subsystem)
	{
		Subsystem->PlayerControllerChanged(Controller);
	});

	Input = Cast<UEnhancedInputComponent>(Controller->InputComponent);
	PlayerInput = Cast<UEnhancedPlayerInput>(Controller->PlayerInput);
	EnhancedInput = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	UFoundryInputManagerSubsystem* Manager = LocalPlayer->GetSubsystem<UFoundryInputManagerSubsystem>();
	if (!TestNotNull(TEXT("Controller uses Enhanced Input"), Input) || !TestNotNull(TEXT("Controller has Enhanced Player Input"), PlayerInput)
		|| !TestNotNull(TEXT("Local Player owns Enhanced Input"), EnhancedInput) || !TestNotNull(TEXT("Local Player owns Foundry Input"), Manager))
	{
		return false;
	}

	Move = LoadObject<UInputAction>(nullptr, TEXT("/GASP/GASPExtras/Input/Actions/IA_Move.IA_Move"));
	Look = LoadObject<UInputAction>(nullptr, TEXT("/GASP/GASPExtras/Input/Actions/IA_Look.IA_Look"));
	Jump = LoadObject<UInputAction>(nullptr, TEXT("/GASP/GASPExtras/Input/Actions/IA_Jump.IA_Jump"));
	MainContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/GASP/GASPExtras/Input/IMC_Default.IMC_Default"));
	UIContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/GASP/GASPExtras/Input/IMC_UI.IMC_UI"));
	if (!TestNotNull(TEXT("Move action loads"), Move) || !TestNotNull(TEXT("Look action loads"), Look) || !TestNotNull(TEXT("Jump action loads"), Jump)
		|| !TestNotNull(TEXT("Main context loads"), MainContext) || !TestNotNull(TEXT("UI context loads"), UIContext))
	{
		return false;
	}
	IndependentContext.Reset(NewObject<UInputMappingContext>());
	EnhancedInput->AddMappingContext(IndependentContext.Get(), 25);
	Controller->Possess(Pawn);
	FModifyContextOptions Options;
	Options.bForceImmediately = true;
	EnhancedInput->RequestRebuildControlMappings(Options);
	return TestTrue(TEXT("Saved Pawn is locally controlled"), Pawn->IsLocallyControlled());
}

void FGASPInputMigrationSpec::ReleaseScenario()
{
	Tickers.Reset();
	if (LocalPlayer.IsValid() && bPlayerAdded)
	{
		LocalPlayer->PlayerRemoved();
	}
	bPlayerAdded = false;
	if (Viewport.IsValid())
	{
		Viewport->DetachViewportClient();
	}
	if (Fixture.IsValid() && Fixture->HasGameInstance())
	{
		Fixture->GetGameInstance()->GetWorldContext()->GameViewport = nullptr;
	}
	Fixture.Reset();
	LocalPlayer.Reset();
	Viewport.Reset();
	IndependentContext.Reset();
	Pawn = nullptr;
	Controller = nullptr;
	Input = nullptr;
	PlayerInput = nullptr;
	EnhancedInput = nullptr;
	bReady = false;
}

int32 FGASPInputMigrationSpec::CountBindings(const UInputAction* Action, const ETriggerEvent Event) const
{
	int32 Count = 0;
	for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : Input->GetActionEventBindings())
	{
		if (Binding->GetAction() == Action && Binding->GetTriggerEvent() == Event)
		{
			++Count;
		}
	}
	return Count;
}

void FGASPInputMigrationSpec::Define()
{
	for (const bool bUseGameAssets : {false, true})
	{
		Describe(bUseGameAssets ? "Game assets" : "Sample assets", [this, bUseGameAssets]()
		{
			BeforeEach([this, bUseGameAssets]()
			{
				bReady = CreateScenario(bUseGameAssets);
			});
			AfterEach([this]()
			{
				ReleaseScenario();
			});
			LatentIt("Should route saved locomotion bindings once and preserve context ownership through possession", [this](const FDoneDelegate& Done)
			{
				if (!bReady)
				{
					Done.Execute();
					return;
				}
				Tickers.AddTicker(TEXT("GASPInput.MigratedBindings"), 0.f, [this, Done, Phase = 0, LastFrame = GFrameCounter](float) mutable
				{
					if (LastFrame == GFrameCounter)
					{
						return true;
					}
					LastFrame = GFrameCounter;
					TArray<UInputComponent*> Stack;
					Stack.Add(Input);
					switch (Phase++)
					{
					case 0:
						TestTrue(TEXT("Foundry installs the main context"), EnhancedInput->HasMappingContext(MainContext));
						TestTrue(TEXT("The controller retains its UI context"), EnhancedInput->HasMappingContext(UIContext));
						TestTrue(TEXT("Possession preserves independently registered contexts"), EnhancedInput->HasMappingContext(IndependentContext.Get()));
						TestEqual(TEXT("Move has one Triggered route"), CountBindings(Move, ETriggerEvent::Triggered), 1);
						TestEqual(TEXT("Look has one Triggered route"), CountBindings(Look, ETriggerEvent::Triggered), 1);
						TestEqual(TEXT("Jump has one Triggered route"), CountBindings(Jump, ETriggerEvent::Triggered), 1);
						TestEqual(TEXT("Jump has one cancellation route"), CountBindings(Jump, ETriggerEvent::Canceled), 1);
						PlayerInput->ProcessInputStack(Stack, 0.016f, false);
						break;
					case 1:
						PlayerInput->InjectInputForAction(Move, FInputActionValue(FVector2D(0.25f, 0.f)));
						PlayerInput->ProcessInputStack(Stack, 0.016f, false);
						TestTrue(TEXT("Move reaches GASP once with its raw magnitude"), Pawn->GetPendingMovementInputVector().Equals(FVector(0.25f, 0.f, 0.f)));
						Pawn->ConsumeMovementInputVector();
						break;
					case 2:
						Controller->RotationInput = FRotator::ZeroRotator;
						PlayerInput->InjectInputForAction(Look, FInputActionValue(FVector2D(0.5f, 0.25f)));
						PlayerInput->ProcessInputStack(Stack, 0.016f, false);
						TestFalse(TEXT("Look reaches the local controller"), Controller->RotationInput.IsNearlyZero());
						break;
					case 3:
						PlayerInput->InjectInputForAction(Jump, FInputActionValue(true));
						PlayerInput->ProcessInputStack(Stack, 0.016f, false);
						TestTrue(TEXT("Jump reaches the Mover pulse through the saved setup"), Pawn->bJustPressedJump);
						break;
					case 4:
						Controller->UnPossess();
						TestFalse(TEXT("Unpossession removes the Foundry-owned main context"), EnhancedInput->HasMappingContext(MainContext));
						TestTrue(TEXT("Unpossession preserves the UI context"), EnhancedInput->HasMappingContext(UIContext));
						TestTrue(TEXT("Unpossession preserves independent contexts"), EnhancedInput->HasMappingContext(IndependentContext.Get()));
						TestEqual(TEXT("Unpossession removes Move routes"), CountBindings(Move, ETriggerEvent::Triggered), 0);
						TestEqual(TEXT("Unpossession removes Look routes"), CountBindings(Look, ETriggerEvent::Triggered), 0);
						TestEqual(TEXT("Unpossession removes Jump routes"), CountBindings(Jump, ETriggerEvent::Triggered), 0);
						TestFalse(TEXT("Unpossession clears the previous Pawn's jump pulse"), Pawn->bJustPressedJump);
						break;
					default:
						Controller->Possess(Pawn);
						TestTrue(TEXT("Repossessing reinstalls the main context"), EnhancedInput->HasMappingContext(MainContext));
						TestTrue(TEXT("Repossessing preserves the independent context"), EnhancedInput->HasMappingContext(IndependentContext.Get()));
						TestEqual(TEXT("Repossessing installs exactly one Move route"), CountBindings(Move, ETriggerEvent::Triggered), 1);
						TestEqual(TEXT("Repossessing installs exactly one Look route"), CountBindings(Look, ETriggerEvent::Triggered), 1);
						TestEqual(TEXT("Repossessing installs exactly one Jump route"), CountBindings(Jump, ETriggerEvent::Triggered), 1);
						Done.Execute();
						return false;
					}
					return true;
				});
			});
		});
	}
}

#endif
