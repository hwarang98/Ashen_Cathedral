// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ACFunctionLibrary.generated.h"

class UACAbilitySystemComponent;
struct FGameplayTag;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Actor에서 AbilitySystemComponent를 반환
	 *
	 * @param InActor 검색 대상 Actor
	 * @return Actor에 연결된 UACAbilitySystemComponent가 존재하면 해당 인스턴스를 반환하고, 그렇지 않으면 nullptr을 반환
	 */
	static UACAbilitySystemComponent* NativeAbilitySystemComponentFromActor(AActor* InActor);

	/**
	 * Actor가 주어진 GameplayTag를 가지고 있지 않은 경우, 해당 태그를 추가
	 *
	 * @param InActor 태그를 추가할 Actor
	 * @param TagToAdd 추가하려는 GameplayTag
	 */
	UFUNCTION(BlueprintCallable, Category = "Crimson Moon|FunctionLibrary")
	static void AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd);

	/**
	 * 주어진 Actor가 특정 GameplayTag를 가지고 있는지 확인
	 *
	 * @param InActor 검사 대상 Actor
	 * @param TagToCheck Actor의 태그 목록에서 확인할 GameplayTag
	 * @return Actor가 지정된 태그를 가지고 있다면 true를 반환하고, 그렇지 않으면 false를 반환
	 */
	static bool NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck);
};