// Ninja Bear Studio Inc., all rights reserved.
#include "GASPActionTestTypes.h"

#include "GASPOverlayTestTypes.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/FoundryInteractionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Common/Fixtures/FoundryCoreTickerFixture.h"
#include "Common/Fixtures/FoundryGameInstanceFixture.h"
#include "Engine/SkeletalMesh.h"
#include "Core/CameraAsset.h"
#include "Core/CameraRigAsset.h"
#include "Directors/SingleCameraDirector.h"
#include "GameFramework/GameplayCameraComponent.h"
#include "Animation/AnimNotify_GASPActionHandoff.h"
#include "Misc/AutomationTest.h"
#include "MovementSet/GASPMoverComponent.h"
#include "PoseSearch/PoseSearchInteractionAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FGASPActionSpec, "NinjaBearStudio.GASP.Actions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FoundryTestFixtures::FGameInstanceFixture> World;
	FoundryTestFixtures::FCoreTickerFixture Tickers;
	AGASPActionTestPawn* Source = nullptr;
	AGASPActionTestPawn* Target = nullptr;
	UGASPTestActionComponent* Actions = nullptr;
	bool bReady = false;
	int32 CompletionCount = 0;
	bool bSucceeded = false;
	bool bSawFirstRoleFinish = false;

	/** Builds an animation-ready Pawn without running unrelated movement simulation. */
	AGASPActionTestPawn* CreatePawn(FVector Location);

	/** Copies the sample performance so a role can finish early without modifying any source asset. */
	bool ConfigurePerformance();

	/** Observes the action's public final result. */
	FGASPActionCompletion MakeCompletion();

	/** Ticks the World until a public observation is satisfied or the scenario exceeds its budget. */
	void TickUntil(FSimpleDelegate Done, TFunction<bool()> Predicate);
END_DEFINE_SPEC(FGASPActionSpec)

AGASPActionTestPawn* FGASPActionSpec::CreatePawn(const FVector Location)
{
	const FTransform SpawnTransform(FRotator::ZeroRotator, Location);
	AGASPActionTestPawn* Pawn = World->GetWorld()->SpawnActorDeferred<AGASPActionTestPawn>(AGASPActionTestPawn::StaticClass(), SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("The scenario creates its Pawn"), Pawn))
	{
		return nullptr;
	}
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/GASP/GASP/Characters/UE5_Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (!TestNotNull(TEXT("The sample mesh loads"), Mesh))
	{
		return nullptr;
	}
	Pawn->GetMesh()->SetSkeletalMeshAsset(Mesh);
	Pawn->GetMesh()->SetAnimInstanceClass(UGASPTestLayerA::StaticClass());
	Pawn->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	UGameplayCameraComponent* Camera = CastChecked<UGameplayCameraComponent>(Pawn->GetDefaultSubobjectByName(TEXT("GameplayCamera")));
	UCameraRigAsset* CameraRig = NewObject<UCameraRigAsset>(Camera);
	USingleCameraDirector* Director = NewObject<USingleCameraDirector>(Camera);
	Director->CameraRig = CameraRig;
	UCameraAsset* CameraAsset = NewObject<UCameraAsset>(Camera);
	CameraAsset->SetCameraDirector(Director);
	Camera->CameraReference.SetCameraAsset(CameraAsset);
	Pawn->FinishSpawning(SpawnTransform);
	Pawn->GetMoverComponent()->SetComponentTickEnabled(false);
	return Pawn;
}

bool FGASPActionSpec::ConfigurePerformance()
{
	UPoseSearchInteractionAsset* Sample = LoadObject<UPoseSearchInteractionAsset>(nullptr,
		TEXT("/GASP/GASP/Characters/UE5_Mannequins/Animations/Interactions/Takedowns/PSIA_M_relaxed_takedown_stand_F.PSIA_M_relaxed_takedown_stand_F"));
	if (!TestNotNull(TEXT("The sample performance loads"), Sample) || !TestEqual(TEXT("The sample supplies two roles"), Sample->Items.Num(), 2))
	{
		return false;
	}
	UPoseSearchInteractionAsset* Performance = DuplicateObject<UPoseSearchInteractionAsset>(Sample, Actions);
	for (int32 Index = 0; Index < Performance->Items.Num(); ++Index)
	{
		UAnimMontage* Original = Cast<UAnimMontage>(Sample->Items[Index].Animation);
		if (!TestNotNull(TEXT("Each role uses a montage"), Original))
		{
			return false;
		}
		UAnimMontage* Montage = DuplicateObject<UAnimMontage>(Original, Performance);
		Montage->RateScale = Index == 0 ? 8.f : 1.f;
		// This contract exercises playback completion; authored gameplay notifies are tested separately.
		Montage->Notifies.Reset();
		Performance->Items[Index].Animation = Montage;
	}
	Actions->Performance = Performance;
	return true;
}

FGASPActionCompletion FGASPActionSpec::MakeCompletion()
{
	return FGASPActionCompletion::CreateLambda([this](const bool bResult)
	{
		++CompletionCount;
		bSucceeded = bResult;
	});
}

void FGASPActionSpec::TickUntil(FSimpleDelegate Done, TFunction<bool()> Predicate)
{
	Tickers.AddTicker(TEXT("GASPActionContract"), 0.f, [this, Done, Predicate = MoveTemp(Predicate), Elapsed = 0.f](float) mutable
	{
		static constexpr float StepSeconds = 0.05f;
		static constexpr float ScenarioBudget = 10.f;
		World->GetWorld()->Tick(LEVELTICK_All, StepSeconds);
		Elapsed += StepSeconds;
		if (Predicate())
		{
			Done.Execute();
			return false;
		}
		if (Elapsed >= ScenarioBudget)
		{
			AddError(TEXT("The action did not reach its expected public state before the scenario timed out."));
			Done.Execute();
			return false;
		}
		return true;
	});
}

void FGASPActionSpec::Define()
{
	BeforeEach([this]()
	{
		bReady = false;
		CompletionCount = 0;
		bSucceeded = false;
		bSawFirstRoleFinish = false;
		World = MakeUnique<FoundryTestFixtures::FGameInstanceFixture>();
		if (!TestTrue(TEXT("The scenario owns an initialized Game Instance"), World->HasGameInstance()))
		{
			return;
		}
		Source = CreatePawn(FVector::ZeroVector);
		Target = CreatePawn(FVector(200.f, 0.f, 0.f));
		if (!IsValid(Source) || !IsValid(Target))
		{
			return;
		}
		Actions = Cast<UGASPTestActionComponent>(Source->GetActionComponent());
		bReady = TestNotNull(TEXT("The Pawn replaces its named Action Component"), Actions) && ConfigurePerformance();
	});
	AfterEach([this]()
	{
		Tickers.Reset();
		if (IsValid(Actions))
		{
			Actions->CancelReservedAction();
		}
		World.Reset();
		Source = nullptr;
		Target = nullptr;
		Actions = nullptr;
	});

	It("Leaves both participants free after repeated feasibility queries", [this]()
	{
		if (!bReady)
		{
			return;
		}
		for (int32 Query = 0; Query < 3; ++Query)
		{
			TestTrue(TEXT("The sample action remains feasible"), Actions->CanPerformAction(TEXT("Shove"), Target, FGASPActionHandle()));
		}
		TestFalse(TEXT("Focus queries do not reserve the source"), Actions->IsReserved());
		TestFalse(TEXT("Focus queries do not reserve the target"), Target->GetActionComponent()->IsReserved());
	});

	It("Reserves the chosen pair and prevents another source from taking its target", [this]()
	{
		if (!bReady)
		{
			return;
		}
		AGASPActionTestPawn* Competitor = CreatePawn(FVector(1000.f, 0.f, 0.f));
		if (!IsValid(Competitor))
		{
			return;
		}
		UGASPTestActionComponent* OtherActions = CastChecked<UGASPTestActionComponent>(Competitor->GetActionComponent());
		OtherActions->Performance = Actions->Performance;
		const FGASPActionHandle First = FGASPActionHandle::FromExecution(FGuid::NewGuid());
		const FGASPActionHandle Second = FGASPActionHandle::FromExecution(FGuid::NewGuid());
		const bool bPrepared = Actions->PrepareAction(First, TEXT("Shove"), Target);
		const bool bContended = OtherActions->PrepareAction(Second, TEXT("Shove"), Target);

		TestTrue(TEXT("Preparation reserves the explicit pair"), bPrepared);
		TestFalse(TEXT("A competing preparation is rejected"), bContended);
		Actions->CancelAction(First);
		const bool bRetried = OtherActions->PrepareAction(Second, TEXT("Shove"), Target);
		TestTrue(TEXT("The released target accepts another source"), bRetried);
		OtherActions->CancelAction(Second);
	});

	It("Keeps a retry safe from stale cancellation and Commit requests", [this]()
	{
		if (!bReady)
		{
			return;
		}
		const FGASPActionHandle Previous = FGASPActionHandle::FromExecution(FGuid::NewGuid());
		const FGASPActionHandle Current = FGASPActionHandle::FromExecution(FGuid::NewGuid());
		if (!TestTrue(TEXT("The first action prepares"), Actions->PrepareAction(Previous, TEXT("Shove"), Target)))
		{
			return;
		}
		Actions->CancelAction(Previous);
		const bool bPrepared = Actions->PrepareAction(Current, TEXT("Shove"), Target);
		Actions->CancelAction(Previous);
		const bool bStaleCommit = Actions->CommitAction(Previous, MakeCompletion());

		TestTrue(TEXT("The retry prepares independently"), bPrepared);
		TestFalse(TEXT("An old Commit cannot consume the retry"), bStaleCommit);
		TestTrue(TEXT("An old cancellation cannot release the retry"), Actions->IsPreparedFor(Current));
		TestEqual(TEXT("Rejected Commit invokes no completion"), CompletionCount, 0);
	});

	It("Releases a prepared pair when its target leaves play", [this]()
	{
		if (!bReady)
		{
			return;
		}
		const FGASPActionHandle Handle = FGASPActionHandle::FromExecution(FGuid::NewGuid());
		if (!TestTrue(TEXT("The pair prepares"), Actions->PrepareAction(Handle, TEXT("Shove"), Target)))
		{
			return;
		}
		Target->Destroy();
		TestFalse(TEXT("Target teardown releases the source"), Actions->IsReserved());
		TestFalse(TEXT("The abandoned action is no longer prepared"), Actions->IsPreparedFor(Handle));
	});

	LatentIt("Waits for the longer role instead of completing at the first blend-out", [this](const FDoneDelegate& Done)
	{
		if (!bReady)
		{
			Done.Execute();
			return;
		}
		const FGASPActionHandle Handle = FGASPActionHandle::FromExecution(FGuid::NewGuid());
		const bool bPrepared = Actions->PrepareAction(Handle, TEXT("Shove"), Target);
		const bool bCommitted = bPrepared && Actions->CommitAction(Handle, MakeCompletion());
		if (!TestTrue(TEXT("The real Mover playback starts"), bCommitted))
		{
			Done.Execute();
			return;
		}
		TickUntil(Done, [this]()
		{
			const bool bSourcePlaying = Source->GetMesh()->GetAnimInstance()->GetCurrentActiveMontage() != nullptr;
			const bool bTargetPlaying = Target->GetMesh()->GetAnimInstance()->GetCurrentActiveMontage() != nullptr;
			if (!bSourcePlaying && bTargetPlaying && CompletionCount == 0)
			{
				bSawFirstRoleFinish = true;
			}
			if (CompletionCount == 0)
			{
				return false;
			}
			TestTrue(TEXT("The shorter role ended while the action still waited"), bSawFirstRoleFinish);
			TestEqual(TEXT("The pair reports completion once"), CompletionCount, 1);
			TestTrue(TEXT("Both roles completed successfully"), bSucceeded);
			TestFalse(TEXT("Completion releases the source"), Actions->IsReserved());
			TestFalse(TEXT("Completion releases the target"), Target->GetActionComponent()->IsReserved());
			return true;
		});
	});

	LatentIt("Interrupts the whole performance when either role is interrupted", [this](const FDoneDelegate& Done)
	{
		if (!bReady)
		{
			Done.Execute();
			return;
		}
		const FGASPActionHandle Handle = FGASPActionHandle::FromExecution(FGuid::NewGuid());
		const bool bPrepared = Actions->PrepareAction(Handle, TEXT("Shove"), Target);
		const bool bCommitted = bPrepared && Actions->CommitAction(Handle, MakeCompletion());
		if (!TestTrue(TEXT("The performance starts"), bCommitted))
		{
			Done.Execute();
			return;
		}
		Target->GetMesh()->GetAnimInstance()->Montage_Stop(0.1f);
		TickUntil(Done, [this]()
		{
			if (CompletionCount == 0)
			{
				return false;
			}
			TestEqual(TEXT("Interruption reports once"), CompletionCount, 1);
			TestFalse(TEXT("An interrupted role fails the whole pair"), bSucceeded);
			TestFalse(TEXT("Interruption releases both participants"), Actions->IsReserved() || Target->GetActionComponent()->IsReserved());
			return true;
		});
	});

	It("Accepts an authored role handoff while continuing to await its partner", [this]()
	{
		if (!bReady)
		{
			return;
		}
		const FGASPActionHandle Handle = FGASPActionHandle::FromExecution(FGuid::NewGuid());
		const bool bPrepared = Actions->PrepareAction(Handle, TEXT("Shove"), Target);
		const bool bCommitted = bPrepared && Actions->CommitAction(Handle, MakeCompletion());
		if (!TestTrue(TEXT("The real performance starts"), bCommitted))
		{
			return;
		}
		UAnimNotify_GASPActionHandoff* Handoff = NewObject<UAnimNotify_GASPActionHandoff>(Source);
		const FAnimNotifyEventReference Event;
		Handoff->Notify(Source->GetMesh(), CastChecked<UAnimMontage>(Actions->Performance->Items[0].Animation), Event);
		TestEqual(TEXT("A single handoff still waits for the partner"), CompletionCount, 0);
		Handoff->Notify(Target->GetMesh(), CastChecked<UAnimMontage>(Actions->Performance->Items[1].Animation), Event);
		TestEqual(TEXT("The final handoff completes the pair once"), CompletionCount, 1);
		TestTrue(TEXT("Authored handoffs are successful completion"), bSucceeded);
		TestFalse(TEXT("The complete pair is released"), Actions->IsReserved() || Target->GetActionComponent()->IsReserved());
	});

	It("Preserves preexisting collision ignores and tick prerequisites when cancelled", [this]()
	{
		if (!bReady)
		{
			return;
		}
		Source->GetCapsuleComponent()->IgnoreComponentWhenMoving(Target->GetCapsuleComponent(), true);
		Source->GetMesh()->AddTickPrerequisiteComponent(Target->GetMesh());
		const FGASPActionHandle Handle = FGASPActionHandle::FromExecution(FGuid::NewGuid());
		const bool bPrepared = Actions->PrepareAction(Handle, TEXT("Shove"), Target);
		const bool bCommitted = bPrepared && Actions->CommitAction(Handle, MakeCompletion());
		if (!TestTrue(TEXT("The performance starts"), bCommitted))
		{
			return;
		}
		Actions->CancelAction(Handle);

		TestTrue(TEXT("The source's earlier collision ignore survives"), Source->GetCapsuleComponent()->GetMoveIgnoreComponents().Contains(Target->GetCapsuleComponent()));
		TestFalse(TEXT("The action's target-side collision ignore is removed"), Target->GetCapsuleComponent()->GetMoveIgnoreComponents().Contains(Source->GetCapsuleComponent()));
		const bool bHasPrerequisite = Source->GetMesh()->PrimaryComponentTick.GetPrerequisites().ContainsByPredicate([this](const FTickPrerequisite& Prerequisite)
		{
			return Prerequisite.Get() == &Target->GetMesh()->PrimaryComponentTick;
		});
		TestTrue(TEXT("The earlier tick prerequisite survives"), bHasPrerequisite);
		TestNull(TEXT("The source's local role result is cleared"), Actions->GetActionResult().SelectedAnim.Get());
		TestEqual(TEXT("Cancellation reports one interrupted result"), CompletionCount, 1);
	});

	LatentIt("Runs sample input through Foundry and releases GASP when Foundry cancels", [this](const FDoneDelegate& Done)
	{
		if (!bReady)
		{
			Done.Execute();
			return;
		}
		TickUntil(Done, [this]()
		{
			UFoundryInteractionComponent* Performer = Source->GetInteractionComponent();
			if (Performer->GetInteractionState().AvailableInteractions.IsEmpty())
			{
				return false;
			}
			Source->RequestAction(TEXT("Shove"));
			TestTrue(TEXT("The granted Execute Ability starts Foundry execution"), Performer->HasActiveInteraction());
			TestTrue(TEXT("Foundry preparation secured the GASP pair"), Actions->IsReserved() && Target->GetActionComponent()->IsReserved());
			Performer->CancelInteraction();
			TestFalse(TEXT("Foundry cancellation releases the entire pair"), Actions->IsReserved() || Target->GetActionComponent()->IsReserved());
			return true;
		});
	});
}

#endif
