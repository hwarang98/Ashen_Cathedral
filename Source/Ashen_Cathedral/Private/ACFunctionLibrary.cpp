// Fill out your copyright notice in the Description page of Project Settings.


#include "ACFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "Enums/ACEnums.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/Abilities/Common/ACAbility_Attack.h"
#include "Interfaces/PawnCombatInterface.h"
#include "Items/Weapon/ACWeaponBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "ACGameplayTags.h"

UACAbilitySystemComponent* UACFunctionLibrary::NativeAbilitySystemComponentFromActor(AActor* InActor)
{
	if (!IsValid(InActor))
	{
		return nullptr;
	}

	return Cast<UACAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InActor));
}

void UACFunctionLibrary::AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd)
{
	UACAbilitySystemComponent* ASC = NativeAbilitySystemComponentFromActor(InActor);
	if (!ASC || ASC->HasMatchingGameplayTag(TagToAdd))
	{
		return;
	}

	ASC->AddLooseGameplayTag(TagToAdd);
}

void UACFunctionLibrary::RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove)
{
	UACAbilitySystemComponent* ASC = NativeAbilitySystemComponentFromActor(InActor);
	if (!ASC || !ASC->HasMatchingGameplayTag(TagToRemove))
	{
		return;
	}

	ASC->RemoveLooseGameplayTag(TagToRemove);
}

void UACFunctionLibrary::AddGameplayTagsToActor(AActor* InActor, const FGameplayTagContainer& TagsToAdd)
{
	UACAbilitySystemComponent* ASC = NativeAbilitySystemComponentFromActor(InActor);
	if (!ASC)
	{
		return;
	}

	ASC->AddLooseGameplayTags(TagsToAdd);
}

void UACFunctionLibrary::RemoveGameplayTagsFromActor(AActor* InActor, const FGameplayTagContainer& TagsToRemove)
{
	UACAbilitySystemComponent* ASC = NativeAbilitySystemComponentFromActor(InActor);
	if (!ASC)
	{
		return;
	}

	ASC->RemoveLooseGameplayTags(TagsToRemove);
}

bool UACFunctionLibrary::NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck)
{
	if (!IsValid(InActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("NativeDoesActorHaveTag: InActor가 null이거나 유효하지 않습니다"));
		return false;
	}

	UACAbilitySystemComponent* ASC = NativeAbilitySystemComponentFromActor(InActor);
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("NativeDoesActorHaveTag: [%s]의 ASC가 null입니다"), *InActor->GetName());
		return false;
	}

	return ASC->HasMatchingGameplayTag(TagToCheck);
}

void UACFunctionLibrary::BP_DoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck, EACConfirmType& OutConfirmType)
{
	OutConfirmType = NativeDoesActorHaveTag(InActor, TagToCheck) ? EACConfirmType::Yes : EACConfirmType::No;
}

bool UACFunctionLibrary::IsTargetPawnHostile(const APawn* QueryPawn, const APawn* TargetPawn)
{
	check(QueryPawn && TargetPawn);

	const IGenericTeamAgentInterface* GenericTeamAgent = Cast<IGenericTeamAgentInterface>(QueryPawn->GetController());
	const IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(TargetPawn->GetController());

	if (GenericTeamAgent && TargetTeamAgent)
	{
		// 팀 ID가 다르면 적대 관계로 간주하여 true 반환
		return GenericTeamAgent->GetGenericTeamId() != TargetTeamAgent->GetGenericTeamId();
	}

	// 팀 인터페이스를 구현하지 않은 경우 적대 관계로 간주하지 않음
	return false;
}

UPawnCombatComponent* UACFunctionLibrary::BP_GetPawnCombatComponentFromActor(AActor* InActor, EACValidType& OutValidType)
{
	UPawnCombatComponent* CombatComponent = NativeGetPawnCombatComponentFromActor(InActor);
	OutValidType = CombatComponent ? EACValidType::Valid : EACValidType::Invalid;
	return CombatComponent;
}

UPawnCombatComponent* UACFunctionLibrary::NativeGetPawnCombatComponentFromActor(AActor* InActor)
{
	// check(InActor);

	if (const IPawnCombatInterface* PawnCombatInterface = Cast<IPawnCombatInterface>(InActor))
	{
		return PawnCombatInterface->GetPawnCombatComponent();
	}

	return nullptr;
}

UNiagaraSystem* UACFunctionLibrary::ResolveWeaponTrailEffect(AActor* InOwner, FName SocketName, UNiagaraSystem* InDefaultNiagaraSystem)
{
	if (const UPawnCombatComponent* PawnCombatComponent = NativeGetPawnCombatComponentFromActor(InOwner))
	{
		if (const AACWeaponBase* WeaponBase = PawnCombatComponent->GetCharacterCurrentEquippedWeapon())
		{
			if (UNiagaraSystem* TrailOverride = WeaponBase->GetTrailEffectOverride(SocketName))
			{
				return TrailOverride;
			}
		}
	}

	return InDefaultNiagaraSystem;
}

FGameplayTag UACFunctionLibrary::ComputeHitReactDirectionTag(const AActor* InAttacker, const AActor* InVictim, float& OutAngleDifference)
{
	check(InAttacker && InVictim);

	const FVector VictimForward = InVictim->GetActorForwardVector();
	const FVector VictimToAttackerNormalized = (InAttacker->GetActorLocation() - InVictim->GetActorLocation()).GetSafeNormal();

	// 두 벡터의 내적 결과 (코사인 값)를 구함
	const float DotResult = FVector::DotProduct(VictimForward, VictimToAttackerNormalized);
	const FVector CrossResult = FVector::CrossProduct(VictimForward, VictimToAttackerNormalized);

	OutAngleDifference = UKismetMathLibrary::DegAcos(DotResult);

	// 외적 결과의 Z 값이 음수이면 오른쪽에서 공격 -> 각도 부호를 음수로 바꿈
	if (CrossResult.Z < 0.f)
	{
		OutAngleDifference *= -1.f;
	}
	return DetermineHitReactionTag(OutAngleDifference);
}

bool UACFunctionLibrary::IsValidBlock(const AActor* InAttacker, const AActor* InDefender, const float AngleThreshold)
{
	check(InAttacker && InDefender);

	// 위치 기반 정면 판정: 방어자 Forward와 방어자→공격자 방향의 각도가 AngleThreshold(도) 이내여야 한다.
	// (Forward끼리 비교하는 방식은 공격자가 방어자 뒤에 있어도 통과할 수 있어 부정확하다)
	const FVector DefenderForward = InDefender->GetActorForwardVector();
	const FVector DefenderToAttacker = (InAttacker->GetActorLocation() - InDefender->GetActorLocation()).GetSafeNormal();
	const float DotResult = FVector::DotProduct(DefenderForward, DefenderToAttacker);

	return DotResult >= FMath::Cos(FMath::DegreesToRadians(AngleThreshold));
}

bool UACFunctionLibrary::IsActorBlocking(const AActor* InActor)
{
	return NativeDoesActorHaveTag(const_cast<AActor*>(InActor), ACGameplayTags::Player_Status_Blocking)
		|| NativeDoesActorHaveTag(const_cast<AActor*>(InActor), ACGameplayTags::Enemy_Status_Blocking);
}

bool UACFunctionLibrary::IsAttackBlockable(const FGameplayTagContainer& AttackTags)
{
	return AttackTags.HasTag(ACGameplayTags::Shared_Attack_Blockable) && !AttackTags.HasTag(ACGameplayTags::Shared_Attack_Unblockable);
}

bool UACFunctionLibrary::IsAttackParryable(const FGameplayTagContainer& AttackTags)
{
	return AttackTags.HasTag(ACGameplayTags::Shared_Attack_Parryable) && !AttackTags.HasTag(ACGameplayTags::Shared_Attack_Unparryable);
}

bool UACFunctionLibrary::IsSuccessfulBlock(const AActor* Attacker, const AActor* Defender, const FGameplayTagContainer& AttackTags)
{
	if (!IsValid(Attacker) || !IsValid(Defender))
	{
		return false;
	}

	if (!IsAttackBlockable(AttackTags))
	{
		return false;
	}

	if (!IsActorBlocking(Defender))
	{
		return false;
	}

	return IsValidBlock(Attacker, Defender);
}

bool UACFunctionLibrary::IsSuccessfulParry(const AActor* Attacker, const AActor* Defender, const FGameplayTagContainer& AttackTags)
{
	if (!IsValid(Attacker) || !IsValid(Defender))
	{
		return false;
	}

	if (!IsAttackParryable(AttackTags))
	{
		return false;
	}

	if (!NativeDoesActorHaveTag(const_cast<AActor*>(Defender), ACGameplayTags::Shared_Status_Parry))
	{
		return false;
	}

	return IsValidBlock(Attacker, Defender);
}

bool UACFunctionLibrary::TryTriggerSuccessfulBlockEvent(const AActor* Attacker, AActor* HitActor, const FGameplayTagContainer& AttackDefenseTags)
{
	if (!Attacker || !HitActor)
	{
		return false;
	}

	// Parry/Block 성공: 데미지 계산·HitReact 억제·Hit Cue 억제와 동일한 공통 판정을 사용한다.
	const bool bParrySuccess = IsSuccessfulParry(Attacker, HitActor, AttackDefenseTags);
	const bool bBlockSuccess = IsSuccessfulBlock(Attacker, HitActor, AttackDefenseTags);

	if (!bParrySuccess && !bBlockSuccess)
	{
		return false;
	}

	FGameplayEventData EventData;
	EventData.Instigator = Attacker;
	EventData.Target = HitActor;
	// 막아낸 공격의 속성 태그를 함께 넘겨, 방어자가 공격 무게에 따라 가드 브레이크 등으로 분기할 수 있게 한다
	EventData.InstigatorTags = AttackDefenseTags;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, ACGameplayTags::Player_Event_SuccessfulBlock, EventData);
	return true;
}

void UACFunctionLibrary::RequestAttackMontageSoftCancel(AActor* InActor)
{
	UACAbilitySystemComponent* ASC = NativeAbilitySystemComponentFromActor(InActor);
	if (!ASC)
	{
		return;
	}

	if (UACAbility_Attack* AttackAbility = Cast<UACAbility_Attack>(ASC->GetAnimatingAbility()))
	{
		AttackAbility->RequestSoftMontageCancel();
	}
}

FGameplayTag UACFunctionLibrary::DetermineHitReactionTag(const float& OutAngleDifference)
{
	// -45 ~ 45도 = 정면
	if (OutAngleDifference >= -45.f && OutAngleDifference <= 45.f)
	{
		return ACGameplayTags::Shared_Status_HitReact_Front;
	}
	// -135 ~ -45도 = 왼쪽
	if (OutAngleDifference < -45.f && OutAngleDifference >= -135.f)
	{
		return ACGameplayTags::Shared_Status_HitReact_Left;
	}
	// - 135보다 작거나 135보다 크면 = 오른쪽
	if (OutAngleDifference < -135.f || OutAngleDifference > 135.f)
	{
		return ACGameplayTags::Shared_Status_HitReact_Back;
	}
	// 45 ~ 135도 = 뒤
	if (OutAngleDifference > 45.f && OutAngleDifference <= 135.f)
	{
		return ACGameplayTags::Shared_Status_HitReact_Right;
	}
	return ACGameplayTags::Shared_Status_HitReact_Front;
}
