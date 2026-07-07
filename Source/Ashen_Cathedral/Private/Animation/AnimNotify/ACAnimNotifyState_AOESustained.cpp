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
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, ACGameplayTags::Shared_Event_AOE_Sustained_Start, FGameplayEventData());
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
