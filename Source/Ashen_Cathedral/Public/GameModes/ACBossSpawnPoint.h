// 레벨에 배치해 GameMode가 첫 보스를 스폰할 위치를 지정하는 마커 액터

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "ACBossSpawnPoint.generated.h"

/**
 * 보스를 더 이상 레벨에 직접 배치하지 않을 때, AACGameMode::StartPlay()가
 * 이 액터의 Transform을 찾아 NextBossSequence의 첫 보스를 스폰하는 데 사용한다.
 * 레벨에 다른 용도의 TargetPoint와 혼동되지 않도록 별도 타입으로 분리했다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACBossSpawnPoint : public ATargetPoint
{
	GENERATED_BODY()
};
