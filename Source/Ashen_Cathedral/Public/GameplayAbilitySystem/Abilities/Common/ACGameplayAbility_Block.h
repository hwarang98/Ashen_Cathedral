// 플레이어·적 공용 Block 어빌리티 베이스 — GE 적용·제거와 몽타주 멤버를 제공한다.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"
#include "ACGameplayAbility_Block.generated.h"

class UAbilityTask_PlayMontageAndWait;

UCLASS()
class ASHEN_CATHEDRAL_API UACGameplayAbility_Block : public UACGameplayAbility
{
	GENERATED_BODY()

public:
	UACGameplayAbility_Block();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	/** 블록 중 재생할 애니메이션 몽타주 */
	UPROPERTY(EditDefaultsOnly, Category = "Block|Animation")
	TObjectPtr<UAnimMontage> BlockMontage;

	/** 블록 중 이동 속도를 제한하는 GE */
	UPROPERTY(EditDefaultsOnly, Category = "Block|Effects")
	TSubclassOf<UGameplayEffect> MoveSpeedEffect;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	FActiveGameplayEffectHandle MoveSpeedEffectHandle;
};
