// 적 전용 무기 데이터 — 전투 스탯만 보유

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "ACDataAsse_EnemyWeaponData.generated.h"

UCLASS()
class ASHEN_CATHEDRAL_API UACDataAsse_EnemyWeaponData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FDataTableRowHandle WeaponStatRow;
};
