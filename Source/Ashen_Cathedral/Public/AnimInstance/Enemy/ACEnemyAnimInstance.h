// Enemy 캐릭터의 전투 상태를 GAS 태그 기반으로 추적하는 애니메이션 인스턴스

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AnimInstance/ACAnimInstanceBase.h"
#include "ACEnemyAnimInstance.generated.h"

UCLASS()
class ASHEN_CATHEDRAL_API UACEnemyAnimInstance : public UACAnimInstanceBase
{
	GENERATED_BODY()

public:
	/**
	 * @brief 소유 보스의 식별 태그를 1회 캐시한다.
	 *
	 * @note ASC가 아니라 캐릭터 프로퍼티에서 읽는다. 이 함수는 SkeletalMeshComponent 등록 시점에 실행되어
	 *       캐릭터의 BeginPlay(=ASC에 Loose 태그를 부여하는 지점)보다 먼저 돌기 때문이다.
	 */
	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/**
	 * 소유 보스의 식별 태그(예: Enemy.Boss.Ordan). 공용 Chooser Table의 GameplayTag 컬럼에 바인딩해 보스별 PSD를 고르는 데 사용한다.
	 * FGameplayTagColumn의 입력이 FGameplayTagContainer이므로 단일 태그가 아닌 컨테이너로 보관한다.
	 * 런타임에 바뀌지 않아 초기화 시 한 번만 채우며, 워커 스레드의 Chooser 평가는 이 캐시 값만 읽는다.
	 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Boss")
	FGameplayTagContainer BossIdentityTags;

	// GAS 태그(Shared_Status_PostureBroken)로 체간 붕괴 상태 여부를 추적
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Combat")
	bool bIsPostureBroken = false;

	// GAS 태그(Shared_Status_Dead)로 사망 상태 여부를 추적
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Combat")
	bool bIsDead = false;
};
