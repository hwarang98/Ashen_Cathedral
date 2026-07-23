// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/Combat/AOEDamageComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ACFunctionLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"

UAOEDamageComponent::UAOEDamageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FVector UAOEDamageComponent::ComputeAOEOrigin(float ForwardOffset) const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() + Owner->GetActorForwardVector() * ForwardOffset : FVector::ZeroVector;
}

void UAOEDamageComponent::TriggerInstantAOE(float Radius, float ForwardOffset, bool bDebugDraw, TFunction<void(AActor*)> OnTargetFound)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	const FVector Origin = ComputeAOEOrigin(ForwardOffset);

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	World->OverlapMultiByChannel(
		OverlapResults,
		Origin,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		QueryParams
		);

#if ENABLE_DRAW_DEBUG
	if (bDebugDraw)
	{
		DrawDebugSphere(World, Origin, Radius, 24, FColor::Orange, false, 1.5f);
	}
#endif

	TArray<AActor*> Candidates;
	Candidates.Reserve(OverlapResults.Num());
	for (const FOverlapResult& Result : OverlapResults)
	{
		if (AActor* HitActor = Result.GetActor())
		{
			Candidates.Add(HitActor);
		}
	}

	// 단발형은 이번 호출 한정으로만 중복을 제거하면 된다.
	TMap<TWeakObjectPtr<AActor>, double> LocalHitTimes;
	BroadcastHostileTargets(Candidates, &LocalHitTimes, 0.f, OnTargetFound);
}

void UAOEDamageComponent::StartSustainedAOE(float Radius, float ForwardOffset, float Interval, bool bDebugDraw, TFunction<void(AActor*)> OnTargetFound, float ReHitInterval)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	// 이전 지속형 AOE의 잔여 상태(타이머/히트 목록)를 정리한 뒤 새로 시작한다.
	StopSustainedAOE();

	SustainedRadius = Radius;
	SustainedForwardOffset = ForwardOffset;
	SustainedReHitInterval = ReHitInterval;
	bSustainedDebugDraw = bDebugDraw;
	SustainedOnTargetFound = MoveTemp(OnTargetFound);
	PreviousAOEOrigin = ComputeAOEOrigin(ForwardOffset);

	const float SafeInterval = FMath::Max(Interval, 0.01f);
	World->GetTimerManager().SetTimer(SustainedTickTimerHandle, this, &ThisClass::TickSustainedAOE, SafeInterval, true);
}

void UAOEDamageComponent::StopSustainedAOE()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SustainedTickTimerHandle);
	}
	SustainedHitTimes.Reset();
	SustainedOnTargetFound = nullptr;
}

void UAOEDamageComponent::TickSustainedAOE()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || !SustainedOnTargetFound)
	{
		StopSustainedAOE();
		return;
	}

	const FVector CurrentOrigin = ComputeAOEOrigin(SustainedForwardOffset);

	// 빠른 이동 중 판정 누락을 막기 위해 이전 위치에서 현재 위치까지 Sphere를 스윕한다.
	TArray<FHitResult> SweepHits;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	World->SweepMultiByChannel(
		SweepHits,
		PreviousAOEOrigin,
		CurrentOrigin,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(SustainedRadius),
		QueryParams
		);

#if ENABLE_DRAW_DEBUG
	if (bSustainedDebugDraw)
	{
		DrawDebugLine(World, PreviousAOEOrigin, CurrentOrigin, FColor::Cyan, false, 1.0f, 0, 2.f);
		DrawDebugSphere(World, CurrentOrigin, SustainedRadius, 16, FColor::Cyan, false, 1.0f);
	}
#endif

	TArray<AActor*> Candidates;
	Candidates.Reserve(SweepHits.Num());
	for (const FHitResult& Hit : SweepHits)
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			Candidates.Add(HitActor);
		}
	}

	BroadcastHostileTargets(Candidates, &SustainedHitTimes, SustainedReHitInterval, SustainedOnTargetFound);

	PreviousAOEOrigin = CurrentOrigin;
}

void UAOEDamageComponent::BroadcastHostileTargets(const TArray<AActor*>& CandidateActors, TMap<TWeakObjectPtr<AActor>, double>* HitTimes, float ReHitInterval, const TFunction<void(AActor*)>& OnTargetFound) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const UWorld* World = GetWorld();
	if (!OwnerPawn || !World || !OnTargetFound)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();

	for (AActor* Candidate : CandidateActors)
	{
		APawn* OtherPawn = Cast<APawn>(Candidate);
		if (!OtherPawn || OtherPawn == OwnerPawn)
		{
			continue;
		}

		if (HitTimes)
		{
			if (const double* LastHitTime = HitTimes->Find(OtherPawn))
			{
				// 재히트 간격이 없으면(단발/1회형) 이미 맞은 대상은 스킵, 있으면 간격이 지나야 다시 히트한다.
				if (ReHitInterval <= 0.f || (Now - *LastHitTime) < ReHitInterval)
				{
					continue;
				}
			}
		}

		if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherPawn))
		{
			continue;
		}

		if (!UACFunctionLibrary::IsTargetPawnHostile(OwnerPawn, OtherPawn))
		{
			continue;
		}

		if (HitTimes)
		{
			HitTimes->Add(OtherPawn, Now);
		}

		OnTargetFound(OtherPawn);
	}
}
