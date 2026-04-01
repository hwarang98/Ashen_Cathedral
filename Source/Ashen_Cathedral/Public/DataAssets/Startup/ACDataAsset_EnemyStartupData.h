// Enemy 전용 시작 데이터 — 어빌리티 부여 담당

#pragma once

#include "CoreMinimal.h"
#include "ACDataAsset_StartupDataBase.h"
#include "ACDataAsset_EnemyStartupData.generated.h"

UCLASS()
class ASHEN_CATHEDRAL_API UACDataAsset_EnemyStartupData : public UACDataAsset_StartupDataBase
{
	GENERATED_BODY()

public:
	virtual void GiveToAbilitySystemComponent(UACAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData", meta = (TitleProperty = "InputTag"))
	TArray<TSubclassOf<UACGameplayAbility>> EnemyGameplayAbility;
};