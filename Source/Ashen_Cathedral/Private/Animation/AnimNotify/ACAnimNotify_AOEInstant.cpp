// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AnimNotify/ACAnimNotify_AOEInstant.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ACGameplayTags.h"
#include "Components/SkeletalMeshComponent.h"

void UACAnimNotify_AOEInstant::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, ACGameplayTags::Shared_Event_AOE_Instant, FGameplayEventData());
	}
}

FString UACAnimNotify_AOEInstant::GetNotifyName_Implementation() const
{
	return TEXT("AOE Instant");
}
