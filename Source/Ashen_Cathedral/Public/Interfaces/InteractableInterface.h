// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

UINTERFACE(MinimalAPI)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 플레이어의 상호작용 입력에 반응해야 하는 액터가 구현하는 인터페이스.
 *
 * - 이 인터페이스를 구현하는 쪽(액터)이 상호작용 시 수행할 동작의 책임을 가짐
 * - 플레이어는 구체적인 액터 타입을 알 필요 없이 인터페이스만 호출
 */
class ASHEN_CATHEDRAL_API IInteractableInterface
{
	GENERATED_BODY()

public:
	/**
	 * 상호작용 입력이 들어왔을 때 호출됨.
	 * @param InstigatorPawn 상호작용을 시작한 Pawn
	 */
	virtual void Interact(APawn* InstigatorPawn) = 0;

	// 상호작용 범위에 들어왔을 때 프롬프트 UI에 표시할 문구. 비어 있으면 프롬프트를 띄우지 않는다.
	virtual FText GetInteractionText() const { return FText::GetEmpty(); }
};
