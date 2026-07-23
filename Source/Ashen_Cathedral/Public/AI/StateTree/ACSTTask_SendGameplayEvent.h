// EventMagnitude에 방향 등을 실어 GameplayEvent를 발송하는 StateTree 태스크 — BT의 BTTask_SendGameplayEvent를 대체한다 (닷지 방향 전달 등에 사용)

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameplayTagContainer.h"
#include "Enums/ACEnums.h"
#include "AI/StateTree/ACSTTaskInstanceData.h"
#include "ACSTTask_SendGameplayEvent.generated.h"

/**
 * @brief AIController가 조종하는 폰에게 닷지 방향을 실은 GameplayEvent를 발송한다.
 *
 * @note 이벤트 트리거형 어빌리티(Enemy.Event.Dodge)를 켜고, DodgeDirection을 EventMagnitude로 전달한다.
 *       WaitOwnedTag를 지정하면 그 태그가 사라질 때까지 상태를 Running으로 유지한다(닷지 몽타주 종료 대기 등).
 *       bRandomLeftRight가 켜지면 DodgeDirection을 무시하고 좌/우 중 무작위로 보낸다(스트레이핑 중 좌우 닷지 랜덤 등).
 */
USTRUCT(meta = (DisplayName = "Send Gameplay Event", Category = "AI|Action"))
struct ASHEN_CATHEDRAL_API FACSTTask_SendGameplayEvent : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FACSTTask_SendGameplayEventInstanceData;

	FACSTTask_SendGameplayEvent();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	/** 정리: 대기 델리게이트와 타임아웃 타이머를 해제한다 */
	void CleanupWait(FInstanceDataType& InstanceData) const;

	// 발송할 이벤트 태그 (예: Enemy.Event.Dodge)
	UPROPERTY(EditAnywhere, Category = "Event")
	FGameplayTag EventTag;

	// 보낼 닷지 방향. EventMagnitude로 변환되어 전달된다
	UPROPERTY(EditAnywhere, Category = "Event", meta = (EditCondition = "!bRandomLeftRight"))
	EACDodgeDirection DodgeDirection = EACDodgeDirection::Forward;

	// true면 DodgeDirection을 무시하고 좌/우 중 무작위로 보낸다 (스트레이핑 중 좌우 닷지 랜덤)
	UPROPERTY(EditAnywhere, Category = "Event")
	bool bRandomLeftRight = false;

	// 설정하면 이 태그가 ASC에서 사라질 때까지 상태를 Running으로 유지한다 (예: Enemy.Status.Dodging)
	UPROPERTY(EditAnywhere, Category = "Wait", meta = (Categories = "Enemy.Status"))
	FGameplayTag WaitOwnedTag;

	// 0보다 크면 이 시간(초)이 지나도 태그가 남아 있을 때 강제 종료한다. 0이면 무제한 대기
	UPROPERTY(EditAnywhere, Category = "Wait", meta = (ClampMin = "0.0", Units = "s"))
	float MaxWaitTime = 0.f;
};
