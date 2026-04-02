// 블랙보드의 타겟 액터 방향으로 AI를 부드럽게 회전시키는 BT 태스크 노드

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTaskNode_RotateToFaceTarget.generated.h"

/**
 * 지정한 블랙보드 키의 액터 방향으로 AI 폰을 보간 회전시킨다.
 * 각도 오차가 AnglePrecision 이하가 되면 태스크를 성공으로 종료한다.
 *
 * bCreateNodeInstance = false 이므로 상태는 NodeMemory(FRotateToFaceTargetTaskMemory)에 저장되며,
 * 여러 AI가 동시에 이 태스크를 실행해도 서로 간섭하지 않는다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UBTTaskNode_RotateToFaceTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTaskNode_RotateToFaceTarget();

	/** BT 에셋 로드 시 블랙보드 키 이름을 ID로 바인딩 */
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	/** NodeMemory에 OwningPawn, TargetActor를 저장하고 태스크 시작 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/**
	 * @brief 매 틱마다 타겟 방향으로 회전을 보간하고, 각도 오차가 AnglePrecision 이하가 되면 태스크를 성공 종료한다
	 *
	 * @param OwnerComp 이 태스크를 소유한 BehaviorTree 컴포넌트
	 * @param NodeMemory AI 인스턴스별 상태 메모리 (FRotateToFaceTargetTaskMemory)
	 * @param DeltaSeconds 이전 틱으로부터 경과한 시간
	 */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** AI 인스턴스마다 할당할 NodeMemory 크기 반환 (FRotateToFaceTargetTaskMemory) */
	virtual uint16 GetInstanceMemorySize() const override;

	/** BT 에디터 노드 박스에 표시할 설명 텍스트 반환 */
	virtual FString GetStaticDescription() const override;

	/**
	 * @brief QueryPawn의 전방 벡터와 타겟 방향 벡터 사이의 각도가 AnglePrecision 이하인지 확인한다
	 *
	 * @param QueryPawn 회전 여부를 확인할 AI 폰
	 * @param TargetActor 바라봐야 할 타겟 액터
	 * @return 각도 오차가 AnglePrecision 이하면 true
	 */
	bool HasReachedAnglePrecision(const APawn* QueryPawn, const AActor* TargetActor) const;

	// 회전이 완료됐다고 판단하는 각도 오차 허용 범위 (도 단위)
	UPROPERTY(EditAnywhere, Category = "Face Target")
	float AnglePrecision;

	// 타겟 방향으로 회전할 때 사용하는 보간 속도
	UPROPERTY(EditAnywhere, Category = "Face Target")
	float RotationInterpSpeed;

	// 바라볼 대상 액터를 가리키는 블랙보드 키 (AActor 타입만 허용)
	UPROPERTY(EditAnywhere, Category = "Face Target")
	FBlackboardKeySelector InTargetToFaceKey;
};