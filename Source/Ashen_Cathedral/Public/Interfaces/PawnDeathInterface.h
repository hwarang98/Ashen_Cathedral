// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PawnDeathInterface.generated.h"

UINTERFACE(MinimalAPI)
class UPawnDeathInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 사망 시 캐릭터 내부에서 처리해야 할 로직을 정의하는 인터페이스.
 *
 * - 이 인터페이스를 구현하는 쪽(캐릭터)이 자신의 사망 후처리 책임을 가짐
 * - 어빌리티는 구체적인 캐릭터 타입을 알 필요 없이 인터페이스만 호출
 *
 * 사용 예:
 *   - AACPlayerCharacter: 게임오버 UI 표시, 입력 차단 등
 *   - AACEnemyCharacterBase: 드롭 아이템, 퀘스트 완료 처리 등
 */
class ASHEN_CATHEDRAL_API IPawnDeathInterface
{
	GENERATED_BODY()

public:
	/**
	 * 사망 어빌리티가 활성화될 때 호출됨.
	 * 각 캐릭터 클래스에서 자신에게 맞는 사망 후처리를 구현할 것.
	 */
	virtual void OnDeath() = 0;
};
