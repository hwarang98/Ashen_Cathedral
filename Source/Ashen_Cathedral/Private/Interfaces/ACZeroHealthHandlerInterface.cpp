// 체력이 0 이하가 된 순간, 기본 사망 처리 대신 상황을 가져갈 기회를 주는 인터페이스

#include "Interfaces/ACZeroHealthHandlerInterface.h"
#include "GameFramework/Actor.h"

bool IACZeroHealthHandlerInterface::TryHandleZeroHealthOnActor(AActor* TargetActor, AActor* DamageInstigator)
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	// 액터 자신이 구현한 경우를 먼저 본다
	if (IACZeroHealthHandlerInterface* ActorHandler = Cast<IACZeroHealthHandlerInterface>(TargetActor))
	{
		if (ActorHandler->TryHandleZeroHealth(DamageInstigator))
		{
			return true;
		}
	}

	for (UActorComponent* Component : TargetActor->GetComponents())
	{
		IACZeroHealthHandlerInterface* ComponentHandler = Cast<IACZeroHealthHandlerInterface>(Component);
		if (ComponentHandler && ComponentHandler->TryHandleZeroHealth(DamageInstigator))
		{
			return true;
		}
	}

	return false;
}
