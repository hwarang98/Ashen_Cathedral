// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enums/ACEnums.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ACFunctionLibrary.generated.h"

class UPawnCombatComponent;
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
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|FunctionLibrary")
	static void AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd);

	/**
	 * Actor에서 특정 GameplayTag를 제거
	 *
	 * @param InActor 태그를 제거할 Actor
	 * @param TagToRemove 제거하려는 GameplayTag
	 *
	 * @details Actor에 연결된 AbilitySystemComponent를 이용하여 주어진 태그가 존재하면 해당 태그를 제거한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|FunctionLibrary")
	static void RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove);

	/**
	 * 주어진 Actor가 특정 GameplayTag를 가지고 있는지 확인
	 *
	 * @param InActor 검사 대상 Actor
	 * @param TagToCheck Actor의 태그 목록에서 확인할 GameplayTag
	 * @return Actor가 지정된 태그를 가지고 있다면 true를 반환하고, 그렇지 않으면 false를 반환
	 */
	static bool NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck);

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|FunctionLibrary", meta = (DisplayName = "Does Actor Have Tag", ExpandEnumAsExecs = "OutConfirmType"))
	static void BP_DoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck, EACConfirmType& OutConfirmType);

	/**
	 * QueryPawn과 TargetPawn 간의 적대 관계를 확인
	 *
	 * @param QueryPawn 적대 관계를 확인하는 주체 Pawn
	 * @param TargetPawn 적대 관계를 확인할 대상 Pawn
	 * @return QueryPawn과 TargetPawn이 적대 관계라면 true를 반환, 그렇지 않으면 false를 반환
	 *
	 * @details 이 함수는 두 Pawn의 Controller가 IGenericTeamAgentInterface를 구현하고 있는지 확인하며, 팀 ID가 다를 경우 적대 관계로 간주한다.
	 * 팀 인식이 없는 경우에는 적대 관계가 아니라고 판단한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathdral|FunctionLibrary")
	static bool IsTargetPawnHostile(const APawn* QueryPawn, const APawn* TargetPawn);

	/* 액터에서 PawnCombatComponent를 가져옴 */
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|FunctionLibrary", meta = (DisplayName = "Get Pawn Combat Component From Actor", ExpandEnumAsExecs = "OutValidType"))
	static UPawnCombatComponent* BP_GetPawnCombatComponentFromActor(AActor* InActor, EACValidType& OutValidType);

	/* 내부적으로 PawnCombatComponent를 직접 검색 */
	static UPawnCombatComponent* NativeGetPawnCombatComponentFromActor(AActor* InActor);

	/* 캐릭터가 맞은 위치 별 태그 반환 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathdral|FunctionLibrary")
	static FGameplayTag ComputeHitReactDirectionTag(const AActor* InAttacker, const AActor* InVictim, float& OutAngleDifference);

	/**
	 * 방어자가 공격자를 향해 블록 가능한 각도 범위 내에 있는지 확인합니다.
	 * @param InAttacker 공격자
	 * @param InDefender 방어자
	 * @param AngleThreshold 블록 허용 각도 (도 단위, 기본값 60도)
	 * @return 유효한 블록이면 true, 아니면 false
	 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathdral|FunctionLibrary")
	static bool IsValidBlock(const AActor* InAttacker, const AActor* InDefender, const float AngleThreshold = 60.0f);

private:
	/* 주어진 각도 차이를 바탕으로 히트 반응 태그를 결정 */
	static FGameplayTag DetermineHitReactionTag(const float& OutAngleDifference);
};