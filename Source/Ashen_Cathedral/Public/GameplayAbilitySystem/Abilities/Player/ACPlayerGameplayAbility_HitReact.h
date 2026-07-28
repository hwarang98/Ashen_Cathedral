// 플레이어가 피격되었을 때 방향별 몽타주와 카메라 셰이크를 재생하는 히트리액트 어빌리티

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "Structs/ACStructTypes.h"
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

	/**
	 * 강한 공격에 피격됐을 때 방향별 몽타주 대신 재생할 몽타주 목록. 위에서부터 검사해 처음 일치하는 항목을 사용한다.
	 * 비워두면 항상 방향별 몽타주를 재생한다(기존 동작).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage|Weight", meta = (AllowPrivateAccess = "true"))
	TArray<FACHitReactWeightMontage> WeightHitReactMontages;

	/** 공격에 실려 온 속성 태그로 대형 히트리액트 몽타주를 찾는다. 해당 항목이 없으면 nullptr */
	UAnimMontage* SelectWeightHitReactMontage(const FGameplayTagContainer& AttackTags) const;

	TSubclassOf<UGameplayEffect> UnderAttackEffect;

	/** 피격 시 재생할 카메라 셰이크 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CameraShake", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UCameraShakeBase> HitCameraShakeClass;

	UFUNCTION()
	void OnMontageEnded();

	UFUNCTION()
	void OnMontageCancelled();
};