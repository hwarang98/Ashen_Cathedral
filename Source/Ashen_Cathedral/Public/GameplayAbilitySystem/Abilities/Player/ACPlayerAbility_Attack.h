// 플레이어 전용 공격 어빌리티 — 스태미나 조건 확인, 콤보 리셋 타이머, 즉시 콤보 체인 처리

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Common/ACAbility_Attack.h"
#include "ACPlayerAbility_Attack.generated.h"

class UACAbilitySystemComponent;

/**
 * 플레이어 전용 공격 어빌리티.
 * 몽타주가 자연 완료되면 콤보 리셋 타이머를 시작하고, 플레이어가 공격 중 재입력하면
 * TriggerComboChain()을 통해 현재 몽타주를 즉시 중단하고 다음 콤보로 전환한다.
 * bParticipatesInSharedCombo를 끄면 공유 콤보에 참여하지 않는 단발성 공격(스페셜)으로 동작하며,
 * 피니셔 후반의 SpecialLinkWindow 구간에서 TryTriggerSpecialAttack()으로 연결된다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerAbility_Attack : public UACAbility_Attack
{
	GENERATED_BODY()

public:
	UACPlayerAbility_Attack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 스태미나가 0 이하면 공격을 차단한다. */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	/** 몽타주 자연 완료 후 콤보 리셋 타이머를 시작한다. 피니셔와 스페셜은 타이머 없이 즉시 공유 상태를 초기화한다. */
	virtual void HandleComboComplete() override;

	/**
	 * @brief 공유 콤보 상태를 기준으로 몽타주를 선택한다. 공유 콤보에 참여하는 Light/Heavy가 단계를 공유한다.
	 *
	 * @return 선택된 몽타주. AttackMontages가 비어 있으면 nullptr
	 * @note 피니셔 예약(bSharedComboFinisherReady)이 걸려 있으면 배열 길이와 무관하게 마지막 몽타주를 고르고,
	 *       그렇지 않으면 SharedComboCount를 [0, LastIndex]로 Clamp해 고른다. 선택 결과에 따라 다음 입력의
	 *       피니셔 예약/현재 피니셔 재생 여부와 SharedComboCount를 갱신한다.
	 *       bParticipatesInSharedCombo가 false면 공유 상태를 건드리지 않고 등록된 몽타주 중 하나를 랜덤으로 재생한다.
	 */
	virtual UAnimMontage* SelectAttackMontage() override;

	/** 인스턴스 로컬 카운트 대신 실제로 선택된 몽타주 단계(1-기반)를 데미지 계산에 전달한다. */
	virtual int32 GetComboDamageCount() const override;

	/**
	 * @brief 콤보 취소 처리.
	 * bComboChaining 플래그가 true이면 ResetComboCount를 건너뛴다.
	 * TriggerComboChain()이 EndAbility(true) 직전에 이 플래그를 설정한다.
	 */
	virtual void HandleComboCancelled() override;

	/**
	 * @brief 어빌리티가 활성 중일 때 즉시 콤보 체인을 트리거한다.
	 * EndAbility(true) 완료 직후 같은 프레임 안에서 InputTag에 해당하는 다음 어빌리티를 동기 활성화한다.
	 * Input_AbilityInputPressed에서 외부 호출용.
	 *
	 * @param InputTag 재활성화할 어빌리티를 찾는 데 사용하는 입력 태그 (LightAttack 또는 HeavyAttack)
	 */
	void TriggerComboChain(const FGameplayTag& InputTag);

	/**
	 * @brief 활성 중인 공격에서 단발성 스페셜 공격으로 전환한다.
	 *
	 * @param InputTag 활성화할 스페셜 어빌리티를 찾는 데 사용하는 입력 태그 (InputTag.SpecialWeaponAbility.*)
	 * @return 입력을 소비했으면 true. 창이 닫혀 있거나 사전 검증에 실패해 아무 일도 하지 않았으면 false
	 * @note 피니셔 재생 중에는 Player.Status.SpecialLinkWindow가, 일반 공격 중에는 기존 Player.Status.ComboWindow가
	 *       열려 있을 때만 전환한다. 현재 어빌리티를 종료하기 전에 Spec 존재/쿨다운/비용을 먼저 검사하므로,
	 *       스페셜을 쓸 수 없는 상황에서 진행 중인 피니셔가 끊기지 않는다.
	 */
	bool TryTriggerSpecialAttack(const FGameplayTag& InputTag);

	/** 스페셜 무기 어빌리티 입력(InputTag.SpecialWeaponAbility.*)인지 판정한다. */
	static bool IsSpecialAttackInputTag(const FGameplayTag& InputTag);

protected:
	/**
	 * false이면 이 어빌리티는 공유 콤보에 참여하지 않는다 — AttackMontages를 콤보 단계가 아니라 랜덤 변형으로 쓰고,
	 * 피니셔 단계 계산에 관여하지 않으며, 콤보 단계 데미지 보너스와 콤보 리셋 타이머도 사용하지 않는다.
	 * 단발성 스페셜 공격 어빌리티에서 체크 해제한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	bool bParticipatesInSharedCombo = true;

private:
	void OnComboResetTimerExpired();

	/** 스태미나가 남아 있는지 확인한다. CanActivateAbility와 스페셜 전환 사전 검증이 공유한다. */
	static bool HasAnyStamina(const FGameplayAbilityActorInfo* ActorInfo);

	/**
	 * TriggerComboChain()이 EndAbility 직전에 true로 설정한다.
	 * HandleComboCancelled에서 ResetComboCount를 건너뛰는 용도로만 사용한다.
	 */
	bool bComboChaining = false;

	/**
	 * SelectAttackMontage()가 실제로 고른 몽타주의 1-기반 단계. 0이면 선택 없음(카운터 어택 포함).
	 * 매 활성화마다 공유 상태에서 새로 유도하므로 이전 콤보의 값이 다음 콤보에 남지 않는다.
	 */
	int32 SelectedComboStage = 0;
};
