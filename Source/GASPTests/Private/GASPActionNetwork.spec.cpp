// Ninja Bear Studio Inc., all rights reserved.
#include "CoreMinimal.h"

#include "GASPActionTestTypes.h"
#include "AssetCompilingManager.h"

#include "Common/Fixtures/FoundryCoreTickerFixture.h"
#include "Common/Fixtures/FoundryPIESessionFixture.h"
#include "Animation/AnimInstance.h"
#include "Components/FoundryInteractionComponent.h"
#include "Components/GASPActionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GASPFoundryPawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Settings/GASPCharacterSettings.h"
#include "Chooser.h"

#if WITH_EDITOR && WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FGASPActionNetworkSpec, "NinjaBearStudio.GASP.Actions.Network", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	enum class EPhase : uint8
	{
		AwaitWorlds,
		AwaitReady,
		AwaitPlayback,
		AwaitCompletion,
		AwaitRetry,
		AwaitCancellation,
		Ending
	};

	FoundryTestFixtures::FPIESessionFixture PlaySession;
	FoundryTestFixtures::FCoreTickerFixture Tickers;
	TStrongObjectPtr<UGASPInteractionTestObserver> Observer;
	TStrongObjectPtr<UClass> SampleClass;
	TWeakObjectPtr<AGASPFoundryPawn> Source;
	TWeakObjectPtr<AGASPFoundryPawn> Target;
	EPhase Phase = EPhase::AwaitWorlds;
	FGuid FirstExecution;
	FName TestAction;
	double StartedAt = 0.0;
	double ReadyAt = 0.0;
	bool bRetry = false;

	/** Starts one real topology using the sample Blueprint and its authored animation setup. */
	void Start(EPlayNetMode NetMode, FName ActionType, const FDoneDelegate& Done);

	/** Observes the same request, completion, repeat and cancellation contract on every peer. */
	bool Tick(const FDoneDelegate& Done);

	/** Supplies a local floor without saving a test map or relying on replicated component configuration. */
	static void CreateFloor(UWorld* World);

	/** Finds the corresponding replicated Pawn without retaining client Worlds. */
	static AGASPFoundryPawn* FindPawn(UWorld* World, FName Name);

	/** Checks that both roles were rebuilt with this machine's animation instances. */
	bool HasLocalPlayback(AGASPFoundryPawn* Pawn, UWorld* World);

	/** Requests shutdown on every exit and reports the failed phase when necessary. */
	void End(const FString& Failure = FString());

END_DEFINE_SPEC(FGASPActionNetworkSpec)

void FGASPActionNetworkSpec::Define()
{
	AfterEach([this]()
	{
		Tickers.Reset();
		PlaySession.Reset();
		Observer.Reset();
		SampleClass.Reset();
		Source.Reset();
		Target.Reset();
	});

	for (const EPlayNetMode Mode : { PIE_ListenServer, PIE_Client })
	{
		const TCHAR* ScenarioDescription = Mode == PIE_ListenServer
			? TEXT("Should complete authored actions and replicate repeat and cancellation to listen-server observers")
			: TEXT("Should complete authored actions and replicate repeat and cancellation to dedicated-server observers");

		LatentIt(ScenarioDescription, FTimespan::FromSeconds(85.0), [this, Mode](const FDoneDelegate& Done)
		{
			Start(Mode, TEXT("Shove"), Done);
		});

		const TCHAR* HandoffDescription = Mode == PIE_ListenServer
			? TEXT("Should complete an authored Takedown and its ragdoll handoff on a listen server")
			: TEXT("Should complete an authored Takedown and its ragdoll handoff on a dedicated server");

		LatentIt(HandoffDescription, FTimespan::FromSeconds(85.0), [this, Mode](const FDoneDelegate& Done)
		{
			Start(Mode, TEXT("Takedown"), Done);
		});
	}
}

void FGASPActionNetworkSpec::Start(const EPlayNetMode NetMode, const FName ActionType, const FDoneDelegate& Done)
{
	TestAction = ActionType;
	SampleClass.Reset(LoadClass<AGASPFoundryPawn>(nullptr, TEXT("/GASP/GASPExtras/Blueprints/CBP_SandboxCharacter.CBP_SandboxCharacter_C")));
	if (!TestNotNull(TEXT("The migrated sample Blueprint loads"), SampleClass.Get()))
	{
		Done.Execute();
		return;
	}
	const AGASPFoundryPawn* Defaults = SampleClass->GetDefaultObject<AGASPFoundryPawn>();
	UChooserTable* InteractionTable = Defaults->GetSettingsChecked()->InteractionTable.LoadSynchronous();
	if (!TestNotNull(TEXT("The sample's authored interaction Chooser loads"), InteractionTable))
	{
		Done.Execute();
		return;
	}
	// Build authored animation indexes before opening connections; startup stalls are not network behavior.
	FAssetCompilingManager::Get().FinishAllCompilation();
	Phase = EPhase::AwaitWorlds;
	bRetry = false;
	FirstExecution.Invalidate();
	Observer.Reset(NewObject<UGASPInteractionTestObserver>());
	StartedAt = FPlatformTime::Seconds();
	const int32 Players = NetMode == PIE_ListenServer ? 3 : 2;
	if (!PlaySession.Start(*this, NetMode, Players, AGameModeBase::StaticClass(), TEXT("/Engine/Maps/Entry")))
	{
		Done.Execute();
		return;
	}

	Tickers.AddTicker(TEXT("GASP.Actions.Network"), 0.f, [this, Done](float)
	{
		return Tick(Done);
	});
}

bool FGASPActionNetworkSpec::Tick(const FDoneDelegate& Done)
{
	if (Phase == EPhase::Ending)
	{
		if (!PlaySession.IsShutdownComplete(*this))
		{
			return true;
		}
		Done.Execute();
		return false;
	}
	static constexpr double ScenarioTimeout = 70.0;
	if (FPlatformTime::Seconds() - StartedAt > ScenarioTimeout)
	{
		if (Source.IsValid())
		{
			AddInfo(FString::Printf(TEXT("Source %s at %s, mode %s, offers %d, active %d, completions %d."), *Source->GetName(), *Source->GetActorLocation().ToString(), *Source->GetMovementMode().ToString(), Source->GetInteractionComponent()->GetInteractionState().AvailableInteractions.Num(), Source->GetInteractionComponent()->HasActiveInteraction(), Observer->CompletionCount));
		}
		End(FString::Printf(TEXT("Authored GASP networking exceeded its deadline at phase %d."), static_cast<int32>(Phase)));
		return true;
	}

	UWorld* Server = nullptr;
	TArray<UWorld*> Clients;
	FoundryTestFixtures::FPIESessionFixture::ResolveWorlds(Server, Clients);
	if (!IsValid(Server) || Clients.Num() != 2)
	{
		return true;
	}
	if (Phase == EPhase::AwaitWorlds)
	{
		APlayerController* Owner = nullptr;
		for (FConstPlayerControllerIterator It = Server->GetPlayerControllerIterator(); It; ++It)
		{
			if (It->IsValid() && !It->Get()->IsLocalController())
			{
				Owner = It->Get();
				break;
			}
		}
		if (!IsValid(Owner))
		{
			return true;
		}
		CreateFloor(Server);
		for (UWorld* Client : Clients)
		{
			CreateFloor(Client);
		}
		const FTransform SourceTransform(FRotator::ZeroRotator, FVector(1000.f, 0.f, 100.f));
		const FTransform TargetTransform(FRotator(0.f, 180.f, 0.f), FVector(1150.f, 0.f, 100.f));
		AGASPFoundryPawn* Initiator = Server->SpawnActorDeferred<AGASPFoundryPawn>(SampleClass.Get(), SourceTransform, Owner, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		AGASPFoundryPawn* Partner = Server->SpawnActorDeferred<AGASPFoundryPawn>(SampleClass.Get(), TargetTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!TestNotNull(TEXT("The sample initiator spawns"), Initiator) || !TestNotNull(TEXT("The sample partner spawns"), Partner))
		{
			End();
			return true;
		}
		Initiator->AutoPossessAI = EAutoPossessAI::Disabled;
		Partner->AutoPossessAI = EAutoPossessAI::Disabled;
		Initiator->FinishSpawning(SourceTransform);
		Partner->FinishSpawning(TargetTransform);
		APawn* PreviousPawn = Owner->GetPawn();
		Owner->Possess(Initiator);
		Initiator->GetInteractionComponent()->OnInteractionFinished.AddUniqueDynamic(Observer.Get(), &UGASPInteractionTestObserver::HandleFinished);
		if (IsValid(PreviousPawn))
		{
			PreviousPawn->Destroy();
		}
		Source = Initiator;
		Target = Partner;
		ReadyAt = FPlatformTime::Seconds();
		Phase = EPhase::AwaitReady;
		AddInfo(TEXT("The remote player now owns the authored source Pawn."));
		return true;
	}
	if (!Source.IsValid() || !Target.IsValid())
	{
		End(TEXT("An authored participant left play unexpectedly."));
		return true;
	}

	AGASPFoundryPawn* RemoteOwner = nullptr;
	bool bAllPlaying = true;
	bool bAllStopped = true;
	bool bAllHandedOff = Target->IsRagdolling();
	for (UWorld* Client : Clients)
	{
		AGASPFoundryPawn* ClientSource = FindPawn(Client, Source->GetFName());
		AGASPFoundryPawn* ClientTarget = FindPawn(Client, Target->GetFName());
		if (!IsValid(ClientSource) || !IsValid(ClientTarget))
		{
			return true;
		}
		if (ClientSource->IsLocallyControlled())
		{
			RemoteOwner = ClientSource;
		}
		bAllPlaying &= HasLocalPlayback(ClientSource, Client) && HasLocalPlayback(ClientTarget, Client);
		bAllStopped &= ClientSource->GetActionComponent()->GetActionResult().SelectedAnim == nullptr;
		bAllStopped &= ClientTarget->GetActionComponent()->GetActionResult().SelectedAnim == nullptr;
		bAllHandedOff &= ClientTarget->IsRagdolling();
	}
	if (!IsValid(RemoteOwner))
	{
		return true;
	}
	UFoundryInteractionComponent* Performer = Source->GetInteractionComponent();
	const int32 ExpectedCompletedAttempts = bRetry ? 1 : 0;
	if (Phase == EPhase::AwaitPlayback && Observer->CompletionCount > ExpectedCompletedAttempts)
	{
		End(FString::Printf(TEXT("The Foundry request ended before every peer showed both roles. Outcome: %s."), *UEnum::GetValueAsString(Observer->LastResult.Outcome)));
		return true;
	}
	if (Phase == EPhase::AwaitReady || Phase == EPhase::AwaitRetry)
	{
		static constexpr double PoseHistoryWarmup = 2.0;
		const bool bHasOffer = !Performer->GetInteractionState().AvailableInteractions.IsEmpty();
		const bool bCanPerform = Source->GetActionComponent()->CanPerformAction(TestAction, Target.Get(), FGASPActionHandle());
		if (!bHasOffer || !bCanPerform || FPlatformTime::Seconds() - ReadyAt < PoseHistoryWarmup)
		{
			return true;
		}
		RemoteOwner->RequestAction(TestAction);
		AddInfo(FString::Printf(TEXT("The owning client requested the authored %s through Foundry."), *TestAction.ToString()));
		Phase = EPhase::AwaitPlayback;
	}
	else if (Phase == EPhase::AwaitPlayback && Performer->HasActiveInteraction() && bAllPlaying)
	{
		const FGuid Execution = Performer->GetActiveInteractionContext().ExecutionId;
		TestTrue(TEXT("The authored performance belongs to an accepted Foundry execution"), Execution.IsValid());
		TestTrue(TEXT("Both authority roles expose local animation results"), HasLocalPlayback(Source.Get(), Server) && HasLocalPlayback(Target.Get(), Server));
		if (bRetry)
		{
			TestTrue(TEXT("Repeating the same action creates a fresh execution"), Execution != FirstExecution);
			Performer->CancelInteraction();
			Phase = EPhase::AwaitCancellation;
		}
		else
		{
			FirstExecution = Execution;
			Phase = EPhase::AwaitCompletion;
		}
	}
	else if (Phase == EPhase::AwaitCompletion && !Performer->HasActiveInteraction() && bAllStopped)
	{
		if (TestAction == TEXT("Takedown") && !bAllHandedOff)
		{
			return true;
		}
		TestEqual(TEXT("Natural playback reports one terminal result"), Observer->CompletionCount, 1);
		TestTrue(TEXT("The authored performance succeeds through Foundry"), Observer->LastResult.IsCompleted());
		TestFalse(TEXT("Natural completion releases the source reservation"), Source->GetActionComponent()->IsReserved());
		TestFalse(TEXT("Natural completion releases the target reservation"), Target->GetActionComponent()->IsReserved());
		if (TestAction == TEXT("Takedown"))
		{
			TestTrue(TEXT("The authored handoff leaves the victim ragdolling on every peer"), bAllHandedOff);
			End();
			return true;
		}
		bRetry = true;
		ReadyAt = FPlatformTime::Seconds();
		Phase = EPhase::AwaitRetry;
	}
	else if (Phase == EPhase::AwaitCancellation && !Performer->HasActiveInteraction() && bAllStopped)
	{
		TestEqual(TEXT("Each execution reports exactly one result"), Observer->CompletionCount, 2);
		TestEqual(TEXT("Cancellation preserves its Foundry outcome"), Observer->LastResult.Outcome, EInteractionOutcome::Cancelled);
		TestFalse(TEXT("Cancellation releases both participant reservations"), Source->GetActionComponent()->IsReserved() || Target->GetActionComponent()->IsReserved());
		End();
	}
	return true;
}

void FGASPActionNetworkSpec::CreateFloor(UWorld* World)
{
	AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector(1000.f, 0.f, -25.f), FRotator::ZeroRotator);
	Floor->SetMobility(EComponentMobility::Movable);
	Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Floor->SetActorScale3D(FVector(40.f, 40.f, 0.5f));
	Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
}

AGASPFoundryPawn* FGASPActionNetworkSpec::FindPawn(UWorld* World, const FName Name)
{
	for (TActorIterator<AGASPFoundryPawn> It(World); It; ++It)
	{
		if (It->GetFName() == Name)
		{
			return *It;
		}
	}
	return nullptr;
}

bool FGASPActionNetworkSpec::HasLocalPlayback(AGASPFoundryPawn* Pawn, UWorld* World)
{
	const FPoseSearchBlueprintResult Result = Pawn->GetActionComponent()->GetActionResult();
	if (Result.SelectedAnim == nullptr || Result.AnimContexts.Num() != 2)
	{
		return false;
	}
	for (const UObject* Context : Result.AnimContexts)
	{
		const UAnimInstance* Animation = Cast<UAnimInstance>(Context);
		if (!IsValid(Animation) || Animation->GetWorld() != World)
		{
			return false;
		}
	}
	return true;
}

void FGASPActionNetworkSpec::End(const FString& Failure)
{
	if (!Failure.IsEmpty())
	{
		AddError(Failure);
	}
	Phase = EPhase::Ending;
	PlaySession.RequestEndPlay();
}

#endif
