// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"
#include "ACGameplayAbility_Death.generated.h"

/**
 *
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACGameplayAbility_Death : public UACGameplayAbility
{
	GENERATED_BODY()

public:
	UACGameplayAbility_Death();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 사망 애니메이션 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Animation")
	TArray<TObjectPtr<UAnimMontage>> DeathMontages;

	/** 사망 후 캐릭터를 파괴하기까지의 지연 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death|Config")
	float DestroyDelay = 5.0f;

private:
	/** 몽타주 종료 타이머 핸들 — 자동 블렌드아웃 비활성화 시 OnCompleted/OnBlendOut이 호출되지 않으므로
	 *  몽타주 재생 길이만큼 대기 후 OnMontageCompleted를 직접 호출함 */
	FTimerHandle MontageEndTimerHandle;

	/** 파괴 타이머 핸들 — 몽타주 종료 후 DestroyDelay 뒤에 액터를 파괴함 */
	FTimerHandle DestroyTimerHandle;

	/** 캐릭터 비활성화 처리 (콜리전, AI, 무기, 인터페이스 호출, 델리게이트 브로드캐스트) */
	void HandleDeath() const;

	/** 몽타주가 정상 종료(BlendOut/Completed)됐을 때 호출 — 파괴 타이머 시작 후 어빌리티 종료 */
	UFUNCTION()
	void OnMontageCompleted();

	/** 몽타주가 중단(Cancelled/Interrupted)됐을 때 호출 — 어빌리티 즉시 종료 */
	UFUNCTION()
	void OnMontageCancelled();

	/** 타이머에서 호출 — 액터 파괴 */
	UFUNCTION()
	void HandleCharacterDestruction();
};
