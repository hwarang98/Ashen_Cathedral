// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Enums/ACEnums.h"
#include "ACGameState.generated.h"

class AACCharacterBase;

/**
 * 보스 전투 상태와 보스 참조를 보유한다.
 * 상태 변경/보스 등록은 AACGameMode가 오케스트레이션하며, GameState는 데이터 보관만 담당.
 * AACGameMode가 AGameMode를 상속하므로 GameState도 AGameStateBase가 아닌 AGameState를 상속해야 한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACGameState : public AGameState
{
	GENERATED_BODY()

public:
	FORCEINLINE EACBattleState GetBattleState() const { return BattleState; }
	FORCEINLINE AACCharacterBase* GetBossCharacter() const { return BossCharacter; }

	void SetBattleState(EACBattleState NewState);
	void SetBossCharacter(AACCharacterBase* InBossCharacter);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle")
	EACBattleState BattleState = EACBattleState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle")
	TObjectPtr<AACCharacterBase> BossCharacter;
};
