// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Combat/ACPressureDetectionComponent.h"
#include "ACGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ACFunctionLibrary.h"

UACPressureDetectionComponent::UACPressureDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACPressureDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningPawn());
	if (!ASC)
	{
		return;
	}

	CachedASC = ASC;
	HitReactEventHandle = ASC->GenericGameplayEventCallbacks.FindOrAdd(ACGameplayTags::Shared_Event_HitReact).AddUObject(this, &ThisClass::OnHitReactEventReceived);
}

void UACPressureDetectionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(ACGameplayTags::Shared_Event_HitReact).Remove(HitReactEventHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UACPressureDetectionComponent::OnHitReactEventReceived(const FGameplayEventData* Payload)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	// 사망/이미 압박 신호 처리 대기 중/이미 회피 중이면 감지하지 않는다
	if (ASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead))
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_State_PressureReady))
	{
		return;
	}

	if (ASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_Status_Dodging))
	{
		return;
	}

	if (bIgnoreDuringStagger && ASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Groggy))
	{
		return;
	}

	RecordHit(Payload ? Payload->Instigator : nullptr);
}

void UACPressureDetectionComponent::RecordHit(const AActor* InstigatorActor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	HitTimestamps.Add(World->GetTimeSeconds());
	PruneOldTimestamps();

	if (HitTimestamps.Num() >= HitThreshold)
	{
		TriggerPressureDetected(InstigatorActor);
	}
}

void UACPressureDetectionComponent::PruneOldTimestamps()
{
	const float Now = GetWorld()->GetTimeSeconds();
	HitTimestamps.RemoveAll([this, Now](float Timestamp) { return Now - Timestamp > TimeWindow; });
}

void UACPressureDetectionComponent::TriggerPressureDetected(const AActor* InstigatorActor)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	HitTimestamps.Reset();
	UACFunctionLibrary::AddGameplayTagToActorIfNone(GetOwner(), ACGameplayTags::Enemy_State_PressureReady);

	FGameplayEventData EventPayload;
	EventPayload.EventTag = ACGameplayTags::Enemy_Event_PressureDetected;
	EventPayload.Instigator = InstigatorActor;
	EventPayload.Target = GetOwner();
	EventPayload.EventMagnitude = static_cast<float>(HitThreshold);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), ACGameplayTags::Enemy_Event_PressureDetected, EventPayload);
}

UAbilitySystemComponent* UACPressureDetectionComponent::GetOwnerASC() const
{
	return CachedASC.Get();
}
