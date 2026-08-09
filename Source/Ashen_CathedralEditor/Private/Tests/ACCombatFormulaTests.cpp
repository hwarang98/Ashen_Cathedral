// 체간·가드 피해 수식의 회귀를 막는 자동화 테스트 — 임시 월드에 ASC를 세우고 즉시형 GE로 메타 어트리뷰트를 주입한다

#include "Misc/AutomationTest.h"

#include "ACGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "GameplayEffect.h"
#include "Tests/ACCombatTestActor.h"

#if WITH_AUTOMATION_TESTS

namespace ACCombatFormulaTests
{
	/** 테스트 하나가 쓰는 임시 월드와 ASC. 소멸자에서 월드를 정리한다 */
	struct FCombatFixture
	{
		UWorld* World = nullptr;
		AACCombatTestActor* Actor = nullptr;
		UACAbilitySystemComponent* ASC = nullptr;

		bool Setup()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World)
			{
				return false;
			}

			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());

			Actor = World->SpawnActor<AACCombatTestActor>();
			if (!Actor)
			{
				return false;
			}

			ASC = Cast<UACAbilitySystemComponent>(Actor->GetAbilitySystemComponent());
			if (!ASC)
			{
				return false;
			}

			ASC->InitAbilityActorInfo(Actor, Actor);
			ASC->AddSet<UACAttributeSet>();
			return true;
		}

		~FCombatFixture()
		{
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		void Set(const FGameplayAttribute& Attribute, const float Value) const
		{
			ASC->SetNumericAttributeBase(Attribute, Value);
		}

		float Get(const FGameplayAttribute& Attribute) const
		{
			return ASC->GetNumericAttribute(Attribute);
		}

		/** 메타 어트리뷰트에 즉시형 GE를 적용해 PostGameplayEffectExecute를 태운다 */
		void Inject(const FGameplayAttribute& MetaAttribute, const float Amount) const
		{
			UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("ACCombatFormulaTestEffect")));
			Effect->DurationPolicy = EGameplayEffectDurationType::Instant;

			FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
			Modifier.Attribute = MetaAttribute;
			Modifier.ModifierOp = EGameplayModOp::Additive;
			Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Amount));

			ASC->ApplyGameplayEffectToSelf(Effect, 1.f, ASC->MakeEffectContext());
		}

		/** 지정한 태그의 게임플레이 이벤트가 몇 번 왔는지 세는 리스너를 단다 */
		void ListenForEvent(const FGameplayTag& EventTag, int32& OutCounter) const
		{
			ASC->GenericGameplayEventCallbacks.FindOrAdd(EventTag).AddLambda([&OutCounter](const FGameplayEventData*)
			{
				++OutCounter;
			});
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACPostureDamageResistanceTest,
	"AshenCathedral.Combat.PostureDamage.Resistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACPostureDamageResistanceTest::RunTest(const FString& Parameters)
{
	ACCombatFormulaTests::FCombatFixture Fixture;
	if (!TestTrue(TEXT("테스트 픽스처 준비"), Fixture.Setup()))
	{
		return false;
	}

	// 체력 100%라 취약도 가중이 걸리지 않는 조건
	Fixture.Set(UACAttributeSet::GetMaxHealthAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetHealthAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetMaxPostureAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetPostureAttribute(), 0.f);
	Fixture.Set(UACAttributeSet::GetPostureResistanceAttribute(), 10.f);

	Fixture.Inject(UACAttributeSet::GetPostureDamageTakenAttribute(), 30.f);

	TestEqual(TEXT("체간 피해가 PostureResistance만큼 감쇄된다"), Fixture.Get(UACAttributeSet::GetPostureAttribute()), 20.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACPostureDamageFullyResistedTest,
	"AshenCathedral.Combat.PostureDamage.FullyResisted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACPostureDamageFullyResistedTest::RunTest(const FString& Parameters)
{
	ACCombatFormulaTests::FCombatFixture Fixture;
	if (!TestTrue(TEXT("테스트 픽스처 준비"), Fixture.Setup()))
	{
		return false;
	}

	Fixture.Set(UACAttributeSet::GetMaxHealthAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetHealthAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetMaxPostureAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetPostureAttribute(), 0.f);
	Fixture.Set(UACAttributeSet::GetPostureResistanceAttribute(), 50.f);

	Fixture.Inject(UACAttributeSet::GetPostureDamageTakenAttribute(), 30.f);

	TestEqual(TEXT("저항이 피해보다 크면 체간이 오르지 않는다"), Fixture.Get(UACAttributeSet::GetPostureAttribute()), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACPostureDamageLowHealthWeightTest,
	"AshenCathedral.Combat.PostureDamage.LowHealthWeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACPostureDamageLowHealthWeightTest::RunTest(const FString& Parameters)
{
	ACCombatFormulaTests::FCombatFixture Fixture;
	if (!TestTrue(TEXT("테스트 픽스처 준비"), Fixture.Setup()))
	{
		return false;
	}

	// 체력 비율이 정확히 0.5면 취약도 가중(1.25배)이 걸린다
	Fixture.Set(UACAttributeSet::GetMaxHealthAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetHealthAttribute(), 50.f);
	Fixture.Set(UACAttributeSet::GetMaxPostureAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetPostureAttribute(), 0.f);
	Fixture.Set(UACAttributeSet::GetPostureResistanceAttribute(), 0.f);

	Fixture.Inject(UACAttributeSet::GetPostureDamageTakenAttribute(), 40.f);

	TestEqual(TEXT("체력 절반 이하에서 체간 피해가 1.25배로 가중된다"), Fixture.Get(UACAttributeSet::GetPostureAttribute()), 50.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACPostureBreakTest,
	"AshenCathedral.Combat.PostureDamage.Break",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACPostureBreakTest::RunTest(const FString& Parameters)
{
	ACCombatFormulaTests::FCombatFixture Fixture;
	if (!TestTrue(TEXT("테스트 픽스처 준비"), Fixture.Setup()))
	{
		return false;
	}

	Fixture.Set(UACAttributeSet::GetMaxHealthAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetHealthAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetMaxPostureAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetPostureAttribute(), 90.f);
	Fixture.Set(UACAttributeSet::GetPostureResistanceAttribute(), 0.f);

	int32 BreakEventCount = 0;
	Fixture.ListenForEvent(ACGameplayTags::Shared_Event_PostureBrokenTriggered, BreakEventCount);

	Fixture.Inject(UACAttributeSet::GetPostureDamageTakenAttribute(), 50.f);

	TestEqual(TEXT("체간이 MaxPosture로 클램프된다"), Fixture.Get(UACAttributeSet::GetPostureAttribute()), 100.f);
	TestEqual(TEXT("체간 붕괴 이벤트가 한 번 발송된다"), BreakEventCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACGuardDamageResistanceTest,
	"AshenCathedral.Combat.GuardDamage.Resistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACGuardDamageResistanceTest::RunTest(const FString& Parameters)
{
	ACCombatFormulaTests::FCombatFixture Fixture;
	if (!TestTrue(TEXT("테스트 픽스처 준비"), Fixture.Setup()))
	{
		return false;
	}

	Fixture.Set(UACAttributeSet::GetMaxGuardGaugeAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetGuardGaugeAttribute(), 0.f);
	Fixture.Set(UACAttributeSet::GetGuardBreakResistanceAttribute(), 5.f);

	Fixture.Inject(UACAttributeSet::GetGuardDamageTakenAttribute(), 30.f);

	TestEqual(TEXT("가드 부하가 GuardBreakResistance만큼 감쇄된다"), Fixture.Get(UACAttributeSet::GetGuardGaugeAttribute()), 25.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACGuardBreakTest,
	"AshenCathedral.Combat.GuardDamage.Break",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACGuardBreakTest::RunTest(const FString& Parameters)
{
	ACCombatFormulaTests::FCombatFixture Fixture;
	if (!TestTrue(TEXT("테스트 픽스처 준비"), Fixture.Setup()))
	{
		return false;
	}

	Fixture.Set(UACAttributeSet::GetMaxGuardGaugeAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetGuardGaugeAttribute(), 90.f);
	Fixture.Set(UACAttributeSet::GetGuardBreakResistanceAttribute(), 0.f);

	int32 BreakEventCount = 0;
	Fixture.ListenForEvent(ACGameplayTags::Shared_Event_GuardBrokenTriggered, BreakEventCount);

	Fixture.Inject(UACAttributeSet::GetGuardDamageTakenAttribute(), 20.f);

	TestEqual(TEXT("가드 브레이크 시 게이지가 0으로 리셋된다"), Fixture.Get(UACAttributeSet::GetGuardGaugeAttribute()), 0.f);
	TestEqual(TEXT("가드 브레이크 이벤트가 한 번 발송된다"), BreakEventCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FACBaseValueClampTest,
	"AshenCathedral.Combat.BaseValueClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FACBaseValueClampTest::RunTest(const FString& Parameters)
{
	ACCombatFormulaTests::FCombatFixture Fixture;
	if (!TestTrue(TEXT("테스트 픽스처 준비"), Fixture.Setup()))
	{
		return false;
	}

	// PreAttributeBaseChange가 주기형 GE의 드리프트를 막기 위해 세 게이지를 클램프한다
	Fixture.Set(UACAttributeSet::GetMaxPostureAttribute(), 100.f);
	Fixture.Set(UACAttributeSet::GetMaxStaminaAttribute(), 80.f);
	Fixture.Set(UACAttributeSet::GetMaxGuardGaugeAttribute(), 60.f);

	Fixture.Set(UACAttributeSet::GetPostureAttribute(), 150.f);
	Fixture.Set(UACAttributeSet::GetStaminaAttribute(), -20.f);
	Fixture.Set(UACAttributeSet::GetGuardGaugeAttribute(), 999.f);

	TestEqual(TEXT("Posture가 MaxPosture로 클램프된다"), Fixture.Get(UACAttributeSet::GetPostureAttribute()), 100.f);
	TestEqual(TEXT("Stamina가 0 미만으로 내려가지 않는다"), Fixture.Get(UACAttributeSet::GetStaminaAttribute()), 0.f);
	TestEqual(TEXT("GuardGauge가 MaxGuardGauge로 클램프된다"), Fixture.Get(UACAttributeSet::GetGuardGaugeAttribute()), 60.f);
	return true;
}

#endif // WITH_AUTOMATION_TESTS
