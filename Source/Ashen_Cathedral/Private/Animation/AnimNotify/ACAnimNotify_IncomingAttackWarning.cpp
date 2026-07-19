// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AnimNotify/ACAnimNotify_IncomingAttackWarning.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/Abilities/Common/ACAbility_Attack.h"
#include "Kismet/KismetSystemLibrary.h"

void UACAnimNotify_IncomingAttackWarning::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Attacker = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Attacker)
	{
		return;
	}

	const UACAbilitySystemComponent* ASC = UACFunctionLibrary::NativeAbilitySystemComponentFromActor(Attacker);
	UACAbility_Attack* AttackAbility = ASC ? Cast<UACAbility_Attack>(ASC->GetAnimatingAbility()) : nullptr;
	if (!AttackAbility)
	{
		return;
	}

	// 방어/패링 가능 여부의 원본 데이터는 이 Notify 자신이 가진다. 같은 Ability의 실행 중 인스턴스에
	// 저장해두면, 실제 Hit 판정 시점(ACAbility_Attack::CreateDamageEffectSpec)도 이 값을 그대로 쓴다.
	AttackAbility->SetCurrentAttackDefenseTags(AttackDefenseTags);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> OverlappedActors;
	UKismetSystemLibrary::SphereOverlapActors(Attacker, Attacker->GetActorLocation(), DetectionRadius, ObjectTypes, AACEnemyCharacter::StaticClass(), TArray<AActor*>{Attacker}, OverlappedActors);

	if (OverlappedActors.IsEmpty())
	{
		return;
	}

	FGameplayTagContainer InstigatorTags = AttackDefenseTags;
	if (AttackAbility->GetComboAttackTypeTag().IsValid())
	{
		InstigatorTags.AddTag(AttackAbility->GetComboAttackTypeTag());
	}

	for (AActor* EnemyActor : OverlappedActors)
	{
		if (!EnemyActor)
		{
			continue;
		}

		FGameplayEventData Payload;
		Payload.EventTag = ACGameplayTags::Shared_Event_Combat_IncomingAttack;
		Payload.Instigator = Attacker;
		Payload.Target = EnemyActor;
		Payload.EventMagnitude = ExpectedHitTime;
		Payload.InstigatorTags = InstigatorTags;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(EnemyActor, ACGameplayTags::Shared_Event_Combat_IncomingAttack, Payload);
	}
}

FString UACAnimNotify_IncomingAttackWarning::GetNotifyName_Implementation() const
{
	return TEXT("Incoming Attack Warning");
}
