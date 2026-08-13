// 사용 중단 — 보스는 스테이지의 아레나 레벨에 직접 배치하므로 런타임 스폰 경로가 없어졌다.
// 레벨과 BP_BossSpawnPoint 에셋의 참조를 정리한 뒤 이 파일을 삭제할 것. 그 전에 지우면 배치된 액터가 깨진다.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "ACBossSpawnPoint.generated.h"

/**
 * 사용 중단된 마커 액터. 보스를 런타임에 스폰하던 시절 스폰 위치를 지정하는 데 쓰였으나,
 * 이제는 스테이지마다 전용 아레나 레벨에 보스를 직접 배치하므로 참조하는 코드가 없다.
 * 배치된 액터와 BP 에셋을 정리한 뒤 이 클래스를 삭제한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACBossSpawnPoint : public ATargetPoint
{
	GENERATED_BODY()
};
