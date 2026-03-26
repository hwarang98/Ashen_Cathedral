// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/Enemy/ACEnemyAnimInstance.h"
#include "ACGameplayTags.h"
#include "GameplayTags/ACGameplayTags_Shared.h"

void UACEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningCharacter)
	{
		return;
	}

	bIsGroggy = DoesOwnerHaveTag(ACGameplayTags::Shared_Status_Groggy);
	bIsDead = DoesOwnerHaveTag(ACGameplayTags::Shared_Status_Dead);
}
