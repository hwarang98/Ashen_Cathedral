// Enemy 전용 닷지 어빌리티 — BT가 Enemy.Event.Dodge 이벤트로 방향을 전달해 활성화

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyGameplayAbility.h"
#include "Enums/ACEnums.h"
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

	/** 회피 중 부여할 무적 GE(Shared.Status.Invincible 등). 비워두면 무적 없이 기존과 동일하게 동작한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Dodge|Effects")
	TSubclassOf<UGameplayEffect> InvincibilityEffect;

private:
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	// 적용한 무적 GE 핸들. 회피가 어떻게 끝나든 EndAbility에서 제거해 무적이 남지 않게 한다.
	FActiveGameplayEffectHandle InvincibilityEffectHandle;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	/** EventMagnitude 값을 EACDodgeDirection으로 변환 (범위 외 값은 Backward로 처리) */
	static EACDodgeDirection ParseDirection(float EventMagnitude);

	/** 방향에 해당하는 몽타주 반환 */
	UAnimMontage* SelectMontage(EACDodgeDirection Direction) const;

	/** 몽타주 태스크를 생성하고 실행 */
	void PlayDodgeMontage(UAnimMontage* Montage);
};
