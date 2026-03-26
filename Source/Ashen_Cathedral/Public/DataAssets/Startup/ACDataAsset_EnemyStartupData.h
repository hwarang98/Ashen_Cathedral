// Enemy 전용 시작 데이터 — 기본 StartupData에 BehaviorTree 참조를 추가

#pragma once

#include "CoreMinimal.h"
#include "ACDataAsset_StartupDataBase.h"
#include "ACDataAsset_EnemyStartupData.generated.h"

class UBehaviorTree;

UCLASS()
class ASHEN_CATHEDRAL_API UACDataAsset_EnemyStartupData : public UACDataAsset_StartupDataBase
{
	GENERATED_BODY()

public:
	virtual void GiveToAbilitySystemComponent(UACAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1) override;

	FORCEINLINE UBehaviorTree* GetBehaviorTreeAsset() const { return BehaviorTreeAsset; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData", meta = (TitleProperty = "InputTag"))
	TArray<TSubclassOf<UACGameplayAbility>> EnemyGameplayAbility;
	// AI가 실행할 Behavior Tree 에셋 — ACEnemyController에서 RunBehaviorTree()로 전달
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
};