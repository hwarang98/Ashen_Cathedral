// 한 스테이지(보스 아레나 레벨 1개)의 변하지 않는 구성을 담는 DataAsset — 런타임 진행 상태는 UACRunStateSubsystem이 소유한다

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Enums/ACEnums.h"
#include "ACDataAsset_StageDefinition.generated.h"

/**
 * @brief 보스 1체와 그 전용 아레나 레벨을 묶는 스테이지 정의.
 *
 * 보스는 LevelAsset이 가리키는 레벨에 미리 배치하는 것이 유일한 방식이다 — 런타임 스폰 경로는 없다.
 * 이 에셋은 순수 데이터이며 런타임 진행 상태(현재 인덱스·클리어 여부)를 갖지 않는다.
 */
UCLASS(BlueprintType)
class ASHEN_CATHEDRAL_API UACDataAsset_StageDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 로그와 에셋 검증에서 이 스테이지를 식별하는 이름. 한 RunDefinition 안에서 중복될 수 없다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	FName StageID;

	// 이 스테이지의 아레나 레벨. 반드시 소프트 참조여야 RunDefinition을 로드할 때 모든 맵이 함께 로드되지 않는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	TSoftObjectPtr<UWorld> LevelAsset;

	// 이 레벨에 배치되어 있어야 하는 보스의 식별 태그. AACEnemyCharacter::BossIdentityTag와 대조된다.
	// UACDataAsset_BossReward::BossID(MetaProgression.BossID.*)와는 다른 네임스페이스이므로 혼동하지 말 것
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage", meta = (Categories = "Enemy.Boss"))
	FGameplayTag ExpectedBossID;

	// 이 스테이지를 클리어했을 때 보스 보상에 곱할 배수. 깊이 들어갈수록 커지게 두면 앞 스테이지 반복 파밍이 손해가 된다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage", meta = (ClampMin = "0.0"))
	float RewardMultiplier = 1.f;

	// 클리어 후 허용할 진행 선택지. 마지막 스테이지에는 AutoNextStage를 쓸 수 없다(이어질 스테이지가 없으므로)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	EACStageExitPolicy ExitPolicy = EACStageExitPolicy::NormalChoice;

	// AutoNextStage일 때 보상 카드 선택이 끝난 뒤 다음 레벨로 넘어가기까지 기다릴 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage", meta = (ClampMin = "0.0", EditCondition = "ExitPolicy == EACStageExitPolicy::AutoNextStage", EditConditionHides))
	float AutoAdvanceDelay = 1.5f;

	// 이 스테이지를 런의 클리어 수(ClearedBossCount) 집계에 포함할지 여부.
	// 집계에만 영향하며 스테이지 순서·HasNextStage·IsCurrentStageFinal 판정에는 관여하지 않는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	bool bCountsAsRunStage = true;
};
