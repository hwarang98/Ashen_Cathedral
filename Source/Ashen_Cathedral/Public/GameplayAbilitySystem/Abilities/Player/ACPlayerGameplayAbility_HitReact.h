// 플레이어가 피격되었을 때 방향별 몽타주와 카메라 셰이크를 재생하는 히트리액트 어빌리티

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "ACPlayerGameplayAbility_HitReact.generated.h"

class UCameraShakeBase;
class UGameplayEffect;

/**
 * @brief Shared_Event_HitReact 이벤트로 트리거되며, 공격 방향에 따른 몽타주와 카메라 셰이크를 재생한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerGameplayAbility_HitReact : public UACPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UACPlayerGameplayAbility_HitReact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage|HitReact", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> FrontHitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage|HitReact", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> LeftHitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage|HitReact", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> RightHitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage|HitReact", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> BackHitReactMontage;

	// 블록 중 피격 시 방향별 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage|Block", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> BlockHitReactMontage;

	TSubclassOf<UGameplayEffect> UnderAttackEffect;

	/** 피격 시 재생할 카메라 셰이크 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CameraShake", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UCameraShakeBase> HitCameraShakeClass;

	UFUNCTION()
	void OnMontageEnded();

	UFUNCTION()
	void OnMontageCancelled();
};