// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Common/ACAbility_Attack.h"
#include "ACPlayerAbility_Attack.generated.h"

// TODO: 패링 관련 헤더 추가 예정

/**
 * 플레이어 전용 공격 어빌리티.
 * 콤보 완료 후 일정 시간 내 입력이 없으면 콤보 카운트를 리셋한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerAbility_Attack : public UACAbility_Attack
{
	GENERATED_BODY()

public:
	UACPlayerAbility_Attack();

	/** 콤보 완료 후 리셋 타이머를 시작한다. */
	virtual void HandleComboComplete() override;

	/** 콤보 취소 시 타이머를 정리하고 즉시 리셋한다. */
	virtual void HandleComboCancelled() override;

	/** 스태미나가 0 이하면 공격을 차단한다. */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

private:
	/** 콤보 윈도우 유지 시간 (초). 이 시간 안에 입력이 없으면 콤보가 리셋된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo", meta = (AllowPrivateAccess = "true"))
	float ComboResetDelay = 2.0f;

	FTimerHandle ComboResetTimerHandle;

	/** 콤보 윈도우 타이머 만료 시 호출 - 로깅 후 콤보 리셋 */
	void OnComboResetTimerExpired();

	// TODO: 패링 성공 여부 감지 및 카운터 어택 태그 부여 로직 구현 예정
};
