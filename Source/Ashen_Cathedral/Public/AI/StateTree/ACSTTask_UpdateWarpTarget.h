// 타겟 액터의 위치를 모션 워핑 타겟으로 주기적으로 갱신하는 StateTree 태스크 — BT의 BTService_MotionWarpingUpdateTarget을 대체한다

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "AI/StateTree/ACSTTaskInstanceData.h"
#include "ACSTTask_UpdateWarpTarget.generated.h"

/**
 * @brief Combat 그룹에 붙여, 공격 몽타주의 MotionWarping NotifyState가 참조할 워프 타겟을 주기적으로 갱신한다.
 *
 * @note WarpTargetName은 공격 몽타주의 AnimNotifyState_MotionWarping에 지정된 이름과 일치해야 한다.
 *       Task를 State에 추가한 뒤 Details 패널에서 Considered For Completion 체크를 해제해야 한다.
 */
USTRUCT(meta = (DisplayName = "Update Warp Target", Category = "AI|Action"))
struct ASHEN_CATHEDRAL_API FACSTTask_UpdateWarpTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FACSTTask_UpdateWarpTargetInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	/** 타겟 위치와 그쪽을 바라보는 회전을 워프 타겟으로 등록한다 */
	void UpdateWarpTarget(const FInstanceDataType& InstanceData) const;

	// 공격 몽타주의 MotionWarping NotifyState에 지정된 워프 타겟 이름
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName WarpTargetName = "AttackTarget";

	// 갱신 주기(초). BT 서비스의 Interval과 동일하게 0.2를 기본값으로 사용한다
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0", Units = "s"))
	float UpdateInterval = 0.2f;
};
