// 재사용 가능한 AOE(범위) 판정 컴포넌트 — 오버랩/스윕, 적대 필터링, 지속형 중복 히트 방지를 담당한다.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AOEDamageComponent.generated.h"

/**
 * @brief AOE(범위) 판정 로직만을 캡슐화하는 재사용 가능한 컴포넌트.
 *
 * GameplayEffect 생성/적용은 담당하지 않는다 — 오버랩/스윕으로 찾아낸 적대 대상마다
 * 호출자가 넘긴 OnTargetFound 콜백을 호출해 위임할 뿐이다. 어떤 GameplayAbility에서도
 * (UACAbility_Attack을 상속하지 않아도) FindComponentByClass로 찾아 재사용할 수 있다.
 */
UCLASS(ClassGroup = (Ashen_Cathedral), meta = (BlueprintSpawnableComponent))
class ASHEN_CATHEDRAL_API UAOEDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAOEDamageComponent();

	/**
	 * @brief 1회 Sphere Overlap을 수행해 적대 Pawn을 찾을 때마다 OnTargetFound를 호출한다.
	 *
	 * @param Radius         판정 반경 (cm)
	 * @param ForwardOffset  Owner 전방으로 밀어낼 판정 원점 오프셋 (cm)
	 * @param bDebugDraw     true면 판정 스피어를 디버그로 표시한다.
	 * @param OnTargetFound  자기 자신/중복/ASC 없음/비적대 필터를 통과한 대상마다 호출되는 콜백
	 */
	void TriggerInstantAOE(float Radius, float ForwardOffset, bool bDebugDraw, TFunction<void(AActor*)> OnTargetFound);

	/**
	 * @brief Interval마다 이전 판정 원점→현재 판정 원점을 Sphere로 스윕하는 지속형 AOE 판정을 시작한다.
	 * 이미 진행 중이면 기존 상태를 정리하고 새로 시작한다. 같은 구간 내 동일 대상은 한 번만 콜백된다.
	 *
	 * @param Radius         판정 반경 (cm)
	 * @param ForwardOffset  Owner 전방으로 밀어낼 판정 원점 오프셋 (cm)
	 * @param Interval       스윕 판정을 반복할 간격 (초)
	 * @param bDebugDraw     true면 스윕 경로를 디버그로 표시한다.
	 * @param OnTargetFound  자기 자신/중복/ASC 없음/비적대 필터를 통과한 대상마다 호출되는 콜백
	 */
	void StartSustainedAOE(float Radius, float ForwardOffset, float Interval, bool bDebugDraw, TFunction<void(AActor*)> OnTargetFound);

	/** 지속형 AOE 판정을 종료하고 타이머와 중복 히트 목록을 정리한다. */
	void StopSustainedAOE();

private:
	FVector ComputeAOEOrigin(float ForwardOffset) const;
	void TickSustainedAOE();

	/** CandidateActors 중 자기 자신/중복(DedupSet)/ASC 없음/비적대를 걸러낸 뒤 살아남은 대상마다 OnTargetFound를 호출한다. */
	void BroadcastHostileTargets(const TArray<AActor*>& CandidateActors, TSet<TWeakObjectPtr<AActor>>* DedupSet, const TFunction<void(AActor*)>& OnTargetFound) const;

	float SustainedRadius = 0.f;
	float SustainedForwardOffset = 0.f;
	bool bSustainedDebugDraw = false;
	TFunction<void(AActor*)> SustainedOnTargetFound;

	FVector PreviousAOEOrigin = FVector::ZeroVector;
	FTimerHandle SustainedTickTimerHandle;
	TSet<TWeakObjectPtr<AActor>> SustainedHitActors;
};
