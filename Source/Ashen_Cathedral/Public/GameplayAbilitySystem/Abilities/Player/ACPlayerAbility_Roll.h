// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "ACPlayerAbility_Roll.generated.h"

class UAbilityTask_PlayMontageAndWait;

UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerAbility_Roll : public UACPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UACPlayerAbility_Roll();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	/** 비무장 상태 롤링 몽타주 맵 (방향별로 설정) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage")
	TMap<ERollDirection, TObjectPtr<UAnimMontage>> RollMontages;

	/** 무기 장착 상태 Dodge 몽타주 맵 (방향별로 설정) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage")
	TMap<ERollDirection, TObjectPtr<UAnimMontage>> DodgeMontages;

	/** 기본 롤링 거리 (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll Settings")
	float DefaultRollDistance = 400.0f;

	/** 롤링 중 무적 여부를 결정하는 GameplayEffect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> InvincibilityEffect;

	/** 모션 워핑 타겟 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion Warping")
	FName WarpTargetName = "RollTarget";

	/** 라인 트레이스에 사용할 오브젝트 타입 (WorldStatic, WorldDynamic 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll Settings")
	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes;

	/** 라인 트레이스 디버그 드로잉 활성화 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roll Settings")
	bool bDebugDrawTrace = false;

private:
	/** 현재 재생 중인 몽타주 태스크 */
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	/** 적용된 무적 이펙트 핸들 */
	FActiveGameplayEffectHandle InvincibilityEffectHandle;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	/**
	 * @brief 캐시된 입력과 카메라 기준으로 8방향 롤 방향과 월드 방향 벡터를 동시에 계산
	 *
	 * @param PlayerCharacter 방향 계산에 사용할 플레이어 캐릭터
	 * @param OutDirection 계산된 8방향 롤 방향
	 * @param OutWorldDir 스냅된 방향을 월드 스페이스로 변환한 벡터
	 */
	void ResolveRollMovement(const AACPlayerCharacter* PlayerCharacter, ERollDirection& OutDirection, FVector& OutWorldDir) const;

	/** 무기 장착 상태에 따라 재생할 몽타주를 선택 */
	UAnimMontage* SelectMontage(ERollDirection Direction, bool bIsWeaponEquipped) const;

	/** 무적 GameplayEffect를 자신에게 적용 */
	void ApplyInvincibilityEffect();

	/** 몽타주 태스크를 생성하고 실행 */
	void PlayRollMontage(UAnimMontage* Montage);

	/** 입력 벡터를 기반으로 롤링 방향 계산 */
	static ERollDirection CalculateRollDirection(const FVector2D& InputVector);

	/**
	 * @brief 라인 트레이스로 장애물과 절벽을 감지하여 안전한 롤링 거리를 계산
	 *
	 * @param StartLocation 롤링 시작 위치
	 * @param Direction 롤링 월드 방향 (정규화된 벡터)
	 * @return 장애물 및 절벽을 고려한 안전한 이동 거리 (cm)
	 */
	float CalculateSafeRollDistance(const FVector& StartLocation, const FVector& Direction) const;

	/** 모션 워핑 타겟 설정 */
	void SetupMotionWarping(const FVector& TargetLocation) const;
};