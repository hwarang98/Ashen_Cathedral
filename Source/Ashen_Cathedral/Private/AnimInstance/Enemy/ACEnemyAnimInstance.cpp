// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/Enemy/ACEnemyAnimInstance.h"
#include "ACGameplayTags.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "GameplayTags/ACGameplayTags_Shared.h"

void UACEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BossIdentityTags.Reset();

	if (const AACEnemyCharacter* EnemyCharacter = Cast<AACEnemyCharacter>(OwningCharacter))
	{
		const FGameplayTag BossIdentityTag = EnemyCharacter->GetBossIdentityTag();
		if (BossIdentityTag.IsValid())
		{
			BossIdentityTags.AddTag(BossIdentityTag);
		}
	}
}

void UACEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningCharacter)
	{
		return;
	}

	bIsPostureBroken = DoesOwnerHaveTag(ACGameplayTags::Shared_Status_PostureBroken);
	bIsDead = DoesOwnerHaveTag(ACGameplayTags::Shared_Status_Dead);
}
