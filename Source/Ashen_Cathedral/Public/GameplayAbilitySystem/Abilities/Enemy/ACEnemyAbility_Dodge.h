// Enemy 전용 닷지 어빌리티 — BT가 Enemy.Event.Dodge 이벤트로 방향을 전달해 활성화

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyGameplayAbility.h"
#include "ACEnemyAbility_Dodge.generated.h"

class UAbilityTask_PlayMontageAndWait;

UCLASS()
class ASHEN_CATHEDRAL_API UACEnemyAbility_Dodge : public UACEnemyGameplayAbility
{
	GENERATED_BODY()

public:
	UACEnemyAbility_Dodge();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 전방 닷지 몽타주 — EventMagnitude 0 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage")
	TObjectPtr<UAnimMontage> ForwardDodgeMontage;

	/** 후방 닷지 몽타주 — EventMagnitude 1 (기본값) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage")
	TObjectPtr<UAnimMontage> BackDodgeMontage;

	/** 좌측 닷지 몽타주 — EventMagnitude 2 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage")
	TObjectPtr<UAnimMontage> LeftDodgeMontage;

	/** 우측 닷지 몽타주 — EventMagnitude 3 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montage")
	TObjectPtr<UAnimMontage> RightDodgeMontage;

	/** 닷지 중 무적 여부를 결정하는 GameplayEffect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> InvincibilityEffect;

private:
	/** BT가 SendGameplayEvent로 전달하는 4방향 닷지 의도 */
	enum class EDodgeDirection : uint8
	{
		Forward  = 0,
		Backward = 1,
		Left     = 2,
		Right    = 3
	};

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	FActiveGameplayEffectHandle InvincibilityEffectHandle;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	/** EventMagnitude 값을 EDodgeDirection으로 변환 (범위 외 값은 Backward로 처리) */
	static EDodgeDirection ParseDirection(float EventMagnitude);

	/** 방향에 해당하는 몽타주 반환 */
	UAnimMontage* SelectMontage(EDodgeDirection Direction) const;

	/** 무적 GameplayEffect를 자신에게 적용 */
	void ApplyInvincibilityEffect();

	/** 몽타주 태스크를 생성하고 실행 */
	void PlayDodgeMontage(UAnimMontage* Montage);
};
