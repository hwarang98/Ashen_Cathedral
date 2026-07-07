// Min~Max 범위에서 매번 무작위로 뽑은 시간 동안 브랜치 재실행을 막는 Cooldown Decorator — 엔진 기본 BTDecorator_Cooldown에는 랜덤 범위 옵션이 없어 추가

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "ACBTDecorator_RandomCooldown.generated.h"

struct FACRandomCooldownDecoratorMemory
{
	double LastUseTimestamp;
	float CurrentCooldownDuration;
	uint8 bRequestedRestart : 1;
};

UCLASS(HideCategories = (Condition))
class ASHEN_CATHEDRAL_API UACBTDecorator_RandomCooldown : public UBTDecorator
{
	GENERATED_BODY()

public:
	UACBTDecorator_RandomCooldown();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual void OnNodeDeactivation(FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 쿨다운 최소 시간 (초) */
	UPROPERTY(Category = Decorator, EditAnywhere, meta = (ClampMin = "0.0"))
	float MinCooldownTime = 3.0f;

	/** 쿨다운 최대 시간 (초) */
	UPROPERTY(Category = Decorator, EditAnywhere, meta = (ClampMin = "0.0"))
	float MaxCooldownTime = 6.0f;
};
