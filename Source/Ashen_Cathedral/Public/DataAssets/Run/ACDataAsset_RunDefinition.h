// 한 Run 전체의 스테이지 구성과 로비 목적지를 담는 DataAsset — 런타임 진행 상태는 UACRunStateSubsystem이 소유한다

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ACDataAsset_RunDefinition.generated.h"

class UACDataAsset_StageDefinition;

/**
 * @brief 로비에서 시작해 순서대로 진행할 스테이지 목록을 정의한다.
 *
 * AACBattleStartPoint가 이 에셋을 들고 있다가 전투 시작 시 GameMode로 넘긴다.
 * UACRunStateSubsystem은 OrderedStages를 직접 진행하지 않고 BeginRun 시점에 복사본을 만들어 사용하므로,
 * 향후 랜덤 스테이지 순서를 지원해도 이 에셋은 런타임에 변경되지 않는다.
 */
UCLASS(BlueprintType)
class ASHEN_CATHEDRAL_API UACDataAsset_RunDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 로그와 에셋 검증에서 이 Run 구성을 식별하는 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run")
	FName RunID;

	// 진행 순서대로 나열한 스테이지. 비어 있거나 원소가 하나라도 비어 있으면 런이 시작되지 않는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run", meta = (TitleProperty = "StageID"))
	TArray<TObjectPtr<UACDataAsset_StageDefinition>> OrderedStages;

	// 런을 정산하거나 포기했을 때 돌아갈 로비 레벨. 비어 있으면 GameMode의 FallbackLobbyLevel이 대신 쓰인다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Run")
	TSoftObjectPtr<UWorld> LobbyLevel;
};
