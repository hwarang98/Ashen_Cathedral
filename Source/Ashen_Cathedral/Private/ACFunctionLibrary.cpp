// Fill out your copyright notice in the Description page of Project Settings.


#include "ACFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "Enums/ACEnums.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Interfaces/PawnCombatInterface.h"
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

	const float DotResult = FVector::DotProduct(InAttacker->GetActorForwardVector(), InDefender->GetActorForwardVector());

	return DotResult < -0.1;
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