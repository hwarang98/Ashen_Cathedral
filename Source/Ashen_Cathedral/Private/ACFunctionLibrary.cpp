// Fill out your copyright notice in the Description page of Project Settings.


#include "ACFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

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

bool UACFunctionLibrary::NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck)
{
	UACAbilitySystemComponent* AbilitySystemComponent = NativeAbilitySystemComponentFromActor(InActor);

	return AbilitySystemComponent->HasMatchingGameplayTag(TagToCheck);
}