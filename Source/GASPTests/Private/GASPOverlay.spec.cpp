// Ninja Bear Studio Inc., all rights reserved.
#include "GASPOverlayTestTypes.h"

#include <initializer_list>
#include "Chooser.h"
#include "GameplayTasksComponent.h"
#include "ObjectChooser_Asset.h"
#include "Common/Fixtures/FoundryWorldFixture.h"
#include "Components/SkeletalMeshComponent.h"
#include "Misc/AutomationTest.h"
#include "MovementSet/GASPMoverComponent.h"
#include "Subsystems/FoundryInventoryPresenceSubsystem.h"
#include "Subsystems/FoundryInventorySubsystem.h"
#include "Types/FInventoryDefaultTypes.h"
#include "Types/TagTypes.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FGASPOverlaySpec, "NinjaBearStudio.GASP.Overlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

	TUniquePtr<FoundryTestFixtures::FWorldFixture> Fixture;
	AGASPTestCharacter* Character = nullptr;
	UGASPTestOverlayComponent* Overlay = nullptr;
	TStrongObjectPtr<UObject> PrimarySource;
	TStrongObjectPtr<UObject> SecondarySource;
	bool bScenarioReady = false;

	/**
	 * Builds an explicitly named tag set for a scenario's authored overlay input.
	 */
	static FGameplayTagContainer MakeOverlayTags(std::initializer_list<FGameplayTag> Values);

END_DEFINE_SPEC(FGASPOverlaySpec)

FGameplayTagContainer FGASPOverlaySpec::MakeOverlayTags(std::initializer_list<FGameplayTag> Values)
{
	FGameplayTagContainer OverlayTags;
	for (const FGameplayTag Tag : Values)
	{
		OverlayTags.AddTag(Tag);
	}

	return OverlayTags;
}

void FGASPOverlaySpec::Define()
{
	BeforeEach([this]()
	{
		bScenarioReady = false;
		Fixture = MakeUnique<FoundryTestFixtures::FWorldFixture>();
		if (!TestTrue(TEXT("Test World is available"), Fixture->HasWorld()))
		{
			return;
		}

		Character = Fixture->GetWorld()->SpawnActor<AGASPTestCharacter>();
		if (!TestNotNull(TEXT("GASP test character spawned"), Character))
		{
			return;
		}

		Overlay = Cast<UGASPTestOverlayComponent>(Character->GetOverlayComponent());
		if (!TestNotNull(TEXT("Character uses the test overlay subclass"), Overlay))
		{
			return;
		}

		Overlay->AnimationInstance = NewObject<UGASPTestLayerA>(Character->GetMesh());
		Overlay->RefreshOverlay();
		Overlay->BindOverlayChanges();
		Overlay->ResetRecordedChanges();

		PrimarySource.Reset(NewObject<UGASPTestOverlaySource>(Character));
		SecondarySource.Reset(NewObject<UGASPTestOverlaySource>(Character));
		bScenarioReady = true;
	});

	AfterEach([this]()
	{
		PrimarySource.Reset();
		SecondarySource.Reset();
		Fixture.Reset();
		Character = nullptr;
		Overlay = nullptr;
		bScenarioReady = false;
	});

	Describe("Source Ownership", [this]()
	{
		It("Should preserve a shared tag until its final source is removed", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			const FGameplayTagContainer RifleTags = MakeOverlayTags({OverlayModeTags::Rifle});
			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), RifleTags);
			Overlay->SetOverlayContribution(SecondarySource.Get(), TEXT("Secondary"), RifleTags);

			TestEqual(TEXT("Class linked once"), Overlay->LinkedLayers.Num(), 1);
			TestEqual(TEXT("Effective state notified once"), Overlay->OverlayChangeCount, 1);

			Overlay->RemoveOverlayContribution(PrimarySource.Get(), TEXT("Primary"));

			TestTrue(TEXT("Other source retains tag"), Character->GetOverlayMode().HasTagExact(OverlayModeTags::Rifle));
			TestEqual(TEXT("Shared class retained"), Overlay->UnlinkedLayers.Num(), 0);

			Overlay->RemoveOverlayContribution(SecondarySource.Get(), TEXT("Secondary"));

			TestTrue(TEXT("Last source clears tag"), Character->GetOverlayMode().IsEmpty());
			TestEqual(TEXT("Class unlinked once"), Overlay->UnlinkedLayers.Num(), 1);
		});

		It("Should accept pending intent safely without a character, mesh, or settings", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			UGASPOverlayComponent* Standalone = NewObject<UGASPOverlayComponent>();

			const FGameplayTagContainer RifleTags = MakeOverlayTags({OverlayModeTags::Rifle});
			const bool bAcceptedNullSource = Standalone->SetOverlayContribution(nullptr, NAME_None, RifleTags);

			TestFalse(TEXT("Null sources rejected"), bAcceptedNullSource);

			Standalone->SetOverlayContribution(PrimarySource.Get(), NAME_None, MakeOverlayTags({OverlayModeTags::Rifle}));

			TestTrue(TEXT("Intent retained without a target"), Standalone->GetOverlayMode().HasTagExact(OverlayModeTags::Rifle));

			Standalone->SetTargetMesh(nullptr);
			Standalone->RemoveOverlayContribution(PrimarySource.Get(), NAME_None);

			TestTrue(TEXT("Pending intent released safely"), Standalone->GetOverlayMode().IsEmpty());
		});

		It("Should treat repeated apply and missing removal as idempotent", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			const FGameplayTagContainer RifleTags = MakeOverlayTags({OverlayModeTags::Rifle});
			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), RifleTags);
			const int32 OverlayEvaluationCount = Overlay->OverlayEvaluationCount;

			const bool bChangedDuplicate = Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), RifleTags);
			const bool bRemovedMissingSource = Overlay->RemoveOverlayContribution(PrimarySource.Get(), TEXT("Missing"));

			TestFalse(TEXT("Duplicate is unchanged"), bChangedDuplicate);
			TestFalse(TEXT("Missing removal is unchanged"), bRemovedMissingSource);
			TestEqual(TEXT("No redundant Chooser evaluation"), Overlay->OverlayEvaluationCount, OverlayEvaluationCount);

			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), FGameplayTagContainer());

			TestTrue(TEXT("Empty replacement releases source"), Character->GetOverlayMode().IsEmpty());
			TestEqual(TEXT("No leaked duplicate ownership"), Overlay->UnlinkedLayers.Num(), 1);
		});

		It("Should keep manual overlays separate from Presence contributions", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			Character->SetOverlayMode(MakeOverlayTags({OverlayModeTags::Bow}));
			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), MakeOverlayTags({OverlayModeTags::Rifle}));

			TestTrue(TEXT("Chooser sees the union"), Character->GetOverlayMode() == MakeOverlayTags({OverlayModeTags::Bow, OverlayModeTags::Rifle}));
			TestTrue(TEXT("Replicated manual state is unchanged"), Character->GetBaseOverlayMode() == MakeOverlayTags({OverlayModeTags::Bow}));

			Character->SetOverlayMode(FGameplayTagContainer());

			TestTrue(TEXT("Manual replacement preserves equipment"), Character->GetOverlayMode() == MakeOverlayTags({OverlayModeTags::Rifle}));
		});
	});

	Describe("Chooser and Layer Precedence", [this]()
	{
		It("Should preserve a class selected by different tags without relinking", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), MakeOverlayTags({OverlayModeTags::Rifle}));
			Overlay->SetOverlayContribution(SecondarySource.Get(), TEXT("Secondary"), MakeOverlayTags({OverlayModeTags::Bow}));
			Overlay->RemoveOverlayContribution(PrimarySource.Get(), TEXT("Primary"));

			TestEqual(TEXT("Unchanged class linked only once"), Overlay->LinkedLayers.Num(), 1);
			TestEqual(TEXT("No unlink cycle"), Overlay->UnlinkedLayers.Num(), 0);
			TestTrue(TEXT("Remaining context published"), Character->GetOverlayMode() == MakeOverlayTags({OverlayModeTags::Bow}));
		});

		It("Should reevaluate the complete combination and swap only changed classes", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), MakeOverlayTags({OverlayModeTags::Rifle}));
			Overlay->SetOverlayContribution(SecondarySource.Get(), TEXT("Secondary"), MakeOverlayTags({OverlayModeTags::Torch}));

			if (!TestFalse(TEXT("A layer was linked before checking its class"), Overlay->LinkedLayers.IsEmpty()))
			{
				return;
			}

			TestEqual(TEXT("Combination class selected"), Overlay->LinkedLayers.Last(), TSubclassOf<UAnimInstance>(UGASPTestLayerB::StaticClass()));
			TestEqual(TEXT("Old class removed once"), Overlay->UnlinkedLayers.Num(), 1);

			Overlay->RemoveOverlayContribution(SecondarySource.Get(), TEXT("Secondary"));

			TestEqual(TEXT("Remaining context selects original class"), Overlay->LinkedLayers.Last(), TSubclassOf<UAnimInstance>(UGASPTestLayerA::StaticClass()));
			TestEqual(TEXT("Combination class removed"), Overlay->UnlinkedLayers.Last(), TSubclassOf<UAnimInstance>(UGASPTestLayerB::StaticClass()));
		});

		It("Should resolve an actual Chooser asset through the character settings", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			UGASPTestLinkedSet* Set = NewObject<UGASPTestLinkedSet>(Character);
			Set->SetLayer(UGASPTestLayerB::StaticClass());
			UChooserTable* Chooser = NewObject<UChooserTable>(Character);
			Chooser->FallbackResult.InitializeAs<FAssetChooser>();
			Chooser->FallbackResult.GetMutable<FAssetChooser>().Asset = Set;
			Character->SetOverlayChooser(Chooser);
			Overlay->bUseConfiguredChooser = true;
			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), MakeOverlayTags({OverlayModeTags::Rifle}));

			TestEqual(TEXT("Configured Chooser result is linked"), Overlay->LinkedLayers.Num(), 1);

			if (!Overlay->LinkedLayers.IsEmpty())
			{

			if (!TestFalse(TEXT("A layer was linked before checking its class"), Overlay->LinkedLayers.IsEmpty()))
			{
				return;
			}

				TestEqual(TEXT("Linked set class selected"), Overlay->LinkedLayers.Last(), TSubclassOf<UAnimInstance>(UGASPTestLayerB::StaticClass()));

			}
		});

		It("Should share classes between pose and overlay and restore lower layers", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			Overlay->PoseLayer = UGASPTestLayerA::StaticClass();
			Overlay->RefreshOverlay();
			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), MakeOverlayTags({OverlayModeTags::Rifle}));
			Overlay->RemoveOverlayContribution(PrimarySource.Get(), TEXT("Primary"));

			TestEqual(TEXT("Pose keeps shared class"), Overlay->UnlinkedLayers.Num(), 0);
			TestEqual(TEXT("Shared class linked once"), Overlay->LinkedLayers.Num(), 1);

			Overlay->PoseLayer = UGASPTestLayerB::StaticClass();
			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), MakeOverlayTags({OverlayModeTags::Rifle}));

			if (!TestFalse(TEXT("A layer was linked before checking its class"), Overlay->LinkedLayers.IsEmpty()))
			{
				return;
			}

			TestEqual(TEXT("Pose applied after overlay"), Overlay->LinkedLayers.Last(), TSubclassOf<UAnimInstance>(UGASPTestLayerB::StaticClass()));

			Overlay->PoseLayer = nullptr;
			Overlay->RefreshOverlay();

			TestEqual(TEXT("Removed pose restores retained overlay"), Overlay->LinkedLayers.Last(), TSubclassOf<UAnimInstance>(UGASPTestLayerA::StaticClass()));
		});
	});

	Describe("Update Batching and Lifecycle", [this]()
	{
		It("Should batch replacement contributions without interrupting an unchanged class", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), MakeOverlayTags({OverlayModeTags::Rifle}));
			const int32 OverlayEvaluationCount = Overlay->OverlayEvaluationCount;
			Overlay->BeginOverlayUpdate();
			Overlay->RemoveOverlayContribution(PrimarySource.Get(), TEXT("Primary"));
			Overlay->SetOverlayContribution(SecondarySource.Get(), TEXT("Secondary"), MakeOverlayTags({OverlayModeTags::Bow}));
			Overlay->EndOverlayUpdate();

			TestEqual(TEXT("One final evaluation"), Overlay->OverlayEvaluationCount, OverlayEvaluationCount + 1);
			TestEqual(TEXT("Class never unlinked"), Overlay->UnlinkedLayers.Num(), 0);
			TestEqual(TEXT("Class never relinked"), Overlay->LinkedLayers.Num(), 1);
		});

		It("Should retain intent without an animation instance and reapply on initialization", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			Overlay->AnimationInstance = nullptr;
			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), MakeOverlayTags({OverlayModeTags::Rifle}));

			TestEqual(TEXT("No animation instance means no link"), Overlay->LinkedLayers.Num(), 0);

			Overlay->AnimationInstance = NewObject<UGASPTestLayerA>(Character->GetMesh());
			Character->GetMesh()->OnAnimInitialized.Broadcast();

			TestEqual(TEXT("Late animation receives layer"), Overlay->LinkedLayers.Num(), 1);

			Character->GetMesh()->OnAnimInitialized.Broadcast();

			TestEqual(TEXT("Same-instance reinitialization reapplies"), Overlay->LinkedLayers.Num(), 2);

			Overlay->AnimationInstance = NewObject<UGASPTestLayerB>(Character->GetMesh());
			Character->GetMesh()->OnAnimInitialized.Broadcast();

			TestEqual(TEXT("Replacement instance receives layer"), Overlay->LinkedLayers.Num(), 3);
			TestEqual(TEXT("Previous instance released"), Overlay->UnlinkedLayers.Num(), 1);
		});

		It("Should handle unmatched Choosers and expired sources", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			Overlay->SingleOverlayLayer = nullptr;
			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), MakeOverlayTags({OverlayModeTags::Rifle}));

			TestEqual(TEXT("Null result is not linked"), Overlay->LinkedLayers.Num(), 0);

			Overlay->SingleOverlayLayer = UGASPTestLayerA::StaticClass();
			Overlay->RefreshOverlay();

			TestEqual(TEXT("Intent survives missing result"), Overlay->LinkedLayers.Num(), 1);

			PrimarySource->MarkAsGarbage();
			Overlay->RefreshOverlay();

			TestTrue(TEXT("Expired source removed"), Character->GetOverlayMode().IsEmpty());
			TestEqual(TEXT("Expired source releases class"), Overlay->UnlinkedLayers.Num(), 1);
		});

		It("Should converge after a listener changes contributions during notification", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			Overlay->bReenterOnChange = true;
			Overlay->ReentrantSource = SecondarySource.Get();
			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), MakeOverlayTags({OverlayModeTags::Rifle}));

			TestTrue(TEXT("Reentrant state retained"), Character->GetOverlayMode() == MakeOverlayTags({OverlayModeTags::Rifle, OverlayModeTags::Torch}));
			if (!TestFalse(TEXT("A layer was linked before checking its class"), Overlay->LinkedLayers.IsEmpty()))
			{
				return;
			}

			TestEqual(TEXT("Reentrant context resolves combination"), Overlay->LinkedLayers.Last(), TSubclassOf<UAnimInstance>(UGASPTestLayerB::StaticClass()));
		});

		It("Should clean shared layers once and reject contributions after teardown", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), MakeOverlayTags({OverlayModeTags::Rifle}));
			Overlay->SetOverlayContribution(SecondarySource.Get(), TEXT("Secondary"), MakeOverlayTags({OverlayModeTags::Bow}));
			Overlay->SimulateEndPlay();

			TestEqual(TEXT("Unique class released once"), Overlay->UnlinkedLayers.Num(), 1);
			const bool bAcceptedAfterTeardown = Overlay->SetOverlayContribution(PrimarySource.Get(), TEXT("Primary"), MakeOverlayTags({OverlayModeTags::Rifle}));

			TestFalse(TEXT("Torn down component rejects new intent"), bAcceptedAfterTeardown);

			Character->GetMesh()->OnAnimInitialized.Broadcast();

			TestEqual(TEXT("Initialization binding removed"), Overlay->LinkedLayers.Num(), 1);
		});
	});

	Describe("Foundry Presence Integration", [this]()
	{
		It("Should use Foundry Presence source identities without modifying manual overlay state", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			UGASPTestPresenceModifier* Modifier = NewObject<UGASPTestPresenceModifier>(Character);
			Modifier->SetTags(MakeOverlayTags({OverlayModeTags::Rifle}));

			FInventoryPresenceContext FirstPresence;
			FirstPresence.Owner = Character;
			FirstPresence.Inventory.Id = FGuid::NewGuid();
			FirstPresence.Item.Id = FGuid::NewGuid();

			FInventoryPresenceContext SecondPresence = FirstPresence;
			SecondPresence.Item.Id = FGuid::NewGuid();
			Modifier->ApplyModifier(FirstPresence);
			Modifier->ApplyModifier(FirstPresence);
			Modifier->ApplyModifier(SecondPresence);
			Modifier->RemoveModifier(FirstPresence);

			TestTrue(TEXT("Second item retains overlay"), Character->GetOverlayMode().HasTagExact(OverlayModeTags::Rifle));
			TestTrue(TEXT("No manual state/RPC path used"), Character->GetBaseOverlayMode().IsEmpty());
			TestNull(TEXT("Presentation does not materialize lazy ASC"), Character->FindAbilitySystemComponent());

			Modifier->RemoveModifier(SecondPresence);

			TestTrue(TEXT("Both item contributions released"), Character->GetOverlayMode().IsEmpty());
			TestEqual(TEXT("Duplicate apply did not leak"), Overlay->UnlinkedLayers.Num(), 1);

			FInventoryPresenceContext Invalid;

			TestFalse(TEXT("Invalid context rejected"), Modifier->CanApplyModifier(Invalid));

			Modifier->RemoveModifier(Invalid);
		});

		It("Should reconcile received Foundry Presence snapshots atomically without a private Inventory", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			UGASPTestItemDefinition* Definition = NewObject<UGASPTestItemDefinition>();
			UGASPTestEquipmentFragment* Equipment = NewObject<UGASPTestEquipmentFragment>(Definition);
			UGASPTestPresenceModifier* Modifier = NewObject<UGASPTestPresenceModifier>(Definition);
			Modifier->SetTags(MakeOverlayTags({OverlayModeTags::Rifle}));
			Equipment->SetEquippedModifier(Modifier);
			Definition->SetEquipmentFragment(Equipment);

			UGASPTestPresenceComponent* Presence = NewObject<UGASPTestPresenceComponent>(Character);
			Presence->RegisterComponent();

			FInventoryPresenceContext FirstPresence;
			FirstPresence.Owner = Character;
			FirstPresence.Inventory.Id = FGuid::NewGuid();
			FirstPresence.Item.Id = FGuid::NewGuid();
			FirstPresence.Container.Id = FGuid::NewGuid();
			FirstPresence.Position.Slot = 0;
			FirstPresence.ItemDefinition = Definition;
			FirstPresence.EquipmentState = EInventoryEquipmentState::Equipped;

			UFoundryInventoryPresenceSubsystem* Subsystem = UFoundryInventoryPresenceSubsystem::Get(Character);
			if (!TestNotNull(TEXT("Presence Subsystem is available"), Subsystem))
			{
				return;
			}

			Subsystem->RegisterPresenceTransport(Presence, FirstPresence.Inventory);
			Presence->ReceivePresences({FirstPresence});

			TestTrue(TEXT("Remote intent contributes overlay"), Character->GetOverlayMode().HasTagExact(OverlayModeTags::Rifle));

			UFoundryInventorySubsystem* Inventories = UFoundryInventorySubsystem::Get(Character);
			if (!TestNotNull(TEXT("Inventory Subsystem is available"), Inventories))
			{
				return;
			}

			TestNull(TEXT("No private Inventory needed"), Inventories->FindInventory(FirstPresence.Inventory));

			FInventoryPresenceContext SecondPresence = FirstPresence;
			SecondPresence.Item.Id = FGuid::NewGuid();
			Presence->ReceivePresences({SecondPresence});

			TestEqual(TEXT("Replacement keeps identical class linked"), Overlay->LinkedLayers.Num(), 1);
			TestEqual(TEXT("Replacement never unlinks identical class"), Overlay->UnlinkedLayers.Num(), 0);

			SecondPresence.Position.Slot = 1;
			Presence->ReceivePresences({SecondPresence});

			TestEqual(TEXT("Placement refresh keeps class linked"), Overlay->LinkedLayers.Num(), 1);
			TestEqual(TEXT("Placement refresh has no unlink cycle"), Overlay->UnlinkedLayers.Num(), 0);

			SecondPresence.EquipmentState = EInventoryEquipmentState::Stored;
			Presence->ReceivePresences({SecondPresence});

			TestTrue(TEXT("Stored state removes Equipped overlays"), Character->GetOverlayMode().IsEmpty());

			SecondPresence.EquipmentState = EInventoryEquipmentState::Equipped;
			Presence->ReceivePresences({SecondPresence});
			Subsystem->UnregisterPresenceTransport(Presence);

			TestTrue(TEXT("Old visual host releases contributions"), Character->GetOverlayMode().IsEmpty());

			AGASPTestCharacter* Replacement = Fixture->GetWorld()->SpawnActor<AGASPTestCharacter>();
			if (!TestNotNull(TEXT("Replacement visual host spawned"), Replacement))
			{
				return;
			}

			UGASPTestPresenceComponent* NewPresence = NewObject<UGASPTestPresenceComponent>(Replacement);
			NewPresence->RegisterComponent();
			Subsystem->RegisterPresenceTransport(NewPresence, SecondPresence.Inventory);
			NewPresence->ReceivePresences({SecondPresence});

			TestTrue(TEXT("New visual host rebuilds persistent equipment overlay"), Replacement->GetOverlayMode().HasTagExact(OverlayModeTags::Rifle));

			NewPresence->ReceivePresences({});

			TestTrue(TEXT("Received removal releases final overlay"), Replacement->GetOverlayMode().IsEmpty());
		});

		It("Should associate persistent Inventory when the Player State arrives after Presence begins", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			AGASPTestPlayerState* State = Fixture->GetWorld()->SpawnActor<AGASPTestPlayerState>();
			if (!TestNotNull(TEXT("Player State spawned"), State))
			{
				return;
			}

			UFoundryInventorySubsystem* Inventories = UFoundryInventorySubsystem::Get(Character);
			if (!TestNotNull(TEXT("Inventory Subsystem is available"), Inventories))
			{
				return;
			}

			const FInventoryHandle Inventory = Inventories->RegisterInventoryOwner(State, FInventoryInitializationData());

			TestTrue(TEXT("Persistent Inventory exists before pawn association"), Inventory.IsValid());

			UGASPTestPresenceComponent* Presence = NewObject<UGASPTestPresenceComponent>(Character);
			Presence->RegisterComponent();

			TestFalse(TEXT("No Player State yet"), Presence->IsPresenceInitialized());

			Character->SetExternalAbilitySystemOwner(State);

			TestTrue(TEXT("ASC handoff retries Inventory association immediately"), Presence->GetInventoryHandle() == Inventory);

			Presence->DestroyComponent();
		});

		It("Should honor manually configured Inventory association during an ASC handoff", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			AGASPTestPlayerState* State = Fixture->GetWorld()->SpawnActor<AGASPTestPlayerState>();
			if (!TestNotNull(TEXT("Player State spawned"), State))
			{
				return;
			}

			UFoundryInventorySubsystem* Inventories = UFoundryInventorySubsystem::Get(Character);
			if (!TestNotNull(TEXT("Inventory Subsystem is available"), Inventories))
			{
				return;
			}

			const FInventoryHandle Inventory = Inventories->RegisterInventoryOwner(State, FInventoryInitializationData());
			UGASPTestPresenceComponent* Presence = NewObject<UGASPTestPresenceComponent>(Character);
			Presence->SetAutomaticAssociation(false);
			Presence->RegisterComponent();
			Character->SetExternalAbilitySystemOwner(State);

			TestFalse(TEXT("Automatic association remains disabled"), Presence->IsPresenceInitialized());

			const bool bInitializedExplicitly = Presence->InitializePresence(Inventory);

			TestTrue(TEXT("Explicit association still works"), bInitializedExplicitly);

			Presence->DestroyComponent();
		});
	});

	Describe("Crucible Pawn Integration", [this]()
	{
		It("Should inherit lazy ASC behavior and keep task execution independent of ASC placement", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			TestTrue(TEXT("GASP derives from Crucible pawn"), Character->IsA<ACruciblePawn>());
			TestNull(TEXT("No ASC eagerly created"), Character->FindAbilitySystemComponent());
			TestNotNull(TEXT("Pawn task runner exists"), Character->GetPawnGameplayTasksComponent());
			TestNull(TEXT("GASP opts out of competing physical animation"), Character->GetPhysicalAnimationComponent());

			AGASPTestPlayerState* State = Fixture->GetWorld()->SpawnActor<AGASPTestPlayerState>();
			if (!TestNotNull(TEXT("Player State spawned"), State))
			{
				return;
			}

			Character->SetExternalAbilitySystemOwner(State);

			TestEqual(TEXT("Player State ASC adopted"), Character->FindAbilitySystemComponent(), State->GetAbilitySystemComponent());
			TestEqual(TEXT("Avatar is GASP pawn"), State->GetAbilitySystemComponent()->GetAvatarActor(), static_cast<AActor*>(Character));

			UGameplayTasksComponent* Tasks = Character->GetPawnGameplayTasksComponent();
			Character->UnPossessed();

			TestNull(TEXT("External ASC released on unpossession"), Character->FindAbilitySystemComponent());
			TestNull(TEXT("External avatar cleared"), State->GetAbilitySystemComponent()->GetAvatarActor());
			TestEqual(TEXT("Task runner survives handoff"), Character->GetPawnGameplayTasksComponent(), Tasks);
		});

		It("Should stop Mover input when Crucible death begins", [this]()
		{
			if (!bScenarioReady)
			{
				return;
			}

			Character->AddMovementInput(FVector::ForwardVector);
			Character->StartDeath();
			FMoverInputCmdContext Input;
			Character->ProduceInput_Implementation(0, Input);
			const FGASPMoverInputs* Result = Input.InputCollection.FindDataByType<FGASPMoverInputs>();

			TestTrue(TEXT("Death disables pawn movement"), Character->IsMovementDisabledByDeath());
			TestTrue(TEXT("Death produces no movement intent"), Result && Result->GetMoveInput().IsNearlyZero());
			TestTrue(TEXT("Death flushes buffered movement"), Character->GetPendingMovementInputVector().IsNearlyZero());
			TestTrue(TEXT("Death suppresses jump intent"), Result && !Result->bIsJumpJustPressed);
		});
	});
}

#endif
