// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AnimNotify/ACAnimNotifyState_AddGameplayTag.h"
#include "Components/SkeletalMeshComponent.h"
#include "Interfaces/ACAnimNotifyTagReceiverInterface.h"

void UACAnimNotifyState_AddGameplayTag::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp || GameplayTags.IsEmpty())
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner || !Owner->GetClass()->ImplementsInterface(UACAnimNotifyTagReceiverInterface::StaticClass()))
	{
		return;
	}

	IACAnimNotifyTagReceiverInterface::Execute_OnAnimNotifyAddGameplayTags(Owner, GameplayTags);
}

void UACAnimNotifyState_AddGameplayTag::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!bRemoveOnEnd || !MeshComp || GameplayTags.IsEmpty())
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner || !Owner->GetClass()->ImplementsInterface(UACAnimNotifyTagReceiverInterface::StaticClass()))
	{
		return;
	}

	IACAnimNotifyTagReceiverInterface::Execute_OnAnimNotifyRemoveGameplayTags(Owner, GameplayTags);
}

FString UACAnimNotifyState_AddGameplayTag::GetNotifyName_Implementation() const
{
	if (GameplayTags.IsEmpty())
	{
		return Super::GetNotifyName_Implementation();
	}

	if (GameplayTags.Num() == 1)
	{
		return FString::Printf(TEXT("Add Tag: %s"), *GameplayTags.First().ToString());
	}

	return FString::Printf(TEXT("Add Tags: %d"), GameplayTags.Num());
}