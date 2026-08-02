// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AnimNotify/ACAnimNotifyState_SpecialLinkWindow.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ACGameplayTags.h"

void UACAnimNotifyState_SpecialLinkWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		ASC->AddLooseGameplayTag(ACGameplayTags::Player_Status_SpecialLinkWindow);
	}
}

void UACAnimNotifyState_SpecialLinkWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	// 몽타주가 중단되어도 UAnimInstance가 활성 NotifyState에 NotifyEnd를 호출하므로
	// (TriggerAnimNotifies / EndNotifyStates) 별도의 방어적 태그 제거 경로는 두지 않는다.
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner))
	{
		ASC->RemoveLooseGameplayTag(ACGameplayTags::Player_Status_SpecialLinkWindow);
	}
}

FString UACAnimNotifyState_SpecialLinkWindow::GetNotifyName_Implementation() const
{
	return TEXT("Special Link Window");
}
