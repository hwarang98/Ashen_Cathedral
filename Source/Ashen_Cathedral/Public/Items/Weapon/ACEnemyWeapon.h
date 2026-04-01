// 적 전용 무기 — 전투 데이터만 보유하며 플레이어 전용 필드를 포함하지 않음

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapon/ACWeaponBase.h"
#include "ACEnemyWeapon.generated.h"

class UACDataAsse_EnemyWeaponData;

UCLASS()
class ASHEN_CATHEDRAL_API AACEnemyWeapon : public AACWeaponBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponData")
	TObjectPtr<UACDataAsse_EnemyWeaponData> WeaponData;
};
