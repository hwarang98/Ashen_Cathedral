// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
#include "GameplayEffectExtension.h"
#include "Components/UI/PawnUIComponent.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interfaces/PawnUIInterface.h"

#if AC_WEB_DEBUG
	#include "Abilities/GameplayAbility.h" // 피격 기록에서 출처 어빌리티의 GetClass()를 부른다
	#include "Debug/ACRunLogSubsystem.h"
	#include "Debug/ACWebDebugSubsystem.h"
#endif

UACAttributeSet::UACAttributeSet()
{
	//Core
	InitMoveSpeed(400.f);
	InitHealth(1.f);
	InitMaxHealth(1.f);
	InitStamina(1.f);
	InitMaxStamina(1.f);
	InitStaminaRegenRate(1.f);

	// Posture
	InitPosture(1.f);
	InitMaxPosture(1.f);
	InitPostureResistance(1.f);
	InitPostureDamageTaken(0.f);

	// Guard — MaxGuardGauge/GuardGaugeRegenRate는 초기화 GE(GE_Player_Init 등)가 설정한다.
	// 여기서는 0 나눗셈과 즉시 브레이크를 피하기 위한 안전값만 둔다(MaxHealth/MaxPosture와 동일한 규칙).
	InitGuardGauge(0.f);
	InitMaxGuardGauge(1.f);
	InitGuardBreakResistance(0.f);
	InitGuardGaugeRegenRate(1.f);
	InitGuardDamageTaken(0.f);

	// Combat
	InitAttackPower(1.f); // 배율
	InitDefensePower(0.f);
	InitAttackSpeed(1.f);
	InitDamageTaken(0.f);
	InitStaminaCost(0.f);

	// Burn
	InitBurnGauge(0.f);
	InitMaxBurnGauge(100.f);
	InitBurnAccumulation(0.f);
}

void UACAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}

	// Core — 현재값은 0과 최대값 사이로 클램프
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetStaminaRegenRateAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}

	// Posture — 게이지는 0과 최대값 사이로 클램프
	else if (Attribute == GetPostureAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPosture());
	}
	else if (Attribute == GetPostureResistanceAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}

	// Guard — 게이지는 0과 최대값 사이로 클램프
	else if (Attribute == GetGuardGaugeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxGuardGauge());
	}
	else if (Attribute == GetGuardBreakResistanceAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetGuardGaugeRegenRateAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}

	// Burn — 게이지는 0과 최대값 사이로 클램프
	else if (Attribute == GetBurnGaugeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxBurnGauge());
	}

	// Combat — 음수 방지
	else if (Attribute == GetAttackPowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetDefensePowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetAttackSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UACAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	// GE_PostureDecay 같은 주기형(Periodic) Additive Modifier는 BaseValue를 직접 변경하므로,
	// PreAttributeChange(CurrentValue 클램프)만으로는 BaseValue의 음수 드리프트를 막지 못한다.
	if (Attribute == GetPostureAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPosture());
	}
	// GE_StaminaRegen도 동일하게 주기형 AddBase 모디파이어를 사용하므로 안전망 차원에서 함께 클램프한다.
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	// GE_GuardGaugeDecay도 주기형으로 BaseValue를 직접 깎으므로 동일하게 클램프한다.
	else if (Attribute == GetGuardGaugeAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxGuardGauge());
	}
}

void UACAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (!CachedPawnUIInterface.IsValid())
	{
		CachedPawnUIInterface = TWeakInterfacePtr<IPawnUIInterface>(Data.Target.GetAvatarActor());
	}

	checkf(CachedPawnUIInterface.IsValid(), TEXT("%s가 IPawnUIInterface를 구현하지 않았습니다."), *Data.Target.GetAvatarActor()->GetActorNameOrLabel());

	UPawnUIComponent* PawnUIComponent = CachedPawnUIInterface->GetPawnUIComponent();

	checkf(PawnUIComponent, TEXT("%s로 부터 PawnUIComponent를 추출할수 없습니다."), *Data.Target.GetAvatarActor()->GetActorNameOrLabel());

	// 변경된 어트리뷰트에 따라 클램프 및 UI 업데이트 / 전용 핸들러 호출
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// 체력을 [0, MaxHealth] 범위로 클램프 후 UI에 정규화된 비율 전달
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
		PawnUIComponent->OnCurrentHealthChanged.Broadcast(GetHealth() / GetMaxHealth());
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		// Periodic GE가 BaseValue를 무한정 올리지 않도록 클램프 후 UI에 비율 전달
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
		PawnUIComponent->OnCurrentStaminaChanged.Broadcast(GetStamina() / GetMaxStamina());
	}
	else if (Data.EvaluatedData.Attribute == GetPostureAttribute())
	{
		// GE_PostureDecay 같은 주기형 GE가 Posture를 직접 변경할 때도 UI에 비율 전달
		SetPosture(FMath::Clamp(GetPosture(), 0.f, GetMaxPosture()));
		PawnUIComponent->OnPostureChanged.Broadcast(GetPosture() / GetMaxPosture());
	}
	else if (Data.EvaluatedData.Attribute == GetDamageTakenAttribute())
	{
		// 피해 처리 및 HitReact 트리거
		HandleDamageAndTriggerHitReact(Data);
		PawnUIComponent->OnCurrentHealthChanged.Broadcast(GetHealth() / GetMaxHealth());
	}
	else if (Data.EvaluatedData.Attribute == GetPostureDamageTakenAttribute())
	{
		// 체간 피해 처리 (누적량에 따라 체간 붕괴 상태 진입 여부 결정)
		HandlePostureDamage(Data);
		PawnUIComponent->OnPostureChanged.Broadcast(GetPosture() / GetMaxPosture());
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaCostAttribute())
	{
		// 스태미나 소모 처리 (스태미나에서 비용 차감)
		HandleStaminaConsumption(Data);
		PawnUIComponent->OnCurrentStaminaChanged.Broadcast(GetStamina() / GetMaxStamina());
	}
	else if (Data.EvaluatedData.Attribute == GetGuardDamageTakenAttribute())
	{
		// 가드 부하 처리 (누적량이 최대치에 도달하면 가드 브레이크 이벤트 발송)
		HandleGuardDamage(Data);
	}
	else if (Data.EvaluatedData.Attribute == GetBurnAccumulationAttribute())
	{
		// 화상 누적량 처리 (임계치 도달 시 번 상태 적용)
		HandleBurnBuildUp(Data);
	}
}


void UACAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMoveSpeedAttribute())
	{
		if (ACharacter* Character = Cast<ACharacter>(GetOwningActor()))
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed = NewValue;
			}
		}
	}

#if AC_WEB_DEBUG
	// 웹 디버그 타임라인 게이지 기록 — 실제 전송은 15Hz 틱에서, 값이 변했을 때만 일어난다.
	// 최대값이 바뀌어도 정규화 값이 달라지므로 Max* 변경도 함께 흘려보낸다.
	const AActor* Owner = GetOwningActor();
	UACWebDebugSubsystem* WebDebug = UACWebDebugSubsystem::Get(Owner);
	if (WebDebug && Owner)
	{
		if (Attribute == GetHealthAttribute() || Attribute == GetMaxHealthAttribute())
		{
			WebDebug->RecordGauge(Owner, TEXT("Health"), GetHealth(), GetMaxHealth());
		}
		else if (Attribute == GetStaminaAttribute() || Attribute == GetMaxStaminaAttribute())
		{
			WebDebug->RecordGauge(Owner, TEXT("Stamina"), GetStamina(), GetMaxStamina());
		}
		else if (Attribute == GetPostureAttribute() || Attribute == GetMaxPostureAttribute())
		{
			WebDebug->RecordGauge(Owner, TEXT("Posture"), GetPosture(), GetMaxPosture());
		}
		else if (Attribute == GetGuardGaugeAttribute() || Attribute == GetMaxGuardGaugeAttribute())
		{
			WebDebug->RecordGauge(Owner, TEXT("GuardGauge"), GetGuardGauge(), GetMaxGuardGauge());
		}
		else if (Attribute == GetBurnGaugeAttribute() || Attribute == GetMaxBurnGaugeAttribute())
		{
			WebDebug->RecordGauge(Owner, TEXT("BurnGauge"), GetBurnGauge(), GetMaxBurnGauge());
		}
	}
#endif
}

void UACAttributeSet::HandlePostureDamage(const FGameplayEffectModCallbackData& Data)
{
	const float PostureDamage = GetPostureDamageTaken();
	SetPostureDamageTaken(0.f);

	UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();

	// 사망 상태라면 체간 누적 불필요
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead))
	{
		return;
	}

	// 이미 체간 붕괴 상태라면 누적하지 않음
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_PostureBroken))
	{
		return;
	}

	// 처형 중에는 체간 누적 불필요
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Executed))
	{
		return;
	}

	// 무적 태그는 항상 체간 데미지 무효화
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Invincible))
	{
		return;
	}

	// 슈퍼아머는 HitReact 모션만 생략시키며, 체간 데미지는 그대로 누적된다.

	float ReducedDamage = FMath::Max(PostureDamage - GetPostureResistance(), 0.f);

	// 체력 비율에 따른 체간 취약도: 체력이 절반 이하이면 체간 데미지를 1.25배 가중한다.
	// TODO: 추후 Posture Recovery 시스템이 도입되면, 이 취약도 로직은 회복 속도 감소 방식으로 분리한다.
	const float HealthRatio = GetMaxHealth() > 0.f ? GetHealth() / GetMaxHealth() : 1.f;
	if (HealthRatio <= 0.5f)
	{
		ReducedDamage *= 1.25f;
	}

	const float OldPosture = GetPosture();
	const float NewPosture = FMath::Clamp(OldPosture + ReducedDamage, 0.f, GetMaxPosture());
	SetPosture(NewPosture);

	// 체간 자연 감소 지연 타이머 리셋 — 실제로 게이지가 증가했을 때만, 마지막 피해 시점부터 유예시간 이후 감소가 재개된다.
	// GE의 Stacking(Refresh on Successful Application)이 Duration을 자동 리셋하므로 재적용만으로 충분하다.
	if (ReducedDamage > 0.f)
	{
		if (UACAbilitySystemComponent* ACTargetASC = Cast<UACAbilitySystemComponent>(TargetASC))
		{
			if (ACTargetASC->PostureDecayDelayEffectClass)
			{
				const UGameplayEffect* DecayDelayGE = ACTargetASC->PostureDecayDelayEffectClass->GetDefaultObject<UGameplayEffect>();
				ACTargetASC->ApplyGameplayEffectToSelf(DecayDelayGE, 1, ACTargetASC->MakeEffectContext());
			}
		}
	}

	if (NewPosture >= GetMaxPosture())
	{
#if AC_WEB_DEBUG
		if (UACWebDebugSubsystem* WebDebug = UACWebDebugSubsystem::Get(Data.Target.GetAvatarActor()))
		{
			WebDebug->RecordMarker(Data.Target.GetAvatarActor(), TEXT("PostureBroken"));
		}
		if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(Data.Target.GetAvatarActor()))
		{
			RunLog->NotifyPostureBreak(Data.Target.GetAvatarActor());
		}
#endif

		// 체간 붕괴 어빌리티(GA_Groggy)를 통해 체간 붕괴 상태 처리
		FGameplayEventData Payload;
		Payload.EventTag = ACGameplayTags::Shared_Event_PostureBrokenTriggered;
		Payload.Target = Data.Target.GetAvatarActor();

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Data.Target.GetAvatarActor(),
			ACGameplayTags::Shared_Event_PostureBrokenTriggered,
			Payload
			);
	}
}

void UACAttributeSet::HandleGuardDamage(const FGameplayEffectModCallbackData& Data)
{
	const float GuardDamage = GetGuardDamageTaken();
	SetGuardDamageTaken(0.f);

	UE_LOG(LogTemp, Warning, TEXT("[Guard] HandleGuardDamage 진입 — 받은 부하=%.1f 현재게이지=%.1f 최대=%.1f"),
		GuardDamage, GetGuardGauge(), GetMaxGuardGauge());

	UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();

	// 사망·처형·체간 붕괴 중에는 방어 자체가 성립하지 않으므로 누적하지 않는다
	if (TargetASC
		&& (TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead)
			|| TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Executed)
			|| TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_PostureBroken)))
	{
		return;
	}

	// 이미 가드가 무너진 상태면 중복 발동하지 않는다
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Player_Status_GuardBroken))
	{
		return;
	}

	// GuardBreakResistance만큼 부하를 깎아준다 — 보상 카드로 이 값을 올리면 가드가 잘 버틴다
	const float ReducedDamage = FMath::Max(GuardDamage - GetGuardBreakResistance(), 0.f);

	const float NewGuardGauge = FMath::Clamp(GetGuardGauge() + ReducedDamage, 0.f, GetMaxGuardGauge());
	SetGuardGauge(NewGuardGauge);

	UE_LOG(LogTemp, Warning, TEXT("[Guard] 누적 결과 — 감쇄후=%.1f 게이지=%.1f / %.1f"), ReducedDamage, NewGuardGauge, GetMaxGuardGauge());

	// 가드 자연 감소 지연 타이머 리셋 — 실제로 게이지가 증가했을 때만, 마지막으로 막아낸 시점부터 유예시간 이후 감소가 재개된다.
	// GE의 Stacking(Refresh on Successful Application)이 Duration을 자동 리셋하므로 재적용만으로 충분하다.
	if (ReducedDamage > 0.f)
	{
		if (UACAbilitySystemComponent* ACTargetASC = Cast<UACAbilitySystemComponent>(TargetASC))
		{
			if (ACTargetASC->GuardDecayDelayEffectClass)
			{
				const UGameplayEffect* DecayDelayGE = ACTargetASC->GuardDecayDelayEffectClass->GetDefaultObject<UGameplayEffect>();
				ACTargetASC->ApplyGameplayEffectToSelf(DecayDelayGE, 1, ACTargetASC->MakeEffectContext());
			}
		}
	}

	if (NewGuardGauge < GetMaxGuardGauge())
	{
		return;
	}

	// 최대치 도달 — 게이지를 비우고 Block 어빌리티에 가드 브레이크를 알린다
	SetGuardGauge(0.f);

#if AC_WEB_DEBUG
	if (UACWebDebugSubsystem* WebDebug = UACWebDebugSubsystem::Get(Data.Target.GetAvatarActor()))
	{
		WebDebug->RecordMarker(Data.Target.GetAvatarActor(), TEXT("GuardBroken"));
	}
	if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(Data.Target.GetAvatarActor()))
	{
		RunLog->NotifyGuardBreak(Data.Target.GetAvatarActor());
	}
#endif

	FGameplayEventData Payload;
	Payload.EventTag = ACGameplayTags::Shared_Event_GuardBrokenTriggered;
	Payload.Target = Data.Target.GetAvatarActor();

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Data.Target.GetAvatarActor(),
		ACGameplayTags::Shared_Event_GuardBrokenTriggered,
		Payload
		);
}

void UACAttributeSet::HandleDamageAndTriggerHitReact(const FGameplayEffectModCallbackData& Data)
{
	UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();

	// 무적 태그 보유시 데미지 무효화
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Invincible))
	{
#if AC_WEB_DEBUG
		// 무적으로 완전히 흘려낸 타격 — 회피 성공(i-frame) 표본
		if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(Data.Target.GetAvatarActor()))
		{
			RunLog->NotifyIframeNegatedHit();
		}
#endif
		SetDamageTaken(0.f);
		return;
	}

	// 이미 사망 상태라면 중복 처리 방지
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead))
	{
		SetDamageTaken(0.f);
		return;
	}

	const float DamageDone = GetDamageTaken();
	SetDamageTaken(0.f);

	if (DamageDone <= 0.f)
	{
		return;
	}

	const float HealthPctBeforeHit = GetMaxHealth() > 0.f ? GetHealth() / GetMaxHealth() : 0.f;
	const float NewHealth = FMath::Clamp(GetHealth() - DamageDone, 0.f, GetMaxHealth());
	SetHealth(NewHealth);

#if AC_WEB_DEBUG
	// 웹 디버그 타임라인 피격 기록 — 공격 성질 태그·출처 어빌리티·방향을 함께 남긴다
	{
		AActor* HitTarget = Data.Target.GetAvatarActor();
		AActor* HitInstigator = Data.EffectSpec.GetEffectContext().GetInstigator();
		const FGameplayTagContainer AttackTags = Data.EffectSpec.GetDynamicAssetTags();

		FACWebDebugHitMeta Meta;
		for (const FGameplayTag& Tag : AttackTags)
		{
			Meta.AttackTags.Add(Tag.ToString());
		}
		if (const UGameplayAbility* SourceAbility = Data.EffectSpec.GetEffectContext().GetAbility())
		{
			Meta.SourceAbility = SourceAbility->GetClass()->GetName();
		}
		float AngleDifference = 0.f;
		const FGameplayTag DirectionTag = UACFunctionLibrary::ComputeHitReactDirectionTag(HitInstigator, HitTarget, AngleDifference);
		// "Shared.Status.HitReact.Front" 에서 마지막 조각만 남긴다
		Meta.Direction = TEXT("Front");
		if (DirectionTag.IsValid())
		{
			FString TagPath = DirectionTag.ToString();
			int32 LastDot = INDEX_NONE;
			if (TagPath.FindLastChar(TEXT('.'), LastDot))
			{
				Meta.Direction = TagPath.RightChop(LastDot + 1);
			}
		}
		// 체간/가드 피해는 별도 GE 실행으로 들어오므로 메타 어트리뷰트는 이 시점에 이미 비어 있다.
		// 공격자가 실어 보낸 SetByCaller 값을 그대로 읽어야 실제 수치가 남는다.
		Meta.PostureDamage = Data.EffectSpec.GetSetByCallerMagnitude(ACGameplayTags::Shared_SetByCaller_PostureDamage, false, 0.f);
		Meta.GuardDamage = Data.EffectSpec.GetSetByCallerMagnitude(ACGameplayTags::Shared_SetByCaller_GuardDamage, false, 0.f);
		Meta.bWasBlocked = TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Player_Status_Blocking);
		Meta.bWasParried = TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Parry);

		if (UACWebDebugSubsystem* WebDebug = UACWebDebugSubsystem::Get(HitTarget))
		{
			WebDebug->RecordHit(HitTarget, DamageDone, Meta);
		}
		if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(HitTarget))
		{
			RunLog->NotifyDamageTaken(HitTarget, DamageDone, AttackTags, Meta.SourceAbility, HealthPctBeforeHit);
			RunLog->NotifyPlayerHealthPct(GetMaxHealth() > 0.f ? GetHealth() / GetMaxHealth() : 0.f);
		}
	}
#endif

	// 사망 판정
	if (GetHealth() <= 0.f)
	{
		UACFunctionLibrary::AddGameplayTagToActorIfNone(Data.Target.GetAvatarActor(), ACGameplayTags::Shared_Status_Dead);

#if AC_WEB_DEBUG
		if (UACWebDebugSubsystem* WebDebug = UACWebDebugSubsystem::Get(Data.Target.GetAvatarActor()))
		{
			const bool bPlayerDied = UACWebDebugSubsystem::ResolveSource(Data.Target.GetAvatarActor()) == EACWebDebugSource::Player;
			WebDebug->RecordMarker(Data.Target.GetAvatarActor(), bPlayerDied ? TEXT("Death") : TEXT("BossDeath"));
			if (bPlayerDied)
			{
				// 사망 시 링 버퍼 전체를 파일로 남긴다 — 게임을 껐다 켜도 마지막 60초를 다시 볼 수 있다
				WebDebug->SaveDeathSnapshot();
			}
		}
#endif

		// 사망 이벤트 전송 (Death Drop Ability 트리거용)
		FGameplayEventData DeathPayload;
		DeathPayload.EventTag = ACGameplayTags::Shared_Event_Death;
		DeathPayload.Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
		DeathPayload.Target = Data.Target.GetAvatarActor();

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Data.Target.GetAvatarActor(), ACGameplayTags::Shared_Event_Death, DeathPayload);

		return;
	}

	// HitReact 면역 태그 초기화 (최초 1회)
	// Shared_Status_Executed: 처형 중 HitReact 이벤트 발송 차단
	static FGameplayTagContainer HitReactImmunityTags;
	if (HitReactImmunityTags.IsEmpty())
	{
		HitReactImmunityTags.AddTag(ACGameplayTags::Shared_Status_Invincible);
		HitReactImmunityTags.AddTag(ACGameplayTags::Shared_Status_SuperArmor);
		HitReactImmunityTags.AddTag(ACGameplayTags::Shared_Status_Executed);
		HitReactImmunityTags.AddTag(ACGameplayTags::Shared_Status_PostureBroken);
		// 가드 브레이크 몽타주 재생 중에는 같은 히트로 발생하는 일반 HitReact가 끼어들어 덮어쓰지 않도록 막는다.
		// 가드 브레이크는 EndAbility로 Player_Status_Blocking을 즉시 제거하므로, 그 직후 적용되는 데미지의
		// HitReact 이벤트가 (Blocking 태그 소실로) 차단되지 않고 새어 들어와 GuardBreakMontage를 밀어내는 문제였다.
		HitReactImmunityTags.AddTag(ACGameplayTags::Player_Status_GuardBroken);
	}

	// Enemy가 실제로 Block에 성공한 피격은 일반 HitReact 이벤트를 보내지 않는다
	// — HitReact 몽타주가 Block 몽타주를 중단시켜 Block 어빌리티가 풀리는 것을 막는다.
	// 실제 성공 여부는 데미지 감쇄(ACCalculation_DamageTaken)와 동일한 공통 판정(IsSuccessfulBlock)을 사용하므로,
	// 뒤/측면 공격이나 Unblockable 공격, 공격자 불명(Attacker=nullptr)인 피격은 기존처럼 HitReact가 발송된다.
	// Enemy.Status.Blocking 게이트는 Player에게 부여되지 않는 태그이므로 Player HitReact 흐름에는 영향이 없다
	// (Player는 기존처럼 UACPlayerGameplayAbility_HitReact의 ActivationBlockedTags가 Blocking 중 HitReact를 막는다).
	const AActor* HitAttacker = Data.EffectSpec.GetEffectContext().GetInstigator();
	const bool bEnemyBlockedHit = TargetASC
		&& TargetASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_Status_Blocking)
		&& UACFunctionLibrary::IsSuccessfulBlock(HitAttacker, Data.Target.GetAvatarActor(), Data.EffectSpec.GetDynamicAssetTags());

	//  HitReact 차단
	if (TargetASC && !bEnemyBlockedHit && !TargetASC->HasAnyMatchingGameplayTags(HitReactImmunityTags))
	{
		FGameplayEventData HitPayload;
		HitPayload.EventTag = ACGameplayTags::Shared_Event_HitReact;
		HitPayload.Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
		HitPayload.Target = Data.Target.GetAvatarActor();
		HitPayload.EventMagnitude = DamageDone;
		// 공격이 실어 보낸 속성 태그(Notify가 채운 CurrentAttackDefenseTags)를 그대로 넘겨, HitReact가 타격 무게에 따라 몽타주를 고를 수 있게 한다
		HitPayload.InstigatorTags = Data.EffectSpec.GetDynamicAssetTags();

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Data.Target.GetAvatarActor(), ACGameplayTags::Shared_Event_HitReact, HitPayload);
	}
}

void UACAttributeSet::HandleStaminaConsumption(const FGameplayEffectModCallbackData& Data)
{
	const float Cost = GetStaminaCost();
	SetStaminaCost(0.f); // 메타 초기화

	if (Cost <= 0.f)
	{
		return;
	}

	// Stamina 실제 감소
	const float NewStamina = FMath::Clamp(GetStamina() - Cost, 0.f, GetMaxStamina());
	SetStamina(NewStamina);

	// 딜레이 GE 재적용 — GE의 Stacking(Refresh on Successful Application)이 Duration을 자동 리셋
	UACAbilitySystemComponent* ASC = Cast<UACAbilitySystemComponent>(GetOwningAbilitySystemComponent());
	if (!ASC || !ASC->StaminaRegenDelayEffectClass)
	{
		return;
	}

	const UGameplayEffect* DelayGE = ASC->StaminaRegenDelayEffectClass->GetDefaultObject<UGameplayEffect>();
	ASC->ApplyGameplayEffectToSelf(DelayGE, 1, ASC->MakeEffectContext());
}

void UACAttributeSet::HandleBurnBuildUp(const FGameplayEffectModCallbackData& Data)
{
	const float BuildUp = GetBurnAccumulation();
	SetBurnAccumulation(0.f);

	if (BuildUp <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();

	// 이미 사망 상태라면 화상 누적 불필요
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead))
	{
		return;
	}

	const float NewBurnGauge = FMath::Clamp(GetBurnGauge() + BuildUp, 0.f, GetMaxBurnGauge());
	SetBurnGauge(NewBurnGauge);

	if (NewBurnGauge >= GetMaxBurnGauge())
	{
		// 화상 게이지 초기화 후 DoT 어빌리티 트리거
		SetBurnGauge(0.f);

		FGameplayEventData Payload;
		Payload.EventTag = ACGameplayTags::Shared_Event_BurnTriggered;
		Payload.Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
		Payload.Target = Data.Target.GetAvatarActor();

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Data.Target.GetAvatarActor(),
			ACGameplayTags::Shared_Event_BurnTriggered,
			Payload
			);
	}
}