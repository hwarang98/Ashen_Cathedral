// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"
#include "ACPlayerGameplayAbility.generated.h"

class AACPlayerController;
class AACPlayerCharacter;
class UPlayerCombatComponent;

UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerGameplayAbility : public UACGameplayAbility
{
	GENERATED_BODY()

protected:
	/**
	 * 플레이어의 ActorInfo에서 AACPlayerCharacter를 반환한다.
	 * ActorInfo에 유효한 AvatarActor가 없거나 AACPlayerCharacter로 캐스팅할 수 없다면 nullptr을 반환한다.
	 * 
	 * @return 유효한 AACPlayerCharacter 객체 또는 nullptr을 반환한다.
	 */
	UFUNCTION(BlueprintPure, Category = "PlayerAbility|Helpers")
	AACPlayerCharacter* GetPlayerCharacterFromActorInfo() const;

	/**
	 * 플레이어의 ActorInfo에서 UPlayerCombatComponent를 반환한다.
	 * ActorInfo에 유효한 AvatarActor가 없거나 AACPlayerCharacter에서 UPlayerCombatComponent를 얻을 수 없으면 nullptr을 반환한다.
	 *
	 * @return 유효한 UPlayerCombatComponent 객체 또는 nullptr을 반환한다.
	 */
	UFUNCTION(BlueprintPure, Category = "PlayerAbility|Helpers")
	UPlayerCombatComponent* GetPlayerCombatComponentFromActorInfo() const;

	/**
	 * 플레이어의 ActorInfo에서 AACPlayerController를 반환한다.
	 * ActorInfo에 유효한 PlayerController가 없거나 AACPlayerController로 캐스팅할 수 없다면 nullptr을 반환한다.
	 *
	 * @return 유효한 AACPlayerController 객체 또는 nullptr을 반환한다.
	 */
	UFUNCTION(BlueprintPure, Category = "PlayerAbility|Helpers")
	AACPlayerController* GetPlayerControllerFromActorInfo();

private:
	mutable TWeakObjectPtr<AACPlayerCharacter> CachedPlayerCharacter;
	mutable TWeakObjectPtr<AACPlayerController> CachedPlayerController;
};