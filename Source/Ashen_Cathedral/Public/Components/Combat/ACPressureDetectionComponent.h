// 짧은 시간 내 다중 피격을 감지해 신호만 발행하는 컴포넌트 — 실제 반격/회피 행동은 이 신호를 구독하는 GAS Ability 또는 AI가 결정한다.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnExtensionComponentBase.h"
#include "ACPressureDetectionComponent.generated.h"

class UAbilitySystemComponent;
struct FGameplayEventData;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ASHEN_CATHEDRAL_API UACPressureDetectionComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()

public:
	UACPressureDetectionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	/** 히트를 누적할 시간 창 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureDetection", meta = (AllowPrivateAccess = "true"))
	float TimeWindow = 3.0f;

	/** TimeWindow 안에 이 횟수 이상 맞으면 압박 상태로 판정한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureDetection", meta = (AllowPrivateAccess = "true"))
	int32 HitThreshold = 5;

	/** true면 그로기(Stagger) 상태의 히트는 감지에서 제외한다 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PressureDetection", meta = (AllowPrivateAccess = "true"))
	bool bIgnoreDuringStagger = true;

private:
	/** Shared.Event.HitReact 이벤트 수신 콜백 */
	void OnHitReactEventReceived(const FGameplayEventData* Payload);

	/** 필터를 통과한 히트를 타임스탬프에 기록하고 임계치를 판정한다 */
	void RecordHit(const AActor* InstigatorActor);

	/** TimeWindow 밖으로 벗어난 타임스탬프를 제거한다 */
	void PruneOldTimestamps();

	/**
	 * @brief 압박 감지 신호를 발행한다.
	 *
	 * Enemy.State.PressureReady 태그를 부여하고 Enemy.Event.PressureDetected GameplayEvent를 발송한다.
	 * 태그 해제는 이 신호를 소비하는 Response Ability(예: ActivateAbility)가 담당한다.
	 * @note 이 함수는 신호 발행까지만 담당하며, 실제 반격/회피 행동은 이 신호를 구독하는 GAS Ability 또는 AI가 결정한다.
	 */
	void TriggerPressureDetected(const AActor* InstigatorActor);

	/** 캐시된 Owner의 AbilitySystemComponent를 반환 (없으면 nullptr) */
	UAbilitySystemComponent* GetOwnerASC() const;

	TArray<float> HitTimestamps;

	FDelegateHandle HitReactEventHandle;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
};
