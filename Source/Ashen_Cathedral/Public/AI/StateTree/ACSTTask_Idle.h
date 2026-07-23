// 아무 동작 없이 상태를 Running으로 유지하는 StateTree 대기 태스크 — 태스크가 하나도 없는 상태는 즉시 완료되어 전이 루프를 유발하므로 대기 상태의 자리를 채운다

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "AI/StateTree/ACSTTaskInstanceData.h"
#include "ACSTTask_Idle.generated.h"

/**
 * @brief 이벤트로 다른 상태로 전이될 때까지 대기하는 태스크.
 *
 * @note EnterState/Tick을 구현하지 않아 기본값인 Running이 유지된다. 틱을 돌지 않으므로 비용이 없고, 태스크 수만 채워 상태가 조기 완료되는 것을 막는다.
 */
USTRUCT(meta = (DisplayName = "Idle", Category = "AI|Action"))
struct ASHEN_CATHEDRAL_API FACSTTask_Idle : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FACSTTask_IdleInstanceData;

	FACSTTask_Idle();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
};
