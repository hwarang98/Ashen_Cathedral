// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTaskNode_RotateToFaceTarget.h"
#include "Structs/ACStructTypes.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/KismetMathLibrary.h"

UBTTaskNode_RotateToFaceTarget::UBTTaskNode_RotateToFaceTarget()
{
	NodeName = TEXT("Native Rotate to Face Target Actor");
	AnglePrecision = 10.f;
	RotationInterpSpeed = 5.f;

	// TickTask(), OnTaskFinished() 콜백을 활성화
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	// 노드 인스턴스를 AI마다 따로 생성하지 않고, NodeMemory로 상태를 분리 -> 여러 AI가 같은 태스크를 동시에 실행해도 서로 간섭 없음
	bCreateNodeInstance = false;

	// bNotifyTick 등의 플래그를 실제 내부 비트 필드에 반영
	INIT_TASK_NODE_NOTIFY_FLAGS();

	// 에디터 드롭다운에서 AActor 타입 키만 표시되도록 필터링
	InTargetToFaceKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, InTargetToFaceKey), AActor::StaticClass());
}

void UBTTaskNode_RotateToFaceTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	// 블랙보드 에셋이 로드될 때 한 번 호출됨
	// 키 이름(문자열)을 키 ID(정수)로 바인딩해야 TickTask에서 GetValueAsObject가 정상 동작함
	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		InTargetToFaceKey.ResolveSelectedKey(*BBAsset);
	}
}

uint16 UBTTaskNode_RotateToFaceTarget::GetInstanceMemorySize() const
{
	// UE가 AI 인스턴스마다 NodeMemory를 할당할 때 이 크기를 사용
	// FRotateToFaceTargetTaskMemory: OwningPawn + TargetActor (TWeakObjectPtr)
	return sizeof(FRotateToFaceTargetTaskMemory);
}

EBTNodeResult::Type UBTTaskNode_RotateToFaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 블랙보드에서 타겟 액터를 가져옴
	UObject* ActorObject = OwnerComp.GetBlackboardComponent()->GetValueAsObject(InTargetToFaceKey.SelectedKeyName);
	AActor* TargetActor = Cast<AActor>(ActorObject);

	APawn* OwningPawn = OwnerComp.GetAIOwner()->GetPawn();

	// NodeMemory를 이 AI 전용 상태 구조체로 캐스팅, null이면 즉시 크래시로 문제를 조기에 발견
	FRotateToFaceTargetTaskMemory* Memory = CastInstanceNodeMemory<FRotateToFaceTargetTaskMemory>(NodeMemory);
	check(Memory);

	// 이후 TickTask에서 사용할 수 있도록 NodeMemory에 저장
	Memory->OwningPawn = OwningPawn;
	Memory->TargetActor = TargetActor;

	// 폰 또는 타겟이 유효하지 않으면 태스크 실패
	if (!Memory->IsValid())
	{
		return EBTNodeResult::Failed;
	}

	// 태스크 시작 시점에 이미 목표 각도에 도달해 있으면 TickTask 없이 즉시 성공
	if (HasReachedAnglePrecision(OwningPawn, TargetActor))
	{
		Memory->Reset();
		return EBTNodeResult::Succeeded;
	}

	// 아직 회전이 필요한 경우 InProgress를 반환해 TickTask가 매 틱 호출되도록 유지
	return EBTNodeResult::InProgress;
}

void UBTTaskNode_RotateToFaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// NodeMemory를 FRotateToFaceTargetTaskMemory 포인터로 캐스팅해 이 AI의 상태를 가져옴
	FRotateToFaceTargetTaskMemory* Memory = CastInstanceNodeMemory<FRotateToFaceTargetTaskMemory>(NodeMemory);

	// 폰이나 타겟이 사망/제거된 경우 즉시 실패 처리
	if (!Memory->IsValid())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}

	// 이미 목표 각도에 도달했으면 메모리를 초기화하고 태스크 성공 종료
	if (HasReachedAnglePrecision(Memory->OwningPawn.Get(), Memory->TargetActor.Get()))
	{
		Memory->Reset();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
	else
	{
		// 타겟을 향한 목표 회전값 계산 후 Pitch를 0으로 고정해 좌우 회전만 적용
		FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(Memory->OwningPawn->GetActorLocation(), Memory->TargetActor->GetActorLocation());
		LookAtRot.Pitch = 0.f;

		// RInterpTo: 프레임레이트 독립적 보간, RotationInterpSpeed가 클수록 빠르게 회전
		const FRotator TargetRot = FMath::RInterpTo(Memory->OwningPawn->GetActorRotation(), LookAtRot, DeltaSeconds, RotationInterpSpeed);

		Memory->OwningPawn->SetActorRotation(TargetRot);
	}
}

FString UBTTaskNode_RotateToFaceTarget::GetStaticDescription() const
{
	// BT 에디터 그래프의 노드 박스에 표시되는 설명 텍스트
	const FString KeyDescription = InTargetToFaceKey.SelectedKeyName.ToString();

	return FString::Printf(TEXT("%s 키의 액터 방향으로 부드럽게 회전 (각도 허용 범위: %s도)"), *KeyDescription, *FString::SanitizeFloat(AnglePrecision));
}

bool UBTTaskNode_RotateToFaceTarget::HasReachedAnglePrecision(const APawn* QueryPawn, const AActor* TargetActor) const
{
	// AI의 현재 전방 벡터 (Z 제거 후 정규화해 수평 방향만 비교)
	const FVector OwnerForward = FVector(QueryPawn->GetActorForwardVector().X, QueryPawn->GetActorForwardVector().Y, 0.f).GetSafeNormal();

	// AI -> 타겟 방향의 단위 벡터 (Z 제거 후 정규화해 수평 각도 오차만 계산)
	const FVector OwnerToTargetNormalized = FVector(TargetActor->GetActorLocation().X - QueryPawn->GetActorLocation().X, TargetActor->GetActorLocation().Y - QueryPawn->GetActorLocation().Y, 0.f).GetSafeNormal();

	// 내적(Dot Product): 두 단위 벡터가 완전히 같은 방향이면 1.0, 직각이면 0.0, 반대 방향이면 -1.0
	const float DotResult = FVector::DotProduct(OwnerForward, OwnerToTargetNormalized);

	// DegAcos: 내적 결과를 각도(도 단위)로 변환 -> 현재 전방과 타겟 방향 사이의 각도 오차
	const float AngleDiff = UKismetMathLibrary::DegAcos(DotResult);

	return AngleDiff <= AnglePrecision;
}