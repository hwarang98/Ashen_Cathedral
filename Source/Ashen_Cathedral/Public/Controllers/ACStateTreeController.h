// StateTree 기반 보스(보스2 이후)를 제어하는 AI 컨트롤러 — Perception/GAS 신호를 StateTree 이벤트로 변환한다

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "ACStateTreeController.generated.h"

struct FAIStimulus;
struct FGameplayEventData;
class UAISenseConfig_Sight;
class UStateTreeAIComponent;
class AACEnemyCharacter;

/** Enemy.StateTree.Event.IncomingAttack 전이 이벤트에 실려 StateTree로 전달되는 공격 예고 정보 */
USTRUCT(BlueprintType)
struct FACIncomingAttackStateTreePayload
{
	GENERATED_BODY()

	/** 공격을 시작한 액터(플레이어) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> Instigator = nullptr;

	/** 실제 히트까지 남은 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeToImpact = 0.f;

	/** 패링 가능한 공격인지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bParryable = false;

	/** 블록 가능한 공격인지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bBlockable = false;
};

/**
 * @brief StateTree로 동작하는 보스 전용 AI 컨트롤러.
 *
 * @note Behavior Tree/Blackboard를 사용하지 않는다. 기존 AACEnemyController(보스1)와는 의도적으로 상속 관계를 두지 않아, 한쪽 변경이 다른 쪽에 전파되지 않는다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACStateTreeController : public AAIController
{
	GENERATED_BODY()

public:
	AACStateTreeController();

	/** StateTree 태스크/평가자가 현재 타겟을 조회할 때 사용한다 */
	UFUNCTION(BlueprintPure, Category = "AI")
	AActor* GetTargetActor() const { return TargetActor; }

	/**
	 * @brief 조우 연출이 끝난 뒤 보스 AI를 시작한다.
	 *
	 * StateTreeAIComponent의 Start Logic Automatically를 꺼 둔 보스(조우 컷신이 있는 보스)에서
	 * 컷신 종료 시점에 호출한다. 이미 실행 중이면 아무것도 하지 않는다.
	 * @note 컷신 동안 Perception이 발송한 TargetAcquired 이벤트는 트리가 꺼져 있어 버려지므로
	 *       (SendStateTreeEvent의 bIsRunning 가드), 잡아둔 타겟이 있으면 여기서 다시 발송한다.
	 *       ST_Aldren의 Combat 진입은 이 이벤트가 필수 조건(Enter Event)이라 재발송이 없으면
	 *       보스가 대기 상태에서 영영 빠져나오지 못한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void StartEncounter();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/**
	 * @brief 다른 Actor에 대한 Team Attitude를 평가한다.
	 *
	 * @param Other 평가 대상 Actor
	 * @return 대상 Actor에 대한 Team의 태도
	 */
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	/** 시야 감지 결과를 TargetActor에 반영하고 획득/상실 이벤트를 StateTree로 발송한다 */
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** Enemy.State.PressureReady 태그가 추가될 때 StateTree로 압박 반응 이벤트를 발송한다 */
	void OnPressureReadyTagChanged(const FGameplayTag Tag, int32 NewCount);

	/**
	 * @brief Shared.Event.Combat.IncomingAttack 수신 시 상태 가드를 통과하면 StateTree로 예고 이벤트를 발송한다.
	 *
	 * @param Payload 공격자/히트까지 남은 시간/패링·블록 가능 여부가 담긴 GAS 이벤트 데이터
	 * @note BT 버전과 달리 값을 걸어두지 않으므로 만료를 지우는 타이머가 필요 없다. 반응 지속 시간은 StateTree 상태가 관리한다.
	 */
	void OnIncomingAttackEventReceived(const FGameplayEventData* Payload);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> BossPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/** 시야 범위(감지 거리) */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float SightRadius = 3000.f;

	/** 시야 상실 범위(추적 해제 거리) */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float LoseSightRadius = 3500.f;

	/** 주변 시야각(도) */
	UPROPERTY(EditDefaultsOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PeripheralVisionAngleDegrees = 180.f;

	/** 이 컨트롤러가 속한 팀 ID. 값이 더 작은 팀을 적대적으로 판정한다 */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	uint8 TeamId = 1;

private:
	/** EditDefaultsOnly로 노출된 시야 값을 SightConfig에 반영한다 */
	void ApplyPerceptionSettings();

	/** 현재 추적 중인 타겟 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> TargetActor;

	/** 현재 제어 중인 Enemy 캐릭터에 대한 캐시 참조 */
	UPROPERTY()
	TObjectPtr<AACEnemyCharacter> CachedEnemyCharacter;
};