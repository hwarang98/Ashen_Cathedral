// 플레이어가 피격되었을 때 카메라 셰이크를 재생하는 히트리액트 어빌리티

#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility_HitReact.h"
#include "ACGameplayTags.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/PlayerController.h"

UACPlayerGameplayAbility_HitReact::UACPlayerGameplayAbility_HitReact()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = ACGameplayTags::Shared_Event_HitReact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UACPlayerGameplayAbility_HitReact::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (HitCameraShakeClass)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(ActorInfo->PlayerController.Get()))
		{
			PlayerController->ClientStartCameraShake(HitCameraShakeClass);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}