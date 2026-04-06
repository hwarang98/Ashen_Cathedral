// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyGameplayAbility.h"
#include "Structs/ACStructTypes.h"
#include "ACGameplayAbility_AshenKnight_Phase2.generated.h"

class UNiagaraComponent;
class AACWeaponBase;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class AAIController;

/**
 * Ashen Knight Phase2 진입 어빌리티.
 *
 * - ActivationOwnedTags: Enemy.State.Phase2 → 활성화 중 ASC에 태그 부여
 * - ActivationBlockedTags: Enemy.State.Phase2 → 태그가 이미 있으면 재실행 차단
 * - Phase2는 되돌아가지 않으므로 EndAbility는 외부에서 직접 호출하지 않는다.
 *   스탯 GE는 Infinite로 영구 적용되며, Material / Niagara도 복구하지 않는다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACGameplayAbility_AshenKnight_Phase2 : public UACEnemyGameplayAbility
{
	GENERATED_BODY()

public:
	UACGameplayAbility_AshenKnight_Phase2();

	/**
	 * @brief Phase2 상태를 활성화한다.
	 * 스탯 GE 즉시 적용 → 몽타주 재생(무적·AI 정지) → 이벤트 수신 후 비주얼 적용 순으로 처리한다.
	 * Phase2는 되돌아가지 않으므로 EndAbility를 호출하지 않는다.
	 *
	 * @param Handle           어빌리티 스펙 핸들
	 * @param ActorInfo        어빌리티 소유 액터 정보
	 * @param ActivationInfo   활성화 컨텍스트 정보
	 * @param TriggerEventData 트리거 이벤트 페이로드
	 */
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	/** Phase2 스탯 강화 GE (Infinite). BP에서 GE_AshenKnight_Phase2Stats 할당. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase2|Effects", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> Phase2StatsEffect;

	/**
	 * Phase2 전환 몽타주. 이 몽타주의 AnimNotify에서 VisualActivateEventTag 이벤트를 발송해야 합니다.
	 * None으로 설정하면 몽타주 없이 즉시 비주얼을 적용합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase2|Montage", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> Phase2TransitionMontage;

	/**
	 * AnimNotify_SendGameplayEvent가 발송할 이벤트 태그.
	 * 기본값: Enemy.Event.Phase2.VisualActivate
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase2|Montage", meta = (AllowPrivateAccess = "true"))
	FGameplayTag VisualActivateEventTag;

	/**
	 * 캐릭터 메시 슬롯별 화염 머티리얼 배열.
	 * 배열 인덱스 = 메시의 머티리얼 슬롯 인덱스.
	 * [0]=cloth [1]=Arm [2]=Greaves [3]=Body [4]=cape
	 * 특정 슬롯을 None으로 남겨두면 해당 슬롯은 원본 머티리얼을 유지합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase2|Visual", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UMaterialInterface>> FireMaterials;

	/** 무기 메시 슬롯에 적용할 화염 머티리얼. None이면 무기 머티리얼 교체를 건너뜁니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase2|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> FireMaterialWeapon;

	/** 캐릭터 Skeletal Mesh 소켓에 부착할 Niagara 이펙트 목록. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase2|Visual", meta = (AllowPrivateAccess = "true"))
	TArray<FACPhase2NiagaraAttachment> NiagaraAttachments;

	/** 무기 Static/Skeletal Mesh 소켓에 부착할 Niagara 이펙트 목록. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase2|Visual", meta = (AllowPrivateAccess = "true"))
	TArray<FACPhase2NiagaraAttachment> NiagaraAttachmentsWeapon;

	/** Phase2 스탯 GE를 소유 ASC에 Infinite 타입으로 적용한다. */
	void ApplyPhase2StatsEffect(const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo) const;

	/** 이벤트 수신 시 호출. 머티리얼 교체 + Niagara 부착을 수행한다. */
	UFUNCTION()
	void OnVisualActivateEventReceived(FGameplayEventData Payload);

	/** 몽타주 완료/중단/취소 시 호출. 무적 태그 제거 + AI BT 재개. */
	UFUNCTION()
	void OnPhase2MontageEnded();

	/** 소유 캐릭터의 AIController를 반환한다. 플레이어 캐릭터나 컨트롤러 없을 경우 nullptr. */
	AAIController* GetOwningAIController() const;

	/** 캐릭터 메시의 슬롯을 FireMaterials 배열 인덱스 순서로 교체합니다. */
	void ApplyFireMaterial(USkeletalMeshComponent* Mesh) const;

	/** 무기 메시의 슬롯을 FireMaterialWeapon으로 덮어씁니다. */
	void ApplyFireMaterialToWeapon(AACWeaponBase* Weapon) const;

	/** NiagaraAttachments 목록을 순회하며 캐릭터 Skeletal Mesh 소켓에 루핑 이펙트를 스폰·부착합니다. */
	void AttachNiagaraEffects(USkeletalMeshComponent* Mesh) const;

	/** NiagaraAttachmentsWeapon 목록을 순회하며 무기 메시 소켓에 루핑 이펙트를 스폰·부착합니다. */
	void AttachNiagaraEffectsToWeapon(AACWeaponBase* Weapon) const;
};