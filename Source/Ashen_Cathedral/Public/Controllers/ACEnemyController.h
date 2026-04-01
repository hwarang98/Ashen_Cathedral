// Enemy 캐릭터를 제어하는 AI 컨트롤러 — Behavior Tree 실행 진입점 역할

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ACEnemyController.generated.h"

struct FAIStimulus;
class UAISenseConfig_Sight;
class AACEnemyCharacter;

UCLASS()
class ASHEN_CATHEDRAL_API AACEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	AACEnemyController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	/**
	 * @brief 다른 Actor에 대한 Team Attitude를 평가한다.
	 *
	 * @param Other 평가 대상 Actor
	 * @return 대상 Actor에 대한 Team의 태도
	 * @note AI의 팀 기반 행동 결정에 사용한다.
	 */
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	/** AI가 감지한 키값을 TargetActor라는 키 값으로 블랙보드에 저장 */
	UFUNCTION()
	virtual void OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAIPerceptionComponent> EnemyPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAISenseConfig_Sight> AISenseConfig_Sight;

private:
	// 현재 제어 중인 Enemy 캐릭터에 대한 캐시 참조
	UPROPERTY()
	TObjectPtr<AACEnemyCharacter> CachedEnemyCharacter;
};