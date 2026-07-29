// StateTree 태스크들의 InstanceData struct를 한곳에 모은 헤더 — 각 태스크 헤더는 이 파일을 include해 자신의 FInstanceDataType으로 사용한다

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "ACSTTaskInstanceData.generated.h"

class AAIController;
class UAbilitySystemComponent;

/** FACSTTask_Idle 용 — 상태 유지 외 별도 데이터가 없다 */
USTRUCT()
struct FACSTTask_IdleInstanceData
{
	GENERATED_BODY()
};

/** FACSTTask_OrientToTarget 용 */
USTRUCT()
struct FACSTTask_OrientToTargetInstanceData
{
	GENERATED_BODY()

	/** 회전시킬 폰을 소유한 컨트롤러. 스키마가 보장하는 AIController 컨텍스트를 바인딩한다 */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	/** 바라볼 대상. 보통 AIController.TargetActor를 바인딩한다 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<AActor> TargetActor = nullptr;
};

/** FACSTTask_UpdateWarpTarget 용 */
USTRUCT()
struct FACSTTask_UpdateWarpTargetInstanceData
{
	GENERATED_BODY()

	/** 워프 컴포넌트를 가진 폰을 소유한 컨트롤러. 스키마가 보장하는 AIController 컨텍스트를 바인딩한다 */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	/** 워프 목표로 삼을 대상. 보통 AIController.TargetActor를 바인딩한다 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** 마지막 갱신 이후 경과 시간 누적 */
	float TimeSinceLastUpdate = 0.f;
};

/** FACSTTask_ActivateAbilityByTag 용 */
USTRUCT()
struct FACSTTask_ActivateAbilityByTagInstanceData
{
	GENERATED_BODY()

	/** 어빌리티를 활성화할 대상. 스키마가 보장하는 AIController 컨텍스트를 바인딩한다 */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	FGameplayAbilitySpecHandle AbilityHandle;

	/** WaitOwnedTag 사용 시 등록하는 태그 변경 델리게이트 핸들. ExitState에서 해제한다 */
	FDelegateHandle TagChangedHandle;

	/** 어빌리티 종료 델리게이트 핸들. WaitOwnedTag 사용 여부와 무관하게 등록한다 */
	FDelegateHandle AbilityEndedHandle;

	/** MaxWaitTime 타임아웃 타이머 */
	FTimerHandle TimeoutTimerHandle;
};

/** FACSTTask_SendGameplayEvent 용 */
USTRUCT()
struct FACSTTask_SendGameplayEventInstanceData
{
	GENERATED_BODY()

	/** 이벤트를 보낼 대상의 폰을 소유한 컨트롤러. 스키마가 보장하는 AIController 컨텍스트를 바인딩한다 */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** WaitOwnedTag가 사라질 때까지 대기할 때 등록하는 델리게이트 핸들. ExitState에서 해제한다 */
	FDelegateHandle TagChangedHandle;

	/** MaxWaitTime 타임아웃 타이머 */
	FTimerHandle TimeoutTimerHandle;
};
