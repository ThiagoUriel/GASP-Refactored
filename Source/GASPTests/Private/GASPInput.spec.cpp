// Ninja Bear Studio Inc., all rights reserved.
#include "GASPInputTestTypes.h"

#include "FoundryInputTags.h"
#include "GASPOverlayTestTypes.h"
#include "TimerManager.h"
#include "Common/Fixtures/FoundryCoreTickerFixture.h"
#include "Common/Fixtures/FoundryGameInstanceFixture.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/CameraAsset.h"
#include "Core/CameraRigAsset.h"
#include "Directors/SingleCameraDirector.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Input/Handlers/InputHandler_GASPJump.h"
#include "GameFramework/Input/Handlers/InputHandler_GASPLook.h"
#include "GameFramework/Input/Handlers/InputHandler_GASPMove.h"
#include "Misc/AutomationTest.h"
#include "MovementSet/GASPMoverComponent.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FGASPInputSpec, "NinjaBearStudio.GASP.Input",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FoundryTestFixtures::FGameInstanceFixture> Fixture;
	FoundryTestFixtures::FCoreTickerFixture Tickers;
	TStrongObjectPtr<ULocalPlayer> LocalPlayer;
	AGASPInputTestPawn* Pawn = nullptr;
	APlayerController* Controller = nullptr;
	FFoundryInputHandlerContext Context;
	bool bReady = false;

	/** Advances the Pawn's real timer across engine frames until the jump pulse resets. */
	void WaitForJumpReset(FSimpleDelegate Done);
END_DEFINE_SPEC(FGASPInputSpec)

void FGASPInputSpec::WaitForJumpReset(FSimpleDelegate Done)
{
	Tickers.AddTicker(TEXT("GASPInput.JumpReset"), 0.f, [this, Done, LastFrame = GFrameCounter, Steps = 0](float) mutable
	{
		if (LastFrame == GFrameCounter)
		{
			return true;
		}
		LastFrame = GFrameCounter;
		Fixture->GetWorld()->GetTimerManager().Tick(0.05f);
		++Steps;
		if (!Pawn->bJustPressedJump || Steps >= 10)
		{
			TestFalse(TEXT("The Pawn-owned timer releases the short jump pulse"), Pawn->bJustPressedJump);
			Done.Execute();
			return false;
		}
		return true;
	});
}

void FGASPInputSpec::Define()
{
	BeforeEach([this]()
	{
		bReady = false;
		Fixture = MakeUnique<FoundryTestFixtures::FGameInstanceFixture>();
		if (!TestTrue(TEXT("Input scenario has an initialized Game Instance"), Fixture->HasGameInstance()))
		{
			return;
		}
		const FTransform Transform = FTransform::Identity;
		Pawn = Fixture->GetWorld()->SpawnActorDeferred<AGASPInputTestPawn>(AGASPInputTestPawn::StaticClass(), Transform);
		if (!TestNotNull(TEXT("Input Pawn spawned"), Pawn))
		{
			return;
		}
		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/GASP/GASP/Characters/UE5_Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
		if (!TestNotNull(TEXT("Sample mesh is available"), Mesh))
		{
			return;
		}
		Pawn->GetMesh()->SetSkeletalMeshAsset(Mesh);
		Pawn->GetMesh()->SetAnimInstanceClass(UGASPTestLayerA::StaticClass());
		UGameplayCameraComponent* Camera = CastChecked<UGameplayCameraComponent>(Pawn->GetDefaultSubobjectByName(TEXT("GameplayCamera")));
		UCameraRigAsset* CameraRig = NewObject<UCameraRigAsset>(Camera);
		USingleCameraDirector* Director = NewObject<USingleCameraDirector>(Camera);
		Director->CameraRig = CameraRig;
		UCameraAsset* CameraAsset = NewObject<UCameraAsset>(Camera);
		CameraAsset->SetCameraDirector(Director);
		Camera->CameraReference.SetCameraAsset(CameraAsset);
		Pawn->FinishSpawning(Transform);
		Pawn->GetMoverComponent()->SetComponentTickEnabled(false);

		Controller = Fixture->GetWorld()->SpawnActor<APlayerController>();
		if (!TestNotNull(TEXT("Local controller spawned"), Controller))
		{
			return;
		}
		LocalPlayer.Reset(NewObject<ULocalPlayer>(GEngine));
		Controller->SetPlayer(LocalPlayer.Get());
		Controller->Possess(Pawn);
		Context = FFoundryInputHandlerContext();
		Context.Pawn = Pawn;
		Context.PlayerController = Controller;
		Context.TriggerEvent = ETriggerEvent::Triggered;
		bReady = TestTrue(TEXT("Input Pawn is locally controlled"), Pawn->IsLocallyControlled());
	});

	AfterEach([this]()
	{
		Tickers.Reset();
		Context = FFoundryInputHandlerContext();
		Fixture.Reset();
		LocalPlayer.Reset();
		Pawn = nullptr;
		Controller = nullptr;
		bReady = false;
	});

	It("Should preserve raw movement axes and magnitude before GASP applies controller yaw once", [this]()
	{
		if (!bReady)
		{
			return;
		}
		TStrongObjectPtr<UInputHandler_GASPMove> Handler(NewObject<UInputHandler_GASPMove>());
		Controller->SetControlRotation(FRotator(0.f, 90.f, 0.f));
		Context.Value = FInputActionValue(FVector2D(0.25f, 0.f));
		Handler->HandleInput(Context);

		TestTrue(TEXT("Forward input retains its quarter-stick magnitude in the unrotated plane"), Pawn->GetPendingMovementInputVector().Equals(FVector(0.25f, 0.f, 0.f)));
		const FVector Direction = Pawn->GetMovementInputVector();
		TestTrue(TEXT("GASP rotates forward input by controller yaw exactly once"), Direction.Equals(FVector::RightVector));
	});

	It("Should honor Foundry movement blocking tags", [this]()
	{
		if (!bReady)
		{
			return;
		}
		TStrongObjectPtr<UInputHandler_GASPMove> Handler(NewObject<UInputHandler_GASPMove>());
		Pawn->GameplayTags.AddTag(NBS::Input::Handlers::BlockMovement);
		Context.Value = FInputActionValue(FVector2D(1.f, 0.f));
		Handler->HandleInput(Context);

		TestTrue(TEXT("Blocked movement adds no intent"), Pawn->GetPendingMovementInputVector().IsNearlyZero());
	});

	It("Should invert pitch without mutating the input and suppress camera input in twin-stick mode", [this]()
	{
		if (!bReady)
		{
			return;
		}
		TStrongObjectPtr<UInputHandler_GASPLook> Handler(NewObject<UInputHandler_GASPLook>());
		Context.Value = FInputActionValue(FVector2D(2.f, 3.f));
		Handler->HandleInput(Context);

		TestEqual(TEXT("Yaw retains its sign"), Pawn->ReceivedYaw, 2.f);
		TestEqual(TEXT("Pitch uses GASP's inversion"), Pawn->ReceivedPitch, -3.f);
		TestTrue(TEXT("Shared context retains the original input"), Context.Value.Get<FVector2D>().Equals(FVector2D(2.f, 3.f)));

		Pawn->SetTwinStickInputEnabled(true);
		Handler->HandleInput(Context);
		TestEqual(TEXT("Twin-stick input adds no camera yaw"), Pawn->ReceivedYaw, 2.f);
		TestEqual(TEXT("Twin-stick input adds no camera pitch"), Pawn->ReceivedPitch, -3.f);

		Pawn->SetTwinStickInputEnabled(false);
		Pawn->GameplayTags.AddTag(NBS::Input::Handlers::BlockCameraPitch);
		Handler->HandleInput(Context);
		TestEqual(TEXT("Foundry still permits unblocked yaw"), Pawn->ReceivedYaw, 4.f);
		TestEqual(TEXT("Foundry's pitch block is retained"), Pawn->ReceivedPitch, -3.f);
	});

	It("Should use the real GASP jump fallback when traversal declines", [this]()
	{
		if (!bReady)
		{
			return;
		}
		TStrongObjectPtr<UInputHandler_GASPJump> Handler(NewObject<UInputHandler_GASPJump>());
		Context.Value = FInputActionValue(true);
		Handler->HandleInput(Context);

		TestEqual(TEXT("GASP Pawn receives traversal first"), Pawn->TraversalAttempts, 1);
		TestTrue(TEXT("No obstacle permits an ordinary Mover jump"), Pawn->bJustPressedJump);
	});

	It("Should let an extended Pawn consume traversal without also jumping", [this]()
	{
		if (!bReady)
		{
			return;
		}
		TStrongObjectPtr<UInputHandler_GASPJump> Handler(NewObject<UInputHandler_GASPJump>());
		Pawn->bHandleTraversal = true;
		Context.Value = FInputActionValue(true);
		Handler->HandleInput(Context);

		TestEqual(TEXT("Traversal is requested once"), Pawn->TraversalAttempts, 1);
		TestFalse(TEXT("Accepted traversal suppresses the Mover jump pulse"), Pawn->bJustPressedJump);
	});

	LatentIt("Should preserve a short tap until Mover can observe the jump pulse", [this](const FDoneDelegate& Done)
	{
		if (!bReady)
		{
			Done.Execute();
			return;
		}
		TStrongObjectPtr<UInputHandler_GASPJump> Handler(NewObject<UInputHandler_GASPJump>());
		Context.Value = FInputActionValue(true);
		Handler->HandleInput(Context);
		TestTrue(TEXT("Initial press starts jumping"), Pawn->bJustPressedJump);

		Pawn->GameplayTags.AddTag(NBS::Input::Handlers::BlockMovement);
		Context.Value = FInputActionValue(false);
		Handler->HandleInput(Context);
		TestTrue(TEXT("Normal release preserves the short pulse despite changed starting eligibility"), Pawn->bJustPressedJump);
		TestEqual(TEXT("Release does not attempt traversal again"), Pawn->TraversalAttempts, 1);
		WaitForJumpReset(Done);
	});

	It("Should clear jump intent on cancellation, setup removal and unpossession", [this]()
	{
		if (!bReady)
		{
			return;
		}
		TStrongObjectPtr<UInputHandler_GASPJump> Handler(NewObject<UInputHandler_GASPJump>());
		Context.Value = FInputActionValue(true);
		Handler->HandleInput(Context);
		Context.TriggerEvent = ETriggerEvent::Canceled;
		Handler->HandleInput(Context);
		TestFalse(TEXT("Canceled input releases jumping"), Pawn->bJustPressedJump);

		Context.TriggerEvent = ETriggerEvent::Triggered;
		Handler->HandleInput(Context);
		Handler->OnInputSetupRemoved(Context);
		TestFalse(TEXT("Removing the setup releases jumping"), Pawn->bJustPressedJump);

		Handler->HandleInput(Context);
		Controller->UnPossess();
		TestFalse(TEXT("The previous Pawn releases jumping during unpossession"), Pawn->bJustPressedJump);
	});
}

#endif
