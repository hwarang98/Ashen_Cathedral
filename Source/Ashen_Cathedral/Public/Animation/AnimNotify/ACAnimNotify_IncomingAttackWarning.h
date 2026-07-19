// 공격 판정 발생 전 시점에 배치해 주변 Boss(Enemy)에게 Shared.Event.Combat.IncomingAttack 예고 이벤트를 보내는 AnimNotify.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "ACAnimNotify_IncomingAttackWarning.generated.h"

/**
 * @brief 공격 예고 타이밍을 표시하는 AnimNotify.
 *
 * 실제 히트 판정보다 앞선 시점에 배치한다. 방어/패링 가능 여부(AttackDefenseTags)는
 * 이 Notify 자신이 원본 데이터를 가지며, 같은 Ability라도 몽타주/타이밍마다 다르게
 * 설정할 수 있다. Notify 발생 시 현재 애니메이팅 중인 UACAbility_Attack에 이 값을
 * 저장해 실제 Hit 판정(CreateDamageEffectSpec)에서도 동일한 값을 쓰게 하고, 동시에
 * Boss(Enemy)에게 Shared.Event.Combat.IncomingAttack 이벤트로 예고를 보낸다.
 * 패링/블록 성공을 결정하지 않으며, Boss AI가 반응 여부를 판단할 기회만 제공한다.
 */
UCLASS(meta = (DisplayName = "AN_IncomingAttackWarning"))
class ASHEN_CATHEDRAL_API UACAnimNotify_IncomingAttackWarning : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** 이 타격이 지닌 방어 가능 속성. Shared.Attack.Blockable/Parryable/Unblockable/Unparryable 조합을 담는다. */
	UPROPERTY(EditAnywhere, Category = "IncomingAttack", meta = (Categories = "Shared.Attack"))
	FGameplayTagContainer AttackDefenseTags;

	/** 이 Notify 발생 시점부터 실제 히트까지 예상 시간(초) */
	UPROPERTY(EditAnywhere, Category = "IncomingAttack", meta = (ClampMin = "0.0"))
	float ExpectedHitTime = 0.3f;

	/** BT/AI 판단 확장을 위한 위협도(0~1). 현재 기본 BT 로직에서는 소비하지 않는다. */
	UPROPERTY(EditAnywhere, Category = "IncomingAttack", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ThreatLevel = 0.5f;

	/** 주변 Boss(Enemy)를 탐색할 반경 (cm) */
	UPROPERTY(EditAnywhere, Category = "IncomingAttack")
	float DetectionRadius = 1500.f;
};
