// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ACGameMode.generated.h"

class AACCharacterBase;

/**
 * 보스 전투 오케스트레이션을 담당.
 * 보스 액터가 BeginPlay 시 RegisterBossCharacter()를 호출하면
 * GameState에 보스를 등록하고 사망 델리게이트를 구독한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AACGameMode();

	/**
	 * @brief 보스 액터를 GameState에 등록하고 사망 이벤트를 구독한다.
	 * Level에 배치된 보스가 BeginPlay에서 호출하며, 이미 보스가 등록된 경우 중복 등록을 막는다.
	 * @param InBossCharacter 등록할 보스 캐릭터
	 */
	void RegisterBossCharacter(AACCharacterBase* InBossCharacter);

private:
	UFUNCTION()
	void HandleBossDeath(AACCharacterBase* DeadCharacter);
};
