// 플레이어가 피격되었을 때 카메라 셰이크를 재생하는 히트리액트 어빌리티

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "ACPlayerGameplayAbility_HitReact.generated.h"

class UCameraShakeBase;

/**
 * @brief Shared_Event_HitReact 이벤트로 트리거되며, 플레이어 카메라 셰이크를 재생한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerGameplayAbility_HitReact : public UACPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UACPlayerGameplayAbility_HitReact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	/** 피격 시 재생할 카메라 셰이크 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CameraShake", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UCameraShakeBase> HitCameraShakeClass;
};
