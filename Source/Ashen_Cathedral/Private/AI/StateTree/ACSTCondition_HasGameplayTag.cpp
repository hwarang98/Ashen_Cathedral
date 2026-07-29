// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/StateTree/ACSTCondition_HasGameplayTag.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Controllers/ACStateTreeController.h"
#include "StateTreeExecutionContext.h"

bool FACSTCondition_HasGameplayTag::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AActor* ActorToTest = InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr;

	if (bCheckTargetActor)
	{
		const AACStateTreeController* StateTreeController = Cast<AACStateTreeController>(InstanceData.AIController);
		ActorToTest = StateTreeController ? StateTreeController->GetTargetActor() : nullptr;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ActorToTest);

	// 검사 대상이나 태그가 유효하지 않으면 '태그 없음'으로 간주한다
	if (!ASC || !Tag.IsValid())
	{
		return bInvert;
	}

	bool bHasTag = false;
	if (bExactMatch)
	{
		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);
		bHasTag = OwnedTags.HasTagExact(Tag);
	}
	else
	{
		bHasTag = ASC->HasMatchingGameplayTag(Tag);
	}

	return bInvert ? !bHasTag : bHasTag;
}
