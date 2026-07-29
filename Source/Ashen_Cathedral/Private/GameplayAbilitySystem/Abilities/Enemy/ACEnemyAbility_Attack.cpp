// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyAbility_Attack.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ACGameplayTags.h"
#include "Engine/World.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

UACEnemyAbility_Attack::UACEnemyAbility_Attack()
{
	// 적은 콤보 횟수에 따른 데미지 증가 없음
	bApplyComboDamageBonus = false;

	ActivationOwnedTags.AddTag(ACGameplayTags::Enemy_Status_Attacking);
}

void UACEnemyAbility_Attack::HandleComboComplete()
{
	// BT가 공격 타이밍을 제어하므로 콤보 카운트 리셋을 하지 않는다.
	// HandleComboCancelled는 베이스 클래스(즉시 리셋)를 그대로 사용
}

UAnimMontage* UACEnemyAbility_Attack::SelectAttackMontage()
{
	if (MontageSelectionMode == EACAttackMontageSelectionMode::ComboSequence)
	{
		return SelectComboSequenceMontage();
	}

	if (AttackMontages.IsEmpty())
	{
		return nullptr;
	}

	if (MontageSelectionMode == EACAttackMontageSelectionMode::Sequential)
	{
		// 배열이 런타임에 줄어들어도 안전하도록 나머지 연산으로 인덱스를 보정한다
		const int32 Index = NextSequentialMontageIndex % AttackMontages.Num();
		NextSequentialMontageIndex = (Index + 1) % AttackMontages.Num();
		return AttackMontages[Index];
	}

	const int32 RandomIndex = FMath::RandRange(0, AttackMontages.Num() - 1);
	return AttackMontages[RandomIndex];
}

bool UACEnemyAbility_Attack::HasAnyAttackMontage() const
{
	if (MontageSelectionMode == EACAttackMontageSelectionMode::ComboSequence)
	{
		// 콤보 모드는 AttackMontages를 쓰지 않으므로 ComboSequences 쪽에 유효한 콤보가 있는지로 판단한다
		for (const FACComboSequence& Sequence : ComboSequences)
		{
			if (!Sequence.Montages.IsEmpty())
			{
				return true;
			}
		}

		return false;
	}

	return Super::HasAnyAttackMontage();
}

void UACEnemyAbility_Attack::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// 콤보는 스텝마다 재활성화되므로 여기서 걸면 다음 스텝이 쿨다운에 막혀 콤보가 끊긴다.
	// 콤보 한 벌이 끝나는 시점에 SelectComboSequenceMontage가 직접 적용한다.
	if (MontageSelectionMode == EACAttackMontageSelectionMode::ComboSequence)
	{
		return;
	}

	Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
}

UAnimMontage* UACEnemyAbility_Attack::SelectComboSequenceMontage()
{
	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;

	// 마지막 스텝 이후 간격이 벌어졌으면 콤보가 끊긴 것으로 보고 새로 뽑는다.
	// StateTree가 스텝마다 어빌리티를 취소·재활성화하므로 '취소' 자체는 끊김 신호로 쓸 수 없다.
	const bool bComboExpired = ComboResetTime > 0.f && (Now - LastComboStepTime) > ComboResetTime;

	if (!ComboSequences.IsValidIndex(ActiveComboIndex) || bComboExpired)
	{
		ActiveComboIndex = PickComboSequenceIndex();
		NextComboStepIndex = 0;
	}

	if (!ComboSequences.IsValidIndex(ActiveComboIndex))
	{
		return nullptr;
	}

	const TArray<TObjectPtr<UAnimMontage>>& Montages = ComboSequences[ActiveComboIndex].Montages;
	if (!Montages.IsValidIndex(NextComboStepIndex))
	{
		return nullptr;
	}

	UAnimMontage* MontageToPlay = Montages[NextComboStepIndex];
	++NextComboStepIndex;
	LastComboStepTime = Now;

	// 남은 스텝이 있는 동안만 진행 중 태그를 유지한다 — StateTree의 콤보 계속 상태가 이 태그로 진입 여부를 판단한다
	const bool bHasRemainingStep = Montages.IsValidIndex(NextComboStepIndex);
	UpdateComboInProgressTag(bHasRemainingStep);

	// 마지막 스텝까지 꺼냈으면 다음 활성화에서 새 콤보를 뽑도록 초기화하고, 이 시점에 쿨다운을 건다.
	// 여기서 걸어야 마지막 타 이후의 재진입이 차단되어 콤보가 자연스럽게 종료된다.
	if (!bHasRemainingStep)
	{
		ActiveComboIndex = INDEX_NONE;
		NextComboStepIndex = 0;

		// ApplyCooldown 오버라이드가 스텝 단위 적용을 막고 있으므로 부모 구현을 직접 호출한다
		Super::ApplyCooldown(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
	}

	return MontageToPlay;
}

void UACEnemyAbility_Attack::UpdateComboInProgressTag(bool bInProgress)
{
	if (!ComboInProgressTag.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 스텝마다 호출되므로 Add가 아닌 Set으로 덮어써 카운트가 누적되지 않게 한다
		ASC->SetLooseGameplayTagCount(ComboInProgressTag, bInProgress ? 1 : 0);
	}
}

int32 UACEnemyAbility_Attack::PickComboSequenceIndex() const
{
	// 몽타주가 없는 콤보는 후보에서 제외한다
	float TotalWeight = 0.f;
	for (const FACComboSequence& Sequence : ComboSequences)
	{
		if (!Sequence.Montages.IsEmpty())
		{
			TotalWeight += FMath::Max(Sequence.Weight, 0.f);
		}
	}

	// 가중치가 전부 0이면 유효한 첫 콤보를 사용한다 (전부 0이라고 아무것도 못 고르면 안 되므로)
	if (TotalWeight <= 0.f)
	{
		for (int32 Index = 0; Index < ComboSequences.Num(); ++Index)
		{
			if (!ComboSequences[Index].Montages.IsEmpty())
			{
				return Index;
			}
		}

		return INDEX_NONE;
	}

	float Roll = FMath::FRandRange(0.f, TotalWeight);
	for (int32 Index = 0; Index < ComboSequences.Num(); ++Index)
	{
		if (ComboSequences[Index].Montages.IsEmpty())
		{
			continue;
		}

		Roll -= FMath::Max(ComboSequences[Index].Weight, 0.f);
		if (Roll <= 0.f)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

#if WITH_EDITOR
void UACEnemyAbility_Attack::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RefreshComboChanceDisplay();
}

void UACEnemyAbility_Attack::PostLoad()
{
	Super::PostLoad();

	RefreshComboChanceDisplay();
}

void UACEnemyAbility_Attack::RefreshComboChanceDisplay()
{
	// 실제 선택에 쓰이는 것과 동일한 기준으로 합계를 낸다 (몽타주 없는 콤보는 후보에서 제외)
	float TotalWeight = 0.f;
	for (const FACComboSequence& Sequence : ComboSequences)
	{
		if (!Sequence.Montages.IsEmpty())
		{
			TotalWeight += FMath::Max(Sequence.Weight, 0.f);
		}
	}

	for (FACComboSequence& Sequence : ComboSequences)
	{
		if (Sequence.Montages.IsEmpty())
		{
			Sequence.Chance = TEXT("몽타주 없음 — 선택 안 됨");
		}
		else if (TotalWeight <= 0.f)
		{
			Sequence.Chance = TEXT("가중치 합 0 — 첫 콤보로 폴백");
		}
		else
		{
			Sequence.Chance = FString::Printf(TEXT("%.1f%%"), FMath::Max(Sequence.Weight, 0.f) / TotalWeight * 100.f);
		}
	}
}
#endif

void UACEnemyAbility_Attack::ModifyDamageSpec(const FGameplayEffectSpecHandle& SpecHandle, const AActor* HitActor, float BaseDamage)
{
	const UACAbilitySystemComponent* OwnerASC = GetACAbilitySystemComponentFromActorInfo();
	if (!OwnerASC || !OwnerASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_State_Phase2))
	{
		return;
	}

	// TODO: 난이도 시스템 연동 필요
	const float DifficultyLevel = GetAbilityLevel();

	// 화염 추가 데미지: BaseDamage * ScalableFloat(DifficultyLevel)
	{
		const float Multiplier = FireBonusDamageMultiplierCurve.GetValueAtLevel(DifficultyLevel);
		if (Multiplier > 0.f)
		{
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_FireBonusDamage, BaseDamage * Multiplier);
		}
	}

	// 화상 축적량: ScalableFloat(DifficultyLevel)
	{
		const float BurnBuildUp = BurnBuildUpAmountCurve.GetValueAtLevel(DifficultyLevel);
		if (BurnBuildUp > 0.f)
		{
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_BurnBuildUp, BurnBuildUp);
		}
	}

	// 체간 데미지 배율: 기존 Spec의 PostureDamage × ScalableFloat(DifficultyLevel)
	{
		const float PostureMultiplier = PostureDamageMultiplierCurve.GetValueAtLevel(DifficultyLevel);
		if (PostureMultiplier > 0.f)
		{
			const float CurrentPostureDamage = SpecHandle.Data->GetSetByCallerMagnitude(ACGameplayTags::Shared_SetByCaller_PostureDamage, false, 0.f);
			if (CurrentPostureDamage > 0.f)
			{
				UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, ACGameplayTags::Shared_SetByCaller_PostureDamage, CurrentPostureDamage * PostureMultiplier);
			}
		}
	}
}