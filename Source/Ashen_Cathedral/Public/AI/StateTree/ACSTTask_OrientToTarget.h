// 매 틱 타겟 액터 방향으로 부드럽게 회전하는 StateTree 태스크 — BT의 ACBTService_OrientToTargetActor를 대체한다

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "AI/StateTree/ACSTTaskInstanceData.h"
#include "ACSTTask_OrientToTarget.generated.h"

/**
 * @brief Combat 그룹에 상시로 붙여, 자식이 Chase든 Melee든 상관없이 타겟을 계속 바라보게 한다.
 *
 * @note Task를 State에 추가한 뒤 Details 패널에서 Considered For Completion 체크를 해제해야 한다.
 *       그래야 이 태스크가 계속 Running이어도 State 자체는 자식 완료로 정상 진행된다.
 */
USTRUCT(meta = (DisplayName = "Orient To Target", Category = "AI|Action"))
struct ASHEN_CATHEDRAL_API FACSTTask_OrientToTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FACSTTask_OrientToTargetInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	// 회전 보간 속도. BT 원본과 동일하게 5.0을 기본값으로 사용한다
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float RotationInterpSpeed = 5.f;
};
