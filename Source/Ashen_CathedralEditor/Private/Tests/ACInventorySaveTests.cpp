// 슬롯별 인벤토리 저장/복원 규칙을 검증한다 — 실제 세이브 파일을 건드리지 않는 것이 이 스위트의 첫 번째 계약이다

#include "Misc/AutomationTest.h"

#include "DataAssets/Items/ACItemDefinition.h"
#include "Engine/GameInstance.h"
#include "GameplayTags/ACGameplayTags_Item.h"
#include "GameplayTags/ACGameplayTags_MetaProgression.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "SaveGame/ACSaveGame_MetaProgression.h"
#include "SaveGame/ACSavedInventoryEntry.h"
#include "Subsystems/ACInventorySubsystem.h"
#include "Subsystems/ACMetaProgressionSubsystem.h"
#include "Subsystems/ACWeaponSelectionSubsystem.h"
#include "Tests/ACInventoryTestListener.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"

#if WITH_AUTOMATION_TESTS

namespace ACInventorySaveTests
{
	// 모든 테스트 슬롯 이름에 강제로 붙는 접두사. 실제 MetaProgressionSave_Slot_* 파일과 절대 겹치지 않게 한다
	static const TCHAR* TestSlotPrefix = TEXT("AutoTest_");

	// 실제 슬롯 번호와 겹치지 않도록 예약한 대역. 접두사가 어떤 이유로 빠지더라도 실제 세이브를 덮지 않는다
	static constexpr int32 SlotA = 9000;
	static constexpr int32 SlotB = 9001;
	static constexpr int32 SlotC = 9002;

	static constexpr int32 SaveUserIndex = 0;

	static FString MakeTestSlotName(int32 SlotIndex)
	{
		return FString::Printf(TEXT("%sMetaProgressionSave_Slot_%d"), TestSlotPrefix, SlotIndex);
	}

	static FString MakeRealSlotName(int32 SlotIndex)
	{
		return FString::Printf(TEXT("MetaProgressionSave_Slot_%d"), SlotIndex);
	}

	// UGameplayStatics의 기본 세이브 시스템이 쓰는 실제 파일 경로
	static FString MakeTestSlotFilePath(int32 SlotIndex)
	{
		return FPaths::ProjectSavedDir() / TEXT("SaveGames") / (MakeTestSlotName(SlotIndex) + TEXT(".sav"));
	}

	/**
	 * GameInstance::Init()은 부르지 않는다 — 전역 델리게이트를 바인딩하고 다른 서브시스템까지 전부 깨우기 때문이다.
	 * 대신 두 서브시스템을 직접 만들고 InitializeForTests로 서로 연결한다.
	 * 소멸자에서 접두사 override를 되돌리고 이번 테스트가 만든 슬롯 파일을 전부 지운다.
	 */
	struct FInventorySaveFixture
	{
		UGameInstance* GameInstance = nullptr;
		UACMetaProgressionSubsystem* MetaProgression = nullptr;
		UACInventorySubsystem* Inventory = nullptr;

		// 테스트가 만든 임시 ItemDefinition. AssetManager가 모르는 객체라 리졸버 훅으로 해석시킨다
		TMap<FPrimaryAssetId, UACItemDefinition*> DefinitionRegistry;
		TArray<UACItemDefinition*> OwnedDefinitions;
		TArray<UACInventoryTestListener*> OwnedListeners;

		FInventorySaveFixture()
		{
			UACMetaProgressionSubsystem::SetSlotNamePrefixOverrideForTests(TestSlotPrefix);
			UACInventorySubsystem::SetItemDefinitionResolverForTests([this](const FPrimaryAssetId& ItemDefinitionId) -> UACItemDefinition*
			{
				UACItemDefinition* const* Found = DefinitionRegistry.Find(ItemDefinitionId);
				return Found ? *Found : nullptr;
			});

			CreateSubsystems();
		}

		~FInventorySaveFixture()
		{
			// 접두사가 살아 있는 동안 지워야 테스트 파일만 정확히 지워진다
			for (const int32 SlotIndex : { SlotA, SlotB, SlotC })
			{
				UGameplayStatics::DeleteGameInSlot(MakeTestSlotName(SlotIndex), SaveUserIndex);
			}

			UACInventorySubsystem::SetItemDefinitionResolverForTests(nullptr);
			UACMetaProgressionSubsystem::SetSlotNamePrefixOverrideForTests(FString());
			// 훅이 켜진 채 남으면 뒤따르는 테스트가 전부 저장에 실패한다
			UACMetaProgressionSubsystem::SetForceSaveFailureForTests(false);

			for (UACInventoryTestListener* Listener : OwnedListeners)
			{
				if (Listener)
				{
					Listener->RemoveFromRoot();
				}
			}

			DestroySubsystems();

			for (UACItemDefinition* Definition : OwnedDefinitions)
			{
				if (Definition)
				{
					Definition->RemoveFromRoot();
				}
			}
		}

		// 테스트 5용 — 게임을 껐다 켠 것과 같은 상태를 만든다. 정의 레지스트리는 에셋에 해당하므로 유지한다
		void RecreateGameInstance()
		{
			DestroySubsystems();
			CreateSubsystems();
		}

		// 인벤토리 델리게이트를 구독하는 리스너를 만들어 붙인다
		UACInventoryTestListener* MakeSubscribedListener()
		{
			UACInventoryTestListener* Listener = NewObject<UACInventoryTestListener>(GetTransientPackage());
			Listener->AddToRoot();
			Listener->ObservedInventory = Inventory;

			Inventory->OnInventoryChangedDelegate.AddDynamic(Listener, &UACInventoryTestListener::HandleInventoryChanged);
			Inventory->OnEquippedItemChangedDelegate.AddDynamic(Listener, &UACInventoryTestListener::HandleEquippedItemChanged);

			OwnedListeners.Add(Listener);
			return Listener;
		}

		UACItemDefinition* MakeDefinition(const TCHAR* AssetName, int32 MaxStackCount, FGameplayTag CategoryTag)
		{
			UACItemDefinition* Definition = NewObject<UACItemDefinition>(GetTransientPackage(), FName(AssetName));
			Definition->AddToRoot();
			Definition->MaxStackCount = MaxStackCount;
			Definition->ItemCategoryTag = CategoryTag;

			OwnedDefinitions.Add(Definition);
			DefinitionRegistry.Add(Definition->GetPrimaryAssetId(), Definition);
			return Definition;
		}

		// 버전 1 빌드가 남겼을 파일을 그대로 재현한다. SaveVersion은 CDO 기본값과 같아 파일에 기록되지 않는다
		void WriteVersion1File(int32 SlotIndex, const TMap<FGameplayTag, int32>& Currencies, const FGameplayTagContainer& ClearedBosses) const
		{
			UACSaveGame_MetaProgression* LegacySave = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::CreateSaveGameObject(UACSaveGame_MetaProgression::StaticClass()));
			LegacySave->CurrencyAmounts = Currencies;
			LegacySave->ClearedBossTags = ClearedBosses;
			LegacySave->SaveVersion = 1;

			UGameplayStatics::SaveGameToSlot(LegacySave, MakeTestSlotName(SlotIndex), SaveUserIndex);
		}

		// 저장 파일을 직접 만들어 손상된/해석 불가능한 항목이 섞인 상황을 재현한다
		void WriteSaveWithEntries(int32 SlotIndex, const TArray<FACSavedInventoryEntry>& InEntries) const
		{
			UACSaveGame_MetaProgression* RawSave = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::CreateSaveGameObject(UACSaveGame_MetaProgression::StaticClass()));
			RawSave->InventoryEntries = InEntries;
			RawSave->SaveVersion = UACSaveGame_MetaProgression::CurrentSaveVersion;

			UGameplayStatics::SaveGameToSlot(RawSave, MakeTestSlotName(SlotIndex), SaveUserIndex);
		}

	private:
		void CreateSubsystems()
		{
			GameInstance = NewObject<UGameInstance>(GetTransientPackage());
			GameInstance->AddToRoot();

			MetaProgression = NewObject<UACMetaProgressionSubsystem>(GameInstance);
			MetaProgression->AddToRoot();
			MetaProgression->InitializeForTests();

			Inventory = NewObject<UACInventorySubsystem>(GameInstance);
			Inventory->AddToRoot();
			Inventory->InitializeForTests(MetaProgression);
		}

		void DestroySubsystems()
		{
			if (Inventory)
			{
				Inventory->RemoveFromRoot();
				Inventory = nullptr;
			}
			if (MetaProgression)
			{
				MetaProgression->RemoveFromRoot();
				MetaProgression = nullptr;
			}
			if (GameInstance)
			{
				GameInstance->RemoveFromRoot();
				GameInstance = nullptr;
			}
		}
	};

	// 세이브 폴더에서 테스트 접두사가 붙지 않은 파일들의 (크기, 수정시각) 스냅샷
	static TMap<FString, TPair<int64, FDateTime>> SnapshotForeignSaveFiles()
	{
		TMap<FString, TPair<int64, FDateTime>> Snapshot;

		const FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("SaveGames");
		TArray<FString> FoundFiles;
		IFileManager::Get().FindFiles(FoundFiles, *(SaveDirectory / TEXT("*.sav")), true, false);

		for (const FString& FileName : FoundFiles)
		{
			if (FileName.StartsWith(TestSlotPrefix))
			{
				continue;
			}

			const FString FullPath = SaveDirectory / FileName;
			Snapshot.Add(FileName, TPair<int64, FDateTime>(IFileManager::Get().FileSize(*FullPath), IFileManager::Get().GetTimeStamp(*FullPath)));
		}

		return Snapshot;
	}
}

using namespace ACInventorySaveTests;

#pragma region 슬롯 격리

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventorySlotIsolationTest,
	"AshenCathedral.Inventory.SlotIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventorySlotIsolationTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	UACItemDefinition* ItemA = Fixture.MakeDefinition(TEXT("DA_Item_TestA"), 10, ACGameplayTags::Item_Category_Material);
	UACItemDefinition* ItemB = Fixture.MakeDefinition(TEXT("DA_Item_TestB"), 10, ACGameplayTags::Item_Category_Consumable);

	// 1) 슬롯 A에 Item A 2개
	Fixture.MetaProgression->LoadSlot(SlotA);
	TestEqual(TEXT("슬롯 A에 Item A 2개가 들어간다"), Fixture.Inventory->AddItem(ItemA, 2), 2);

	// 2) 슬롯 B에 Item B 1개
	Fixture.MetaProgression->LoadSlot(SlotB);
	TestEqual(TEXT("슬롯 B는 비어 있는 상태로 시작한다"), Fixture.Inventory->GetEntries().Num(), 0);
	TestEqual(TEXT("슬롯 B에 Item B 1개가 들어간다"), Fixture.Inventory->AddItem(ItemB, 1), 1);

	// 3) 슬롯 A 재로드 시 A만 존재
	Fixture.MetaProgression->LoadSlot(SlotA);
	TestEqual(TEXT("슬롯 A에는 Item A가 2개 남아 있다"), Fixture.Inventory->GetTotalItemCount(ItemA), 2);
	TestEqual(TEXT("슬롯 A에는 Item B가 없다"), Fixture.Inventory->GetTotalItemCount(ItemB), 0);

	// 4) 슬롯 B 재로드 시 B만 존재
	Fixture.MetaProgression->LoadSlot(SlotB);
	TestEqual(TEXT("슬롯 B에는 Item B가 1개 남아 있다"), Fixture.Inventory->GetTotalItemCount(ItemB), 1);
	TestEqual(TEXT("슬롯 B에는 Item A가 없다"), Fixture.Inventory->GetTotalItemCount(ItemA), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventorySurvivesGameInstanceRecreateTest,
	"AshenCathedral.Inventory.SurvivesGameInstanceRecreate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventorySurvivesGameInstanceRecreateTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	UACItemDefinition* ItemA = Fixture.MakeDefinition(TEXT("DA_Item_RecreateA"), 10, ACGameplayTags::Item_Category_Material);
	UACItemDefinition* ItemB = Fixture.MakeDefinition(TEXT("DA_Item_RecreateB"), 10, ACGameplayTags::Item_Category_Consumable);

	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.Inventory->AddItem(ItemA, 2);
	Fixture.Inventory->EquipItem(ItemA);

	Fixture.MetaProgression->LoadSlot(SlotB);
	Fixture.Inventory->AddItem(ItemB, 1);

	// 5) GameInstance와 서브시스템을 통째로 새로 만들어도 슬롯별 내용이 같아야 한다
	Fixture.RecreateGameInstance();

	Fixture.MetaProgression->LoadSlot(SlotA);
	TestEqual(TEXT("재생성 후에도 슬롯 A에는 Item A가 2개"), Fixture.Inventory->GetTotalItemCount(ItemA), 2);
	TestEqual(TEXT("재생성 후에도 슬롯 A에는 Item B가 없다"), Fixture.Inventory->GetTotalItemCount(ItemB), 0);
	TestTrue(TEXT("장착 기록도 슬롯과 함께 복원된다"), Fixture.Inventory->GetEquippedItemDefinition() == ItemA);

	Fixture.MetaProgression->LoadSlot(SlotB);
	TestEqual(TEXT("재생성 후에도 슬롯 B에는 Item B가 1개"), Fixture.Inventory->GetTotalItemCount(ItemB), 1);
	TestEqual(TEXT("재생성 후에도 슬롯 B에는 Item A가 없다"), Fixture.Inventory->GetTotalItemCount(ItemA), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryDeleteSlotKeepsOtherSlotTest,
	"AshenCathedral.Inventory.DeleteSlotKeepsOtherSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryDeleteSlotKeepsOtherSlotTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	UACItemDefinition* ItemA = Fixture.MakeDefinition(TEXT("DA_Item_DeleteA"), 10, ACGameplayTags::Item_Category_Material);
	UACItemDefinition* ItemB = Fixture.MakeDefinition(TEXT("DA_Item_DeleteB"), 10, ACGameplayTags::Item_Category_Consumable);

	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.Inventory->AddItem(ItemA, 2);
	Fixture.MetaProgression->AddCurrency(ACGameplayTags::MetaProgression_Currency_AshSoul, 50);

	Fixture.MetaProgression->LoadSlot(SlotB);
	Fixture.Inventory->AddItem(ItemB, 1);

	// 6) 활성 슬롯인 B를 지우면 슬롯 미선택 상태로 돌아가고, A는 그대로다
	Fixture.MetaProgression->DeleteSlot(SlotB);
	TestFalse(TEXT("활성 슬롯을 지우면 슬롯 미선택 상태가 된다"), Fixture.MetaProgression->HasActiveSlot());
	TestEqual(TEXT("삭제 직후 슬롯 번호는 INDEX_NONE"), Fixture.MetaProgression->GetActiveSlotIndex(), (int32)INDEX_NONE);
	TestEqual(TEXT("삭제 직후 인벤토리는 비어 있다"), Fixture.Inventory->GetEntries().Num(), 0);

	Fixture.MetaProgression->LoadSlot(SlotA);
	TestEqual(TEXT("슬롯 B를 지워도 슬롯 A의 아이템은 남는다"), Fixture.Inventory->GetTotalItemCount(ItemA), 2);
	TestEqual(TEXT("슬롯 B를 지워도 슬롯 A의 재화는 남는다"), Fixture.MetaProgression->GetCurrencyAmount(ACGameplayTags::MetaProgression_Currency_AshSoul), 50);

	// 7) 지운 슬롯 B로 새 게임을 시작하면 이전 아이템이 보이지 않는다
	Fixture.MetaProgression->LoadSlot(SlotB);
	TestEqual(TEXT("새 게임 슬롯 B에는 이전 Item B가 없다"), Fixture.Inventory->GetTotalItemCount(ItemB), 0);
	TestEqual(TEXT("새 게임 슬롯 B는 완전히 비어 있다"), Fixture.Inventory->GetEntries().Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryDeletedSlotIsNotResurrectedTest,
	"AshenCathedral.Inventory.DeletedSlotIsNotResurrected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryDeletedSlotIsNotResurrectedTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	// 슬롯을 지운 뒤의 변경은 갈 곳이 없으므로 경고가 난다 — 의도된 경로다
	AddExpectedMessagePlain(TEXT("활성 슬롯이 없어 저장을 건너뜁니다"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	UACItemDefinition* ItemA = Fixture.MakeDefinition(TEXT("DA_Item_ResurrectA"), 10, ACGameplayTags::Item_Category_Material);

	Fixture.MetaProgression->LoadSlot(SlotC);
	Fixture.Inventory->AddItem(ItemA, 3);
	Fixture.MetaProgression->DeleteSlot(SlotC);

	// 삭제 후의 변경이 flush로 되살아나면 안 된다
	Fixture.Inventory->AddItem(ItemA, 5);
	Fixture.MetaProgression->AddCurrency(ACGameplayTags::MetaProgression_Currency_AshSoul, 999);

	TestFalse(TEXT("삭제한 슬롯 파일이 다시 생기지 않았다"), UGameplayStatics::DoesSaveGameExist(MakeTestSlotName(SlotC), SaveUserIndex));

	Fixture.MetaProgression->LoadSlot(SlotC);
	TestEqual(TEXT("다시 로드해도 슬롯 C는 비어 있다"), Fixture.Inventory->GetTotalItemCount(ItemA), 0);
	TestEqual(TEXT("삭제 후의 재화 변경도 슬롯에 남지 않았다"), Fixture.MetaProgression->GetCurrencyAmount(ACGameplayTags::MetaProgression_Currency_AshSoul), 0);

	return true;
}

#pragma endregion

#pragma region 슬롯 미선택 / 실제 세이브 보호

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventorySlotlessDoesNotWriteFilesTest,
	"AshenCathedral.Inventory.SlotlessDoesNotWriteFiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventorySlotlessDoesNotWriteFilesTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	AddExpectedMessagePlain(TEXT("활성 슬롯이 없어 저장을 건너뜁니다"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	UACItemDefinition* ItemA = Fixture.MakeDefinition(TEXT("DA_Item_SlotlessA"), 10, ACGameplayTags::Item_Category_Material);

	// 8) 슬롯을 고르기 전에는 아무 슬롯 파일도 만들지 않는다
	TestFalse(TEXT("초기 상태는 슬롯 미선택이다"), Fixture.MetaProgression->HasActiveSlot());
	TestEqual(TEXT("초기 슬롯 번호는 INDEX_NONE"), Fixture.MetaProgression->GetActiveSlotIndex(), (int32)INDEX_NONE);

	TestEqual(TEXT("슬롯이 없어도 메모리 인벤토리는 동작한다"), Fixture.Inventory->AddItem(ItemA, 4), 4);
	Fixture.MetaProgression->AddCurrency(ACGameplayTags::MetaProgression_Currency_AshSoul, 123);
	TestEqual(TEXT("슬롯이 없어도 메모리 재화는 동작한다"), Fixture.MetaProgression->GetCurrencyAmount(ACGameplayTags::MetaProgression_Currency_AshSoul), 123);

	TestFalse(TEXT("슬롯 0 파일이 만들어지지 않았다"), UGameplayStatics::DoesSaveGameExist(MakeTestSlotName(0), SaveUserIndex));
	TestFalse(TEXT("접두사 없는 실제 슬롯 0 파일도 만들어지지 않았다"), UGameplayStatics::DoesSaveGameExist(MakeRealSlotName(0), SaveUserIndex));

	// 슬롯을 고르면 슬롯 없이 쌓인 데이터는 그 슬롯으로 옮겨가지 않고 버려진다
	Fixture.MetaProgression->LoadSlot(SlotA);
	TestEqual(TEXT("슬롯 미선택 상태의 아이템이 슬롯으로 새어 들어가지 않는다"), Fixture.Inventory->GetTotalItemCount(ItemA), 0);
	TestEqual(TEXT("슬롯 미선택 상태의 재화도 새어 들어가지 않는다"), Fixture.MetaProgression->GetCurrencyAmount(ACGameplayTags::MetaProgression_Currency_AshSoul), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryNeverTouchesRealSaveFilesTest,
	"AshenCathedral.Inventory.NeverTouchesRealSaveFiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryNeverTouchesRealSaveFilesTest::RunTest(const FString& Parameters)
{
	// 11 / 12) 디버그 경로(여기서는 접두사 override, 실제 게임에서는 PIE 샌드박스)가
	// 실제 메타 세이브와 UGT 진행 세이브를 한 바이트도 건드리지 않는지 확인한다
	const TMap<FString, TPair<int64, FDateTime>> Before = SnapshotForeignSaveFiles();

	{
		FInventorySaveFixture Fixture;

		UACItemDefinition* ItemA = Fixture.MakeDefinition(TEXT("DA_Item_UntouchedA"), 10, ACGameplayTags::Item_Category_Material);

		Fixture.MetaProgression->LoadSlot(SlotA);
		Fixture.Inventory->AddItem(ItemA, 3);
		Fixture.Inventory->EquipItem(ItemA);
		Fixture.MetaProgression->AddCurrency(ACGameplayTags::MetaProgression_Currency_RelicFragment, 7);
		Fixture.MetaProgression->DeleteSlot(SlotA);

		TestTrue(TEXT("테스트가 쓰는 슬롯 이름에는 테스트 접두사가 붙어 있다"), MakeTestSlotName(SlotA).StartsWith(TestSlotPrefix));
	}

	const TMap<FString, TPair<int64, FDateTime>> After = SnapshotForeignSaveFiles();

	TestEqual(TEXT("테스트 외 세이브 파일 개수가 변하지 않았다"), After.Num(), Before.Num());

	for (const TPair<FString, TPair<int64, FDateTime>>& BeforeEntry : Before)
	{
		const TPair<int64, FDateTime>* AfterEntry = After.Find(BeforeEntry.Key);
		if (!AfterEntry)
		{
			AddError(FString::Printf(TEXT("세이브 파일 %s가 사라졌습니다."), *BeforeEntry.Key));
			continue;
		}

		TestEqual(*FString::Printf(TEXT("%s의 크기가 그대로다"), *BeforeEntry.Key), AfterEntry->Key, BeforeEntry.Value.Key);
		TestTrue(*FString::Printf(TEXT("%s의 수정 시각이 그대로다"), *BeforeEntry.Key), AfterEntry->Value == BeforeEntry.Value.Value);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryRejectsNegativeSlotTest,
	"AshenCathedral.Inventory.RejectsNegativeSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryRejectsNegativeSlotTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	AddExpectedMessagePlain(TEXT("유효하지 않은 슬롯 번호"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	Fixture.MetaProgression->LoadSlot(-1);
	TestFalse(TEXT("음수 슬롯 로드는 거부된다"), Fixture.MetaProgression->HasActiveSlot());

	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.MetaProgression->DeleteSlot(-5);
	TestTrue(TEXT("음수 슬롯 삭제는 활성 슬롯에 영향을 주지 않는다"), Fixture.MetaProgression->HasActiveSlot());
	TestEqual(TEXT("음수 슬롯 삭제 후에도 활성 슬롯 번호가 그대로다"), Fixture.MetaProgression->GetActiveSlotIndex(), SlotA);

	return true;
}

#pragma endregion

#pragma region 마이그레이션

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryVersion1FileMigratesTest,
	"AshenCathedral.Inventory.Version1FileMigrates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryVersion1FileMigratesTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	// 기본값이 1이어야 버전 1 파일에 SaveVersion이 기록되지 않는다 — 마이그레이션 판정의 전제다
	TestEqual(TEXT("SaveGame CDO의 SaveVersion 기본값은 1이다"), GetDefault<UACSaveGame_MetaProgression>()->SaveVersion, 1);

	TMap<FGameplayTag, int32> LegacyCurrencies;
	LegacyCurrencies.Add(ACGameplayTags::MetaProgression_Currency_AshSoul, 120);
	LegacyCurrencies.Add(ACGameplayTags::MetaProgression_Currency_RelicFragment, 4);

	FGameplayTagContainer LegacyBosses;
	LegacyBosses.AddTag(ACGameplayTags::MetaProgression_BossID_AshenKnight);

	Fixture.WriteVersion1File(SlotA, LegacyCurrencies, LegacyBosses);

	// 9) 버전 1 파일을 열면 재화와 보스 기록이 그대로 유지되고 인벤토리는 빈 배열이다
	Fixture.MetaProgression->LoadSlot(SlotA);

	TestEqual(TEXT("버전 1의 AshSoul이 유지된다"), Fixture.MetaProgression->GetCurrencyAmount(ACGameplayTags::MetaProgression_Currency_AshSoul), 120);
	TestEqual(TEXT("버전 1의 RelicFragment가 유지된다"), Fixture.MetaProgression->GetCurrencyAmount(ACGameplayTags::MetaProgression_Currency_RelicFragment), 4);
	TestTrue(TEXT("버전 1의 보스 클리어 기록이 유지된다"), Fixture.MetaProgression->IsBossCleared(ACGameplayTags::MetaProgression_BossID_AshenKnight));
	TestEqual(TEXT("버전 1 파일에는 인벤토리가 없으므로 빈 배열이다"), Fixture.Inventory->GetEntries().Num(), 0);
	TestFalse(TEXT("장착 기록도 비어 있다"), Fixture.Inventory->GetEquippedItemDefinitionId().IsValid());

	// 마이그레이션 결과가 디스크에 반영되는지 — 변경이 한 번 생기면 버전 2로 기록된다
	UACItemDefinition* ItemA = Fixture.MakeDefinition(TEXT("DA_Item_MigrateA"), 10, ACGameplayTags::Item_Category_Material);
	Fixture.Inventory->AddItem(ItemA, 1);

	const UACSaveGame_MetaProgression* Reloaded = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::LoadGameFromSlot(MakeTestSlotName(SlotA), SaveUserIndex));
	if (!Reloaded)
	{
		AddError(TEXT("마이그레이션 후 저장된 파일을 다시 읽지 못했습니다."));
		return false;
	}

	TestEqual(TEXT("저장된 파일의 버전이 2로 올라갔다"), Reloaded->SaveVersion, UACSaveGame_MetaProgression::CurrentSaveVersion);
	TestEqual(TEXT("마이그레이션 후에도 재화가 유지된다"), Reloaded->CurrencyAmounts.FindRef(ACGameplayTags::MetaProgression_Currency_AshSoul), 120);
	TestTrue(TEXT("마이그레이션 후에도 보스 기록이 유지된다"), Reloaded->ClearedBossTags.HasTagExact(ACGameplayTags::MetaProgression_BossID_AshenKnight));
	TestEqual(TEXT("마이그레이션 후 인벤토리 항목이 기록된다"), Reloaded->InventoryEntries.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryInvalidDefinitionSkippedTest,
	"AshenCathedral.Inventory.InvalidDefinitionSkipped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryInvalidDefinitionSkippedTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	AddExpectedMessagePlain(TEXT("해석할 수 없는 인벤토리 항목"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	UACItemDefinition* GoodItem = Fixture.MakeDefinition(TEXT("DA_Item_GoodA"), 10, ACGameplayTags::Item_Category_Material);

	// 존재하지 않는 아이템 정의
	FACSavedInventoryEntry MissingEntry;
	MissingEntry.ItemDefinitionId = FPrimaryAssetId(UACItemDefinition::ItemDefinitionAssetType, TEXT("DA_Item_DoesNotExist"));
	MissingEntry.StackCount = 3;
	MissingEntry.InstanceId = FGuid::NewGuid();

	// 수량이 0이라 무효한 항목
	FACSavedInventoryEntry ZeroEntry;
	ZeroEntry.ItemDefinitionId = GoodItem->GetPrimaryAssetId();
	ZeroEntry.StackCount = 0;
	ZeroEntry.InstanceId = FGuid::NewGuid();

	// 정상 항목
	FACSavedInventoryEntry GoodEntry;
	GoodEntry.ItemDefinitionId = GoodItem->GetPrimaryAssetId();
	GoodEntry.StackCount = 2;
	GoodEntry.InstanceId = FGuid::NewGuid();
	GoodEntry.UpgradeLevel = 3;

	Fixture.WriteSaveWithEntries(SlotA, { MissingEntry, ZeroEntry, GoodEntry });

	// 10) 잘못된 항목만 건너뛰고 나머지는 복원한다
	Fixture.MetaProgression->LoadSlot(SlotA);

	TestEqual(TEXT("복원된 항목은 정상 항목 하나뿐이다"), Fixture.Inventory->GetEntries().Num(), 1);
	TestEqual(TEXT("정상 항목의 수량이 복원된다"), Fixture.Inventory->GetTotalItemCount(GoodItem), 2);

	const FACInventoryEntry* RestoredEntry = Fixture.Inventory->FindEntryByInstanceId(GoodEntry.InstanceId);
	if (!RestoredEntry)
	{
		AddError(TEXT("정상 항목이 InstanceId로 복원되지 않았습니다."));
		return false;
	}

	TestEqual(TEXT("강화 단계도 함께 복원된다"), RestoredEntry->UpgradeLevel, 3);

	// 해석하지 못한 항목은 저장 데이터에 남아 있어야 한다 — 에셋이 잠시 빠졌을 때 다음 저장 한 번으로 사라지면 안 된다
	Fixture.Inventory->AddItem(GoodItem, 1);

	const UACSaveGame_MetaProgression* Reloaded = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::LoadGameFromSlot(MakeTestSlotName(SlotA), SaveUserIndex));
	if (!Reloaded)
	{
		AddError(TEXT("재저장한 파일을 다시 읽지 못했습니다."));
		return false;
	}

	const bool bMissingEntryPreserved = Reloaded->InventoryEntries.ContainsByPredicate([&MissingEntry](const FACSavedInventoryEntry& Entry)
	{
		return Entry.ItemDefinitionId == MissingEntry.ItemDefinitionId && Entry.StackCount == 3;
	});
	TestTrue(TEXT("해석하지 못한 항목이 저장 데이터에 보존된다"), bMissingEntryPreserved);

	const bool bZeroEntryDropped = !Reloaded->InventoryEntries.ContainsByPredicate([](const FACSavedInventoryEntry& Entry)
	{
		return Entry.StackCount <= 0;
	});
	TestTrue(TEXT("수량이 0인 손상 항목은 버려진다"), bZeroEntryDropped);

	return true;
}

#pragma endregion

#pragma region 인벤토리 규칙

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryStackRulesTest,
	"AshenCathedral.Inventory.StackRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryStackRulesTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	UACItemDefinition* StackableItem = Fixture.MakeDefinition(TEXT("DA_Item_Stack5"), 5, ACGameplayTags::Item_Category_Consumable);
	UACItemDefinition* SingleItem = Fixture.MakeDefinition(TEXT("DA_Item_NoStack"), 1, ACGameplayTags::Item_Category_Weapon);

	Fixture.MetaProgression->LoadSlot(SlotA);

	TestEqual(TEXT("상한 5짜리 아이템 7개는 두 항목으로 나뉜다"), Fixture.Inventory->AddItem(StackableItem, 7), 7);
	TestEqual(TEXT("항목 수는 2개"), Fixture.Inventory->GetEntries().Num(), 2);
	TestEqual(TEXT("총 보유량은 7"), Fixture.Inventory->GetTotalItemCount(StackableItem), 7);

	TestEqual(TEXT("스택되지 않는 아이템 3개는 항목 3개가 된다"), Fixture.Inventory->AddItem(SingleItem, 3), 3);
	TestEqual(TEXT("스택되지 않는 아이템의 총 보유량은 3"), Fixture.Inventory->GetTotalItemCount(SingleItem), 3);

	TestEqual(TEXT("보유량보다 많이 제거하면 있는 만큼만 제거된다"), Fixture.Inventory->RemoveItem(StackableItem, 100), 7);
	TestEqual(TEXT("전부 제거되면 항목이 사라진다"), Fixture.Inventory->GetTotalItemCount(StackableItem), 0);

	TestEqual(TEXT("nullptr 추가는 0을 반환한다"), Fixture.Inventory->AddItem(nullptr, 5), 0);
	TestEqual(TEXT("0개 추가는 0을 반환한다"), Fixture.Inventory->AddItem(StackableItem, 0), 0);
	TestEqual(TEXT("음수 추가는 0을 반환한다"), Fixture.Inventory->AddItem(StackableItem, -3), 0);

	// 분류 태그 필터
	const TArray<FACInventoryEntry> WeaponEntries = Fixture.Inventory->GetEntriesByCategory(ACGameplayTags::Item_Category_Weapon);
	TestEqual(TEXT("무기 분류 필터는 무기 항목만 돌려준다"), WeaponEntries.Num(), 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryEquipRulesTest,
	"AshenCathedral.Inventory.EquipRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryEquipRulesTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	AddExpectedMessagePlain(TEXT("보유하지 않은 아이템"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	UACItemDefinition* OwnedItem = Fixture.MakeDefinition(TEXT("DA_Item_EquipOwned"), 1, ACGameplayTags::Item_Category_Weapon);
	UACItemDefinition* UnownedItem = Fixture.MakeDefinition(TEXT("DA_Item_EquipUnowned"), 1, ACGameplayTags::Item_Category_Weapon);

	Fixture.MetaProgression->LoadSlot(SlotA);

	TestFalse(TEXT("보유하지 않은 아이템은 장착할 수 없다"), Fixture.Inventory->EquipItem(UnownedItem));
	TestFalse(TEXT("거부된 장착은 기록되지 않는다"), Fixture.Inventory->GetEquippedItemDefinitionId().IsValid());

	Fixture.Inventory->AddItem(OwnedItem, 1);
	TestTrue(TEXT("보유한 아이템은 장착된다"), Fixture.Inventory->EquipItem(OwnedItem));
	TestTrue(TEXT("장착 아이템을 조회할 수 있다"), Fixture.Inventory->GetEquippedItemDefinition() == OwnedItem);

	// 장착 중인 아이템을 전부 잃으면 장착 기록도 사라져야 한다
	Fixture.Inventory->RemoveItem(OwnedItem, 1);
	TestFalse(TEXT("마지막 하나를 잃으면 장착 기록이 지워진다"), Fixture.Inventory->GetEquippedItemDefinitionId().IsValid());

	// 장착 기록은 무기 선택 서브시스템을 자동으로 건드리지 않는다 (1차 범위 밖)
	UACWeaponSelectionSubsystem* WeaponSelection = NewObject<UACWeaponSelectionSubsystem>(Fixture.GameInstance);
	TestNull(TEXT("장착 복원은 무기 선택 서브시스템을 건드리지 않는다"), WeaponSelection->GetSelectedWeaponData());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryRoundTripPreservesEntryDataTest,
	"AshenCathedral.Inventory.RoundTripPreservesEntryData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryRoundTripPreservesEntryDataTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	UACItemDefinition* Item = Fixture.MakeDefinition(TEXT("DA_Item_RoundTrip"), 10, ACGameplayTags::Item_Category_Relic);

	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.Inventory->AddItem(Item, 4);

	const FGuid InstanceId = Fixture.Inventory->GetEntries()[0].InstanceId;
	TestTrue(TEXT("강화 단계를 설정할 수 있다"), Fixture.Inventory->SetUpgradeLevel(InstanceId, 5));
	TestTrue(TEXT("수치 스택을 추가할 수 있다"), Fixture.Inventory->AddStatTagStack(InstanceId, ACGameplayTags::Item_Stat_Durability, 42));

	// 다른 슬롯을 거쳐 돌아와야 메모리 캐시가 아니라 디스크에서 다시 읽는다
	Fixture.MetaProgression->LoadSlot(SlotB);
	Fixture.MetaProgression->LoadSlot(SlotA);

	const FACInventoryEntry* Restored = Fixture.Inventory->FindEntryByInstanceId(InstanceId);
	if (!Restored)
	{
		AddError(TEXT("InstanceId가 저장/복원을 통과하지 못했습니다 — FPrimaryAssetId 또는 FGuid 직렬화를 확인하세요."));
		return false;
	}

	TestEqual(TEXT("수량이 그대로다"), Restored->StackCount, 4);
	TestEqual(TEXT("강화 단계가 그대로다"), Restored->UpgradeLevel, 5);
	TestEqual(TEXT("수치 스택이 그대로다"), Restored->StatTagStacks.FindRef(ACGameplayTags::Item_Stat_Durability), 42);
	TestTrue(TEXT("정의가 다시 해석된다"), Restored->ItemDefinition.Get() == Item);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventorySameSlotReloadKeepsProgressTest,
	"AshenCathedral.Inventory.SameSlotReloadKeepsProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventorySameSlotReloadKeepsProgressTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	UACItemDefinition* Item = Fixture.MakeDefinition(TEXT("DA_Item_SameSlot"), 10, ACGameplayTags::Item_Category_Material);

	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.Inventory->AddItem(Item, 2);

	// BP_ACGameModeBase의 BeginPlay가 레벨을 옮길 때마다 같은 슬롯으로 동기화를 다시 부른다.
	// 여기서 디스크를 다시 읽으면 진행이 되감기므로 아무 일도 없어야 한다
	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.MetaProgression->LoadSlot(SlotA);

	TestEqual(TEXT("같은 슬롯 재동기화 후에도 아이템이 그대로다"), Fixture.Inventory->GetTotalItemCount(Item), 2);
	TestEqual(TEXT("같은 슬롯 재동기화 후에도 항목이 중복되지 않는다"), Fixture.Inventory->GetEntries().Num(), 1);

	return true;
}

#pragma endregion

#pragma region 회귀 결함 수정 검증

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventorySetStackCountZeroClearsEquipTest,
	"AshenCathedral.Inventory.SetStackCountZeroClearsEquip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventorySetStackCountZeroClearsEquipTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	UACItemDefinition* Item = Fixture.MakeDefinition(TEXT("DA_Item_ZeroEquip"), 5, ACGameplayTags::Item_Category_Weapon);

	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.Inventory->AddItem(Item, 1);
	TestTrue(TEXT("장착이 기록된다"), Fixture.Inventory->EquipItem(Item));

	const FGuid InstanceId = Fixture.Inventory->GetEntries()[0].InstanceId;

	UACInventoryTestListener* Listener = Fixture.MakeSubscribedListener();

	// 마지막 보유분을 SetStackCount(0)으로 없애면 RemoveItem으로 전부 없앤 것과 같아야 한다
	TestTrue(TEXT("수량을 0으로 지정할 수 있다"), Fixture.Inventory->SetStackCount(InstanceId, 0));

	TestEqual(TEXT("항목이 사라진다"), Fixture.Inventory->GetTotalItemCount(Item), 0);
	TestFalse(TEXT("장착 ID가 지워진다"), Fixture.Inventory->GetEquippedItemDefinitionId().IsValid());
	TestEqual(TEXT("장착 변경 통지는 한 번만 온다"), Listener->EquippedChangeCount, 1);

	// 재로드 후에도 비어 있어야 한다 — 다른 슬롯을 거쳐야 디스크에서 다시 읽는다
	Fixture.MetaProgression->LoadSlot(SlotB);
	Fixture.MetaProgression->LoadSlot(SlotA);

	TestFalse(TEXT("재로드 후에도 장착 ID가 비어 있다"), Fixture.Inventory->GetEquippedItemDefinitionId().IsValid());
	TestEqual(TEXT("재로드 후에도 아이템이 없다"), Fixture.Inventory->GetTotalItemCount(Item), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventorySlotSwitchNotifiesZeroTest,
	"AshenCathedral.Inventory.SlotSwitchNotifiesZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventorySlotSwitchNotifiesZeroTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	UACItemDefinition* ItemA = Fixture.MakeDefinition(TEXT("DA_Item_SwitchA"), 10, ACGameplayTags::Item_Category_Material);
	UACItemDefinition* ItemB = Fixture.MakeDefinition(TEXT("DA_Item_SwitchB"), 10, ACGameplayTags::Item_Category_Consumable);

	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.Inventory->AddItem(ItemA, 2);

	Fixture.MetaProgression->LoadSlot(SlotB);
	Fixture.Inventory->AddItem(ItemB, 1);

	// B가 활성인 상태에서 구독하고 A로 전환한다
	UACInventoryTestListener* Listener = Fixture.MakeSubscribedListener();
	Fixture.MetaProgression->LoadSlot(SlotA);

	TestEqual(TEXT("새 슬롯 아이템은 복원된 총수량을 받는다"), Listener->FindLastCountFor(ItemA), 2);
	TestEqual(TEXT("이전 슬롯에만 있던 아이템은 0을 받는다"), Listener->FindLastCountFor(ItemB), 0);

	// 통지 시점에 이미 새 슬롯 데이터가 보여야 한다 (A의 항목 1개)
	TestEqual(TEXT("통지 시점의 GetAllEntries()는 이미 새 슬롯 데이터다"), Listener->EntryCountAtLastNotify, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryUpgradedEntryNotMergedTest,
	"AshenCathedral.Inventory.UpgradedEntryNotMerged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryUpgradedEntryNotMergedTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	UACItemDefinition* Item = Fixture.MakeDefinition(TEXT("DA_Item_UpgradeMerge"), 5, ACGameplayTags::Item_Category_Relic);

	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.Inventory->AddItem(Item, 1);

	const FGuid UpgradedInstanceId = Fixture.Inventory->GetEntries()[0].InstanceId;
	TestTrue(TEXT("강화 단계를 올린다"), Fixture.Inventory->SetUpgradeLevel(UpgradedInstanceId, 3));
	TestTrue(TEXT("수치 스택도 붙인다"), Fixture.Inventory->AddStatTagStack(UpgradedInstanceId, ACGameplayTags::Item_Stat_Durability, 10));

	// 스택 여유(5-1=4)가 있어도 강화된 항목에는 합쳐지면 안 된다
	TestEqual(TEXT("일반 아이템 1개를 추가한다"), Fixture.Inventory->AddItem(Item, 1), 1);

	TestEqual(TEXT("총 보유량은 2"), Fixture.Inventory->GetTotalItemCount(Item), 2);
	TestEqual(TEXT("강화 항목과 별개로 새 항목이 생긴다"), Fixture.Inventory->GetEntries().Num(), 2);

	const FACInventoryEntry* UpgradedEntry = Fixture.Inventory->FindEntryByInstanceId(UpgradedInstanceId);
	if (!UpgradedEntry)
	{
		AddError(TEXT("강화된 항목을 찾지 못했습니다."));
		return false;
	}

	TestEqual(TEXT("강화 항목의 수량은 그대로 1"), UpgradedEntry->StackCount, 1);
	TestEqual(TEXT("강화 단계가 유지된다"), UpgradedEntry->UpgradeLevel, 3);

	// 새로 들어온 항목은 기본 상태여야 한다
	int32 PlainEntryCount = 0;
	for (const FACInventoryEntry& Entry : Fixture.Inventory->GetEntries())
	{
		if (Entry.InstanceId != UpgradedInstanceId)
		{
			++PlainEntryCount;
			TestEqual(TEXT("새 항목의 강화 단계는 0이다"), Entry.UpgradeLevel, 0);
			TestEqual(TEXT("새 항목에 수치 스택이 복제되지 않는다"), Entry.StatTagStacks.Num(), 0);
		}
	}
	TestEqual(TEXT("새 항목은 하나다"), PlainEntryCount, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryStackShrinkPreservesQuantityTest,
	"AshenCathedral.Inventory.StackShrinkPreservesQuantity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryStackShrinkPreservesQuantityTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	AddExpectedMessagePlain(TEXT("스택 상한이 줄어든 저장 데이터"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	// 상한이 3으로 줄어든 뒤, 예전에 7개가 한 항목에 들어 있던 저장을 연다
	UACItemDefinition* Item = Fixture.MakeDefinition(TEXT("DA_Item_Shrink"), 3, ACGameplayTags::Item_Category_Material);

	FACSavedInventoryEntry LegacyEntry;
	LegacyEntry.ItemDefinitionId = Item->GetPrimaryAssetId();
	LegacyEntry.StackCount = 7;
	LegacyEntry.InstanceId = FGuid::NewGuid();
	LegacyEntry.UpgradeLevel = 2;

	Fixture.WriteSaveWithEntries(SlotA, { LegacyEntry });
	Fixture.MetaProgression->LoadSlot(SlotA);

	TestEqual(TEXT("수량이 잘리지 않고 전부 보존된다"), Fixture.Inventory->GetTotalItemCount(Item), 7);
	TestEqual(TEXT("상한 3에 맞춰 3개 항목으로 나뉜다"), Fixture.Inventory->GetEntries().Num(), 3);

	const FACInventoryEntry* OriginalEntry = Fixture.Inventory->FindEntryByInstanceId(LegacyEntry.InstanceId);
	if (!OriginalEntry)
	{
		AddError(TEXT("원본 InstanceId를 가진 항목이 남아 있지 않습니다."));
		return false;
	}

	TestEqual(TEXT("원본 항목이 강화 단계를 유지한다"), OriginalEntry->UpgradeLevel, 2);
	TestEqual(TEXT("원본 항목은 상한까지만 담는다"), OriginalEntry->StackCount, 3);

	// 넘친 수량이 강화 상태를 복제하면 상한 축소가 강화 증식 수단이 된다
	for (const FACInventoryEntry& Entry : Fixture.Inventory->GetEntries())
	{
		if (Entry.InstanceId != LegacyEntry.InstanceId)
		{
			TestEqual(TEXT("넘친 항목에는 강화가 복제되지 않는다"), Entry.UpgradeLevel, 0);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventorySaveFailureKeepsDirtyTest,
	"AshenCathedral.Inventory.SaveFailureKeepsDirty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventorySaveFailureKeepsDirtyTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	AddExpectedMessagePlain(TEXT("슬롯 저장에 실패했습니다"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	UACItemDefinition* Item = Fixture.MakeDefinition(TEXT("DA_Item_SaveFail"), 10, ACGameplayTags::Item_Category_Material);

	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.Inventory->AddItem(Item, 1);
	TestFalse(TEXT("정상 저장 후에는 미저장 변경이 없다"), Fixture.MetaProgression->HasPendingSave());

	// 여기서부터 쓰기가 실패한다
	UACMetaProgressionSubsystem::SetForceSaveFailureForTests(true);

	Fixture.MetaProgression->AddCurrency(ACGameplayTags::MetaProgression_Currency_AshSoul, 25);
	TestTrue(TEXT("재화 저장 실패 후 dirty가 유지된다"), Fixture.MetaProgression->HasPendingSave());

	Fixture.Inventory->AddItem(Item, 3);
	TestTrue(TEXT("인벤토리 저장 실패 후에도 dirty가 유지된다"), Fixture.MetaProgression->HasPendingSave());

	Fixture.MetaProgression->MarkBossCleared(ACGameplayTags::MetaProgression_BossID_Ordan);
	TestTrue(TEXT("보스 기록 저장 실패 후에도 dirty가 유지된다"), Fixture.MetaProgression->HasPendingSave());

	Fixture.Inventory->EquipItem(Item);
	TestTrue(TEXT("장착 ID 저장 실패 후에도 dirty가 유지된다"), Fixture.MetaProgression->HasPendingSave());

	// 실패한 동안에는 디스크에 아무것도 반영되지 않았어야 한다
	{
		const UACSaveGame_MetaProgression* OnDisk = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::LoadGameFromSlot(MakeTestSlotName(SlotA), SaveUserIndex));
		if (!OnDisk)
		{
			AddError(TEXT("실패 이전에 저장된 파일을 읽지 못했습니다."));
			return false;
		}
		TestEqual(TEXT("실패한 재화는 디스크에 없다"), OnDisk->CurrencyAmounts.FindRef(ACGameplayTags::MetaProgression_Currency_AshSoul), 0);
		TestEqual(TEXT("실패한 인벤토리 증가분도 디스크에 없다"), OnDisk->InventoryEntries.Num(), 1);
	}

	// 쓰기를 다시 허용하고, 슬롯 전환으로 flush 재시도를 유발한다
	UACMetaProgressionSubsystem::SetForceSaveFailureForTests(false);
	Fixture.MetaProgression->LoadSlot(SlotB);

	const UACSaveGame_MetaProgression* Retried = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::LoadGameFromSlot(MakeTestSlotName(SlotA), SaveUserIndex));
	if (!Retried)
	{
		AddError(TEXT("재시도 후 저장된 파일을 읽지 못했습니다."));
		return false;
	}

	TestEqual(TEXT("재시도로 재화가 저장된다"), Retried->CurrencyAmounts.FindRef(ACGameplayTags::MetaProgression_Currency_AshSoul), 25);
	TestEqual(TEXT("재시도로 인벤토리가 저장된다"), Retried->InventoryEntries.Num(), 1);
	TestEqual(TEXT("재시도로 저장된 인벤토리 수량이 맞다"), Retried->InventoryEntries[0].StackCount, 4);
	TestTrue(TEXT("재시도로 보스 기록이 저장된다"), Retried->ClearedBossTags.HasTagExact(ACGameplayTags::MetaProgression_BossID_Ordan));
	TestTrue(TEXT("재시도로 장착 ID가 저장된다"), Retried->EquippedItemDefinitionId.IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryDeleteFailureKeepsStateTest,
	"AshenCathedral.Inventory.DeleteFailureKeepsState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryDeleteFailureKeepsStateTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	AddExpectedMessagePlain(TEXT("삭제에 실패했습니다"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	UACItemDefinition* Item = Fixture.MakeDefinition(TEXT("DA_Item_DeleteFail"), 10, ACGameplayTags::Item_Category_Material);

	Fixture.MetaProgression->LoadSlot(SlotC);
	Fixture.Inventory->AddItem(Item, 2);
	Fixture.MetaProgression->AddCurrency(ACGameplayTags::MetaProgression_Currency_AshSoul, 40);

	const FString SlotFilePath = MakeTestSlotFilePath(SlotC);

	// 파일 핸들을 열어 둔 채로 삭제를 시도시킨다. Windows에서는 공유 위반으로 삭제가 실패한다
	TUniquePtr<IFileHandle> OpenHandle(FPlatformFileManager::Get().GetPlatformFile().OpenRead(*SlotFilePath, /*bAllowWrite*/ false));

	Fixture.MetaProgression->DeleteSlot(SlotC);

	const bool bFileStillExists = UGameplayStatics::DoesSaveGameExist(MakeTestSlotName(SlotC), SaveUserIndex);

	if (bFileStillExists)
	{
		// 삭제가 실패했으므로 메모리 상태가 그대로여야 한다
		TestTrue(TEXT("삭제 실패 시 활성 슬롯이 유지된다"), Fixture.MetaProgression->HasActiveSlot());
		TestEqual(TEXT("삭제 실패 시 슬롯 번호가 유지된다"), Fixture.MetaProgression->GetActiveSlotIndex(), SlotC);
		TestEqual(TEXT("삭제 실패 시 인벤토리가 유지된다"), Fixture.Inventory->GetTotalItemCount(Item), 2);
		TestEqual(TEXT("삭제 실패 시 재화가 유지된다"), Fixture.MetaProgression->GetCurrencyAmount(ACGameplayTags::MetaProgression_Currency_AshSoul), 40);
	}
	else
	{
		// 열린 핸들이 삭제를 막지 않는 플랫폼이다. 이 경우 검증할 실패 경로가 없다
		AddInfo(TEXT("이 플랫폼에서는 열린 파일 핸들이 삭제를 막지 않아 삭제 실패 경로를 재현하지 못했습니다."));
	}

	OpenHandle.Reset();

	// 핸들을 놓으면 정상적으로 삭제되어야 한다
	Fixture.MetaProgression->DeleteSlot(SlotC);
	TestFalse(TEXT("핸들 해제 후에는 삭제된다"), UGameplayStatics::DoesSaveGameExist(MakeTestSlotName(SlotC), SaveUserIndex));
	TestFalse(TEXT("삭제 성공 후에는 슬롯 미선택 상태가 된다"), Fixture.MetaProgression->HasActiveSlot());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryDeleteMissingFileIsEmptySlotTest,
	"AshenCathedral.Inventory.DeleteMissingFileIsEmptySlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryDeleteMissingFileIsEmptySlotTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	// 한 번도 저장한 적 없는 슬롯을 지우는 것은 실패가 아니라 이미 빈 슬롯이다
	Fixture.MetaProgression->LoadSlot(SlotB);
	TestFalse(TEXT("파일이 없는 새 슬롯이다"), UGameplayStatics::DoesSaveGameExist(MakeTestSlotName(SlotB), SaveUserIndex));

	Fixture.MetaProgression->DeleteSlot(SlotB);

	TestFalse(TEXT("파일이 없어도 활성 슬롯이 정상적으로 해제된다"), Fixture.MetaProgression->HasActiveSlot());
	TestEqual(TEXT("슬롯 번호가 INDEX_NONE으로 돌아간다"), Fixture.MetaProgression->GetActiveSlotIndex(), (int32)INDEX_NONE);

	return true;
}

#pragma endregion

#pragma region 저장 실패 지속 보류 쓰기 큐

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryPendingWriteRetriedAfterSlotSwitchTest,
	"AshenCathedral.Inventory.PendingWriteRetriedAfterSlotSwitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryPendingWriteRetriedAfterSlotSwitchTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	AddExpectedMessagePlain(TEXT("슬롯 저장에 실패했습니다"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);
	AddExpectedMessagePlain(TEXT("보류 큐에 담아 두고"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	UACItemDefinition* Item = Fixture.MakeDefinition(TEXT("DA_Item_PendingRetry"), 10, ACGameplayTags::Item_Category_Material);

	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.Inventory->AddItem(Item, 1);

	// 저장이 계속 실패하는 상태에서 슬롯 A를 바꾼다
	UACMetaProgressionSubsystem::SetForceSaveFailureForTests(true);
	Fixture.MetaProgression->AddCurrency(ACGameplayTags::MetaProgression_Currency_AshSoul, 25);
	TestTrue(TEXT("저장 실패로 dirty가 남는다"), Fixture.MetaProgression->HasPendingSave());

	Fixture.MetaProgression->LoadSlot(SlotB);

	// 전환 시 flush도 실패했으므로 A의 데이터가 보류 큐로 넘어가야 한다
	TestEqual(TEXT("보류 큐에 슬롯 A가 담긴다"), Fixture.MetaProgression->GetPendingSlotWriteCount(), 1);

	{
		const UACSaveGame_MetaProgression* SlotAOnDisk = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::LoadGameFromSlot(MakeTestSlotName(SlotA), SaveUserIndex));
		if (!SlotAOnDisk)
		{
			AddError(TEXT("슬롯 A 파일을 읽지 못했습니다."));
			return false;
		}
		TestEqual(TEXT("아직 디스크에는 반영되지 않았다"), SlotAOnDisk->CurrencyAmounts.FindRef(ACGameplayTags::MetaProgression_Currency_AshSoul), 0);
	}

	// 디스크가 다시 살아난 뒤, 슬롯 B에서 아무 저장이나 일어나면 밀려 있던 A부터 정리된다
	UACMetaProgressionSubsystem::SetForceSaveFailureForTests(false);
	Fixture.MetaProgression->AddCurrency(ACGameplayTags::MetaProgression_Currency_RelicFragment, 1);

	TestEqual(TEXT("보류 큐가 비워진다"), Fixture.MetaProgression->GetPendingSlotWriteCount(), 0);

	const UACSaveGame_MetaProgression* RetriedSlotA = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::LoadGameFromSlot(MakeTestSlotName(SlotA), SaveUserIndex));
	if (!RetriedSlotA)
	{
		AddError(TEXT("재시도 후 슬롯 A 파일을 읽지 못했습니다."));
		return false;
	}

	TestEqual(TEXT("슬롯을 바꿨어도 원래 슬롯 A에 반영된다"), RetriedSlotA->CurrencyAmounts.FindRef(ACGameplayTags::MetaProgression_Currency_AshSoul), 25);
	TestEqual(TEXT("슬롯 A의 인벤토리도 함께 보존된다"), RetriedSlotA->InventoryEntries.Num(), 1);

	// 슬롯 B의 데이터가 A로 새어 들어가지 않았는지도 확인한다
	TestEqual(TEXT("슬롯 B의 재화가 A에 섞이지 않는다"), RetriedSlotA->CurrencyAmounts.FindRef(ACGameplayTags::MetaProgression_Currency_RelicFragment), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryPendingWriteAdoptedOnReturnTest,
	"AshenCathedral.Inventory.PendingWriteAdoptedOnReturn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryPendingWriteAdoptedOnReturnTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	AddExpectedMessagePlain(TEXT("슬롯 저장에 실패했습니다"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);
	AddExpectedMessagePlain(TEXT("보류 큐에 담아 두고"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	UACItemDefinition* Item = Fixture.MakeDefinition(TEXT("DA_Item_PendingAdopt"), 10, ACGameplayTags::Item_Category_Material);

	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.Inventory->AddItem(Item, 1);

	UACMetaProgressionSubsystem::SetForceSaveFailureForTests(true);
	Fixture.Inventory->AddItem(Item, 4);
	Fixture.MetaProgression->AddCurrency(ACGameplayTags::MetaProgression_Currency_AshSoul, 7);

	Fixture.MetaProgression->LoadSlot(SlotB);
	TestEqual(TEXT("보류 큐에 슬롯 A가 담긴다"), Fixture.MetaProgression->GetPendingSlotWriteCount(), 1);

	// 저장이 여전히 실패하는 중에 슬롯 A로 되돌아온다.
	// 디스크에는 옛 데이터(1개)뿐이므로, 보류분(5개)을 되찾아 오지 않으면 진행이 되감긴다
	Fixture.MetaProgression->LoadSlot(SlotA);

	TestEqual(TEXT("보류분을 되찾아 큐가 비워진다"), Fixture.MetaProgression->GetPendingSlotWriteCount(), 0);
	TestEqual(TEXT("디스크의 옛 값이 아니라 보류분이 복원된다"), Fixture.Inventory->GetTotalItemCount(Item), 5);
	TestEqual(TEXT("재화도 보류분 기준으로 복원된다"), Fixture.MetaProgression->GetCurrencyAmount(ACGameplayTags::MetaProgression_Currency_AshSoul), 7);
	TestTrue(TEXT("되찾은 데이터는 아직 디스크에 없으므로 dirty로 남는다"), Fixture.MetaProgression->HasPendingSave());

	// 디스크가 살아나면 그대로 확정된다
	UACMetaProgressionSubsystem::SetForceSaveFailureForTests(false);
	Fixture.MetaProgression->LoadSlot(SlotB);

	const UACSaveGame_MetaProgression* SlotAOnDisk = Cast<UACSaveGame_MetaProgression>(UGameplayStatics::LoadGameFromSlot(MakeTestSlotName(SlotA), SaveUserIndex));
	if (!SlotAOnDisk)
	{
		AddError(TEXT("슬롯 A 파일을 읽지 못했습니다."));
		return false;
	}

	TestEqual(TEXT("최종적으로 보류분이 디스크에 반영된다"), SlotAOnDisk->CurrencyAmounts.FindRef(ACGameplayTags::MetaProgression_Currency_AshSoul), 7);
	TestEqual(TEXT("인벤토리 수량도 5로 반영된다"), SlotAOnDisk->InventoryEntries[0].StackCount, 5);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACInventoryShutdownReportsUnsavedSlotsTest,
	"AshenCathedral.Inventory.ShutdownReportsUnsavedSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACInventoryShutdownReportsUnsavedSlotsTest::RunTest(const FString& Parameters)
{
	FInventorySaveFixture Fixture;

	AddExpectedMessagePlain(TEXT("슬롯 저장에 실패했습니다"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);
	AddExpectedMessagePlain(TEXT("보류 큐에 담아 두고"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);
	// 종료 시점의 유실은 진짜 데이터 손실이므로 Error로 남긴다
	AddExpectedErrorPlain(TEXT("종료 시점까지 슬롯"), EAutomationExpectedErrorFlags::Contains, 0);

	UACItemDefinition* Item = Fixture.MakeDefinition(TEXT("DA_Item_ShutdownLoss"), 10, ACGameplayTags::Item_Category_Material);

	Fixture.MetaProgression->LoadSlot(SlotA);
	Fixture.Inventory->AddItem(Item, 1);

	UACMetaProgressionSubsystem::SetForceSaveFailureForTests(true);
	Fixture.MetaProgression->AddCurrency(ACGameplayTags::MetaProgression_Currency_AshSoul, 11);
	TestTrue(TEXT("저장 실패로 dirty가 남는다"), Fixture.MetaProgression->HasPendingSave());

	// 끝까지 디스크가 돌아오지 않은 채 종료한다
	Fixture.MetaProgression->Deinitialize();

	TestEqual(TEXT("종료 후 보류 큐는 비워진다(유실 확정)"), Fixture.MetaProgression->GetPendingSlotWriteCount(), 0);

	return true;
}

#pragma endregion

#endif // WITH_AUTOMATION_TESTS
