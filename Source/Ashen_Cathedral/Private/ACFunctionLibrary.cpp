// Fill out your copyright notice in the Description page of Project Settings.


#include "ACFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "Enums/ACEnums.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Interfaces/PawnCombatInterface.h"

UACAbilitySystemComponent* UACFunctionLibrary::NativeAbilitySystemComponentFromActor(AActor* InActor)
{
	check(InActor);

	return CastChecked<UACAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InActor));
}

void UACFunctionLibrary::AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd)
{
	UACAbilitySystemComponent* AbilitySystemComponent = NativeAbilitySystemComponentFromActor(InActor);
	if (!AbilitySystemComponent->HasMatchingGameplayTag(TagToAdd))
	{
		AbilitySystemComponent->AddLooseGameplayTag(TagToAdd);
	}
}

void UACFunctionLibrary::RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove)
{
	UACAbilitySystemComponent* AbilitySystemComponent = NativeAbilitySystemComponentFromActor(InActor);

	if (AbilitySystemComponent->HasMatchingGameplayTag(TagToRemove))
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(TagToRemove);
	}
}

bool UACFunctionLibrary::NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck)
{
	UACAbilitySystemComponent* AbilitySystemComponent = NativeAbilitySystemComponentFromActor(InActor);

	return AbilitySystemComponent->HasMatchingGameplayTag(TagToCheck);
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
	check(InActor);

	if (const IPawnCombatInterface* PawnCombatInterface = Cast<IPawnCombatInterface>(InActor))
	{
		return PawnCombatInterface->GetPawnCombatComponent();
	}

	return nullptr;
}