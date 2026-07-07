// 태그로 Enemy Ability를 활성화하고, 해당 어빌리티가 종료될 때까지 BT를 InProgress로 유지하는 태스크

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "ACBTTask_ActivateAbilityByTagAndWait.generated.h"

struct FActivateAbilityAndWaitTaskMemory;

/**
 * AbilityTagToActivate와 매칭되는 Enemy Ability를 찾아 활성화하고, 해당 어빌리티가 끝날 때까지 태스크를 InProgress로 유지한다.
 * 몽타주 재생 시간이 긴 Enemy Ability(Special Attack 등)와 BT 시퀀스를 동기화하는 데 사용한다.
 *
 * bCreateNodeInstance = false 이므로 상태는 NodeMemory(FActivateAbilityAndWaitTaskMemory)에 저장되며,
 * 여러 AI가 동시에 이 태스크를 실행해도 서로 간섭하지 않는다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACBTTask_ActivateAbilityByTagAndWait : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UACBTTask_ActivateAbilityByTagAndWait();

	/** AbilityTagToActivate와 매칭되는 어빌리티를 찾아 활성화하고, NodeMemory에 대기 상태를 저장한다 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/**
	 * @brief 매 틱마다 어빌리티가 아직 실행 중인지 확인하고, 종료되었거나 타임아웃되면 태스크를 종료한다
	 *
	 * @param OwnerComp 이 태스크를 소유한 BehaviorTree 컴포넌트
	 * @param NodeMemory AI 인스턴스별 상태 메모리 (FActivateAbilityAndWaitTaskMemory)
	 * @param DeltaSeconds 이전 틱으로부터 경과한 시간
	 */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** AI 인스턴스마다 할당할 NodeMemory 크기 반환 (FActivateAbilityAndWaitTaskMemory) */
	virtual uint16 GetInstanceMemorySize() const override;

	/** BT 에디터 노드 박스에 표시할 설명 텍스트 반환 */
	virtual FString GetStaticDescription() const override;

private:
	/** WaitOwnedTag 설정 여부에 따라 태그 존재 또는 GameplayAbilitySpec::IsActive()로 어빌리티 진행 여부를 판단한다 */
	bool IsAbilityStillRunning(const FActivateAbilityAndWaitTaskMemory& Memory) const;

	// 활성화할 Enemy Ability를 식별하는 태그. Ability의 AssetTags(SetAssetTags로 부여)와 매칭한다.
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Enemy.Ability"))
	FGameplayTagContainer AbilityTagToActivate;

	// 설정하면 GameplayAbilitySpec::IsActive() 대신 이 태그가 ASC에서 사라질 때까지 대기한다 (예: Enemy.Status.Attacking)
	UPROPERTY(EditAnywhere, Category = "Ability|Wait", meta = (Categories = "Enemy.Status"))
	FGameplayTag WaitOwnedTag;

	// 어빌리티를 찾지 못했거나 활성화(TryActivateAbility)가 실패하면 Failed를 반환할지 여부
	UPROPERTY(EditAnywhere, Category = "Ability")
	bool bFailIfAbilityNotActivated = true;

	// 활성화 직전 AIController->StopMovement()를 호출할지 여부 (전용 StopMovement BT 노드를 쓴다면 불필요)
	UPROPERTY(EditAnywhere, Category = "Ability")
	bool bStopMovementBeforeActivate = false;

	// 0보다 크면 이 시간(초)이 지나도 어빌리티가 끝나지 않을 때 강제 종료한다. 0이면 무제한 대기
	UPROPERTY(EditAnywhere, Category = "Ability|Wait", meta = (ClampMin = "0.0", Units = "s"))
	float MaxWaitTime = 0.f;

	// 타임아웃으로 종료될 때 Succeeded(true)로 처리할지 Failed(false)로 처리할지
	UPROPERTY(EditAnywhere, Category = "Ability|Wait")
	bool bSucceedOnTimeout = true;
};
