// Enemy 전용 전투 컴포넌트 — 무기 데이터 기반 데미지 계산 담당

#pragma once

#include "CoreMinimal.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "EnemyCombatComponent.generated.h"

UCLASS()
class ASHEN_CATHEDRAL_API UEnemyCombatComponent : public UPawnCombatComponent
{
	GENERATED_BODY()

public:
	virtual float GetCurrentWeaponBaseDamage() const override;
	virtual float GetCurrentWeaponHeavyAttackGroggyDamage() const override;
	virtual float GetCurrentWeaponCounterAttackGroggyDamage() const override;
	virtual float GetCurrentWeaponAttackSpeed() const override;

private:
	// 현재 장착 무기의 DataTable Row를 반환
	const struct FACWeaponStatRow* GetCurrentWeaponStatRow() const;
};
