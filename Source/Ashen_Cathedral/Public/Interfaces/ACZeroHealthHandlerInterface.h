// 체력이 0 이하가 된 순간, 기본 사망 처리 대신 상황을 가져갈 기회를 주는 인터페이스

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ACZeroHealthHandlerInterface.generated.h"

UINTERFACE(MinimalAPI)
class UACZeroHealthHandlerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 체력 0 처리를 가로챌 수 있는 대상이 구현하는 인터페이스.
 *
 * UACAttributeSet은 이 인터페이스만 알고 있으면 되며, 보스 페이즈 데이터나 구체 보스 클래스를 알 필요가 없다.
 * 액터 자신이 구현해도 되고, 액터에 붙은 컴포넌트(UACBossPhaseComponent 등)가 구현해도 된다.
 */
class ASHEN_CATHEDRAL_API IACZeroHealthHandlerInterface
{
	GENERATED_BODY()

public:
	/**
	 * @brief 체력이 0 이하가 된 순간 호출된다. 기본 사망 처리를 대신 가져갈지 결정한다.
	 *
	 * @param DamageInstigator 마지막 피해를 입힌 액터. 없을 수 있다
	 * @return true면 호출자는 Shared.Status.Dead 부여와 Shared.Event.Death 발송을 모두 생략한다
	 * @note true를 반환하는 구현체는 이후 들어올 추가 피해로 사망하지 않도록 스스로를 보호할 책임이 있다.
	 */
	virtual bool TryHandleZeroHealth(AActor* DamageInstigator) = 0;

	/**
	 * @brief 액터 자신과 액터의 모든 컴포넌트에서 핸들러를 찾아 순서대로 질의한다.
	 *
	 * @param TargetActor      체력이 0이 된 액터
	 * @param DamageInstigator 마지막 피해를 입힌 액터. 없을 수 있다
	 * @return 어느 핸들러든 처리했다면 true. 핸들러가 없거나 아무도 처리하지 않으면 false
	 * @note 핸들러가 아예 없는 일반 적은 항상 false가 되어 기존 사망 로직을 그대로 탄다.
	 */
	static bool TryHandleZeroHealthOnActor(AActor* TargetActor, AActor* DamageInstigator);
};
