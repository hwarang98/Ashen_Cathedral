// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AnimNotify/ACAnimNotifyState_AOESustained.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ACGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"

void UACAnimNotifyState_AOESustained::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		// 다단히트 재타격 간격을 EventMagnitude로 실어 보낸다. bMultiHit이 false면 0을 보내 대상당 1회 판정으로 동작한다.
		FGameplayEventData Payload;
		Payload.EventMagnitude = bMultiHit ? ReHitInterval : 0.f;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, ACGameplayTags::Shared_Event_AOE_Sustained_Start, Payload);
	}
}

void UACAnimNotifyState_AOESustained::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, ACGameplayTags::Shared_Event_AOE_Sustained_End, FGameplayEventData());
	}
}

FString UACAnimNotifyState_AOESustained::GetNotifyName_Implementation() const
{
	return TEXT("AOE Sustained");
}
