// 태그로 Enemy Ability를 활성화하고 종료될 때까지 상태를 유지하는 StateTree 태스크 — BT의 ACBTTask_ActivateAbilityByTagAndWait를 대체한다

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "AI/StateTree/ACSTTaskInstanceData.h"
#include "ACSTTask_ActivateAbilityByTag.generated.h"

class UAbilitySystemComponent;

/**
 * @brief AbilityTagToActivate와 매칭되는 Enemy Ability를 활성화하고, 끝날 때까지 상태를 Running으로 유지한다.
 *
 * @note 틱을 돌지 않는다. 종료 감지는 델리게이트 콜백에서 FinishTask()로 처리하므로,
 *       긴 몽타주가 재생되는 동안 StateTree 컴포넌트가 틱을 멈출 수 있다.
 *       BT 버전(ACBTTask_ActivateAbilityByTagAndWait)이 TickTask로 매 틱 폴링하던 것과의 차이다.
 */
USTRUCT(meta = (DisplayName = "Activate Ability By Tag", Category = "AI|Ability"))
struct ASHEN_CATHEDRAL_API FACSTTask_ActivateAbilityByTag : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FACSTTask_ActivateAbilityByTagInstanceData;

	FACSTTask_ActivateAbilityByTag();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	/** ASC의 활성화 가능 목록에서 AbilityTagToActivate를 모두 가진 첫 어빌리티를 찾는다 */
	FGameplayAbilitySpecHandle FindAbilityHandle(const UAbilitySystemComponent& ASC) const;

	/** WaitOwnedTag가 설정되면 태그 보유 여부로, 아니면 Spec::IsActive()로 진행 여부를 판단한다 */
	bool IsAbilityStillRunning(const FInstanceDataType& InstanceData) const;

	/** 대기를 정리하고 델리게이트·타이머를 해제한다 */
	void CleanupWait(FInstanceDataType& InstanceData) const;

	// 활성화할 Enemy Ability를 식별하는 태그. Ability의 AssetTags와 매칭한다
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Enemy.Ability"))
	FGameplayTagContainer AbilityTagToActivate;

	// 설정하면 GameplayAbilitySpec::IsActive() 대신 이 태그가 ASC에서 사라질 때까지 대기한다
	UPROPERTY(EditAnywhere, Category = "Ability|Wait", meta = (Categories = "Enemy.Status"))
	FGameplayTag WaitOwnedTag;

	// 어빌리티를 찾지 못했거나 활성화가 실패하면 Failed로 끝낼지 여부
	UPROPERTY(EditAnywhere, Category = "Ability")
	bool bFailIfAbilityNotActivated = true;

	// 활성화 직전 AIController->StopMovement()를 호출할지 여부
	UPROPERTY(EditAnywhere, Category = "Ability")
	bool bStopMovementBeforeActivate = false;

	// 0보다 크면 이 시간(초)이 지나도 끝나지 않을 때 강제 종료한다. 0이면 무제한 대기
	UPROPERTY(EditAnywhere, Category = "Ability|Wait", meta = (ClampMin = "0.0", Units = "s"))
	float MaxWaitTime = 0.f;

	// 타임아웃으로 종료될 때 Succeeded로 처리할지 Failed로 처리할지
	UPROPERTY(EditAnywhere, Category = "Ability|Wait")
	bool bSucceedOnTimeout = true;
};
