// 로비에 배치해 플레이어가 상호작용하면 보스 아레나로 진입시키는 트리거 액터

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/InteractableInterface.h"
#include "ACBattleStartPoint.generated.h"

class USphereComponent;

/**
 * 로비 맵에 배치하는 전투 시작 상호작용 지점.
 * 플레이어가 SphereComponent 범위에 들어오면 자신을 CurrentInteractable로 등록시키고,
 * 상호작용 입력이 들어오면(Interact) GameMode의 RequestStartRun()을 호출해 보스 아레나로 이동한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACBattleStartPoint : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AACBattleStartPoint();

	virtual void Interact(APawn* InstigatorPawn) override;

	virtual FText GetInteractionText() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USphereComponent> InteractionSphere;

	// 상호작용 프롬프트에 표시할 문구
	UPROPERTY(EditAnywhere, Category = "Interaction")
	FText InteractionText = FText::FromString(TEXT("보스 아레나로 향한다"));

private:
	UFUNCTION()
	void OnInteractionSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
