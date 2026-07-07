// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/ACBTDecorator_RandomCooldown.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Engine/World.h"

UACBTDecorator_RandomCooldown::UACBTDecorator_RandomCooldown()
{
	NodeName = TEXT("랜덤 쿨다운");
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();

	// 쿨다운이 끝난 뒤 하위 브랜치를 abort하는 건 의미가 없음 — 쿨다운은 브랜치를 벗어난 뒤 시작됨
	bAllowAbortChildNodes = false;
}

bool UACBTDecorator_RandomCooldown::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	FACRandomCooldownDecoratorMemory* DecoratorMemory = CastInstanceNodeMemory<FACRandomCooldownDecoratorMemory>(NodeMemory);
	const double RecalcTime = OwnerComp.GetWorld()->GetTimeSeconds() - DecoratorMemory->CurrentCooldownDuration;
	return RecalcTime >= DecoratorMemory->LastUseTimestamp;
}

void UACBTDecorator_RandomCooldown::OnNodeDeactivation(FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult)
{
	FACRandomCooldownDecoratorMemory* DecoratorMemory = GetNodeMemory<FACRandomCooldownDecoratorMemory>(SearchData);
	DecoratorMemory->LastUseTimestamp = SearchData.OwnerComp.GetWorld()->GetTimeSeconds();
	DecoratorMemory->CurrentCooldownDuration = FMath::FRandRange(MinCooldownTime, MaxCooldownTime);
	DecoratorMemory->bRequestedRestart = false;
}

void UACBTDecorator_RandomCooldown::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FACRandomCooldownDecoratorMemory* DecoratorMemory = CastInstanceNodeMemory<FACRandomCooldownDecoratorMemory>(NodeMemory);
	if (!DecoratorMemory->bRequestedRestart)
	{
		const double RecalcTime = OwnerComp.GetWorld()->GetTimeSeconds() - DecoratorMemory->CurrentCooldownDuration;
		if (RecalcTime >= DecoratorMemory->LastUseTimestamp)
		{
			DecoratorMemory->bRequestedRestart = true;
			OwnerComp.RequestExecution(this);
		}
	}
}

FString UACBTDecorator_RandomCooldown::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("%s: %.1f~%.1f초 랜덤 쿨다운 후 %s 반환"),
		*Super::GetStaticDescription(),
		MinCooldownTime,
		MaxCooldownTime,
		*UBehaviorTreeTypes::DescribeNodeResult(EBTNodeResult::Failed)
		);
}

void UACBTDecorator_RandomCooldown::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	FACRandomCooldownDecoratorMemory* DecoratorMemory = CastInstanceNodeMemory<FACRandomCooldownDecoratorMemory>(NodeMemory);
	const double TimePassed = OwnerComp.GetWorld()->GetTimeSeconds() - DecoratorMemory->LastUseTimestamp;

	if (TimePassed < DecoratorMemory->CurrentCooldownDuration)
	{
		Values.Add(FString::Printf(
			TEXT("%s in %ss"),
			(FlowAbortMode == EBTFlowAbortMode::None) ? TEXT("unlock") : TEXT("restart"),
			*FString::SanitizeFloat(DecoratorMemory->CurrentCooldownDuration - TimePassed)
			));
	}
}

uint16 UACBTDecorator_RandomCooldown::GetInstanceMemorySize() const
{
	return sizeof(FACRandomCooldownDecoratorMemory);
}

void UACBTDecorator_RandomCooldown::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	FACRandomCooldownDecoratorMemory* DecoratorMemory = InitializeNodeMemory<FACRandomCooldownDecoratorMemory>(NodeMemory, InitType);
	if (InitType == EBTMemoryInit::Initialize)
	{
		DecoratorMemory->LastUseTimestamp = TNumericLimits<double>::Lowest();
		DecoratorMemory->CurrentCooldownDuration = 0.f;
	}

	DecoratorMemory->bRequestedRestart = false;
}

void UACBTDecorator_RandomCooldown::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<FACRandomCooldownDecoratorMemory>(NodeMemory, CleanupType);
}
