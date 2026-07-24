// 폰의 ASC가 특정 GameplayTag를 보유했는지 검사하는 StateTree 조건 — 내장 Has Tag와 달리 TagContainer 바인딩이 필요 없다

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "GameplayTagContainer.h"
#include "ACSTCondition_HasGameplayTag.generated.h"

class AAIController;

/** FACSTCondition_HasGameplayTag 용 */
USTRUCT()
struct FACSTCondition_HasGameplayTagInstanceData
{
	GENERATED_BODY()

	/** 태그를 검사할 폰을 소유한 컨트롤러. 스키마가 보장하는 AIController 컨텍스트를 바인딩한다 */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController = nullptr;
};

/**
 * @brief AIController가 조종하는 폰의 ASC가 지정한 GameplayTag를 보유했는지 검사한다.
 *
 * 엔진 내장 Has Tag 조건은 검사 대상 TagContainer를 별도로 바인딩해야 하지만,
 * 이 조건은 AIController 컨텍스트에서 ASC를 직접 읽으므로 Tag만 지정하면 된다.
 * 쿨다운(Enemy.Cooldown.*)이나 상태 태그(Enemy.Status.*) 검사에 사용한다.
 */
USTRUCT(meta = (DisplayName = "Has Gameplay Tag (Enemy)", Category = "AI|Condition"))
struct ASHEN_CATHEDRAL_API FACSTCondition_HasGameplayTag : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FACSTCondition_HasGameplayTagInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/**
	 * @brief 폰의 ASC에서 Tag 보유 여부를 검사한다.
	 *
	 * @return bInvert가 false면 태그를 보유했을 때 true.
	 * @note ASC를 못 찾거나 Tag가 비어 있으면 '보유하지 않음'으로 처리한다(= bInvert 값이 그대로 결과가 된다).
	 */
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

private:
	/** 보유 여부를 검사할 태그 (예: Enemy.Cooldown.Attack) */
	UPROPERTY(EditAnywhere, Category = "Condition")
	FGameplayTag Tag;

	/** true면 정확히 일치하는 태그만 인정한다. false면 하위 태그도 매칭된다 */
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bExactMatch = false;

	/** true면 결과를 뒤집는다 — "이 태그가 없을 때 통과"시키려면 켠다 (쿨다운 검사용) */
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bInvert = false;
};
