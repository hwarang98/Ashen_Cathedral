// Enemy 캐릭터를 제어하는 AI 컨트롤러 — Behavior Tree 실행 진입점 역할

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"
#include "ACEnemyController.generated.h"

struct FAIStimulus;
struct FGameplayTag;
struct FGameplayEventData;
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

	/** Enemy.State.PressureReady 태그 추가/제거를 Blackboard의 bPressureResponseRequested로 동기화한다 */
	void OnPressureReadyTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** Shared.Event.Combat.IncomingAttack 수신 시 상태 가드를 통과하면 Blackboard에 예고 정보를 기록한다 */
	void OnIncomingAttackEventReceived(const FGameplayEventData* Payload);

	/** IncomingAttack 예고 키가 실제 히트 타이밍 이후에도 남지 않도록 지운다 */
	void ClearIncomingAttackBlackboard(int32 ExpectedWarningId);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAIPerceptionComponent> EnemyPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAISenseConfig_Sight> AISenseConfig_Sight;

	UPROPERTY(EditDefaultsOnly, Category = "Incoming Attack", meta = (ClampMin = "0.0"))
	float IncomingAttackBlackboardGraceTime = 0.15f;

private:
	void ResetIncomingAttackBlackboard();

	// 현재 제어 중인 Enemy 캐릭터에 대한 캐시 참조
	UPROPERTY()
	TObjectPtr<AACEnemyCharacter> CachedEnemyCharacter;

	FTimerHandle IncomingAttackClearTimerHandle;
	int32 IncomingAttackWarningId = 0;
};
