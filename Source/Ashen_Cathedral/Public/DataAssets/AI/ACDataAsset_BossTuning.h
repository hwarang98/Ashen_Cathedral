// StateTree 보스 AI의 거리 임계값을 모아둔 DataAsset — 여러 보스가 같은 에셋을 공유해 밸런싱 값을 한 곳에서 관리한다

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ACDataAsset_BossTuning.generated.h"

/**
 * @brief StateTree 보스 AI의 행동 전환 거리 정의 DataAsset.
 *
 * AACStateTreeController의 TuningData 프로퍼티에 할당하고, StateTree의 Distance Compare 조건에서
 * AIController → TuningData → 각 값으로 바인딩해 사용한다.
 *
 * @note StateTree의 External Data로 직접 등록할 수 없다. UStateTreeComponentSchema::IsExternalItemAllowed가
 *       AActor/UActorComponent/UWorldSubsystem만 허용하므로 반드시 컨트롤러를 경유해야 한다.
 */
UCLASS(BlueprintType)
class ASHEN_CATHEDRAL_API UACDataAsset_BossTuning : public UDataAsset
{
	GENERATED_BODY()

public:
	// 이 거리를 넘으면 추적(Chase)에 들어간다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Distance", meta = (ClampMin = "0.0", Units = "cm"))
	float ChaseDistance = 800.f;

	// 이 거리 이하에서 근거리 기동(접근/회피/스트레이핑)을 시작한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Distance", meta = (ClampMin = "0.0", Units = "cm"))
	float StrafeDistance = 700.f;

	// 이 거리 이하에서 근접 공격을 허용한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Distance", meta = (ClampMin = "0.0", Units = "cm"))
	float MeleeDistance = 500.f;

	// 이 거리 이하에서 압박 반격을 허용한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Distance", meta = (ClampMin = "0.0", Units = "cm"))
	float PressureCounterDistance = 350.f;

	// 이 거리 이하에서 패링/블록 반응을 허용한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Distance", meta = (ClampMin = "0.0", Units = "cm"))
	float DefenseDistance = 300.f;

	// 이 거리 이하로 붙으면 후방 회피를 고려한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Distance", meta = (ClampMin = "0.0", Units = "cm"))
	float BackDodgeDistance = 250.f;

	// 대시 공격을 시작하는 최소 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Distance", meta = (ClampMin = "0.0", Units = "cm"))
	float RunAttackMinDistance = 600.f;

	// 대시 공격을 시작하는 최대 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Distance", meta = (ClampMin = "0.0", Units = "cm"))
	float RunAttackMaxDistance = 800.f;

	// 백대시 공격을 시작하는 최대 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Distance", meta = (ClampMin = "0.0", Units = "cm"))
	float BackDashAttackMaxDistance = 150.f;

	// 백대시 공격을 시작하는 최대 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Distance", meta = (ClampMin = "0.0", Units = "cm"))
	float BackDashAttackMinDistance = 0.f;
};