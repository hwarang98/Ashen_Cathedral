// Enemy 캐릭터의 전투 상태를 GAS 태그 기반으로 추적하는 애니메이션 인스턴스

#pragma once

#include "CoreMinimal.h"
#include "AnimInstance/ACAnimInstanceBase.h"
#include "ACEnemyAnimInstance.generated.h"

UCLASS()
class ASHEN_CATHEDRAL_API UACEnemyAnimInstance : public UACAnimInstanceBase
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// GAS 태그(Shared_Status_Groggy)로 그로기 상태 여부를 추적
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Combat")
	bool bIsGroggy = false;

	// GAS 태그(Shared_Status_Dead)로 사망 상태 여부를 추적
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Combat")
	bool bIsDead = false;
};
