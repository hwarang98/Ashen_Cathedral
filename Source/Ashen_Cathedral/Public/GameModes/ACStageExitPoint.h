// 보스 아레나에 배치해 클리어 후 상호작용하면 다음 스테이지로 진행시키는 액터

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Enums/ACEnums.h"
#include "Interfaces/InteractableInterface.h"
#include "ACStageExitPoint.generated.h"

class USphereComponent;

/**
 * 보스 아레나 맵에 배치하는 스테이지 출구 상호작용 지점.
 * 시작 시에는 숨겨져 있다가 GameMode의 보스 전투 완료 델리게이트를 받아 활성화된다.
 *
 * Destination에 따라 상호작용 동작이 갈린다.
 * NextStage는 RequestProgressAfterBossClear()를 호출해 Run을 이어가고(카드·적립분 유지),
 * ReturnToLobby는 RequestReturnToLobby()를 호출해 적립분을 정산하고 Run을 끝낸다.
 * 두 종류를 아레나에 함께 배치하면 보스 클리어 후 "더 갈지 챙겨 나갈지"의 선택이 만들어진다.
 *
 * 다음 보스가 같은 맵에 스폰되는 구성에서는 이 액터가 스테이지마다 재사용되므로,
 * 상호작용 직후 아레나의 출구를 모두 비활성화해 다음 전투 중에 다시 상호작용되는 것을 막는다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACStageExitPoint : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AACStageExitPoint();

	virtual void BeginPlay() override;

	/**
	 * @brief 상호작용 시 다음 스테이지 진행을 요청한다.
	 * 활성화 상태가 아니면 무시하며, GameMode를 호출하기 전에 아레나의 모든 출구를 비활성화해
	 * 다음 보스 전투 중에 다시 상호작용되지 않도록 한다.
	 * @param InstigatorPawn 상호작용을 시작한 Pawn
	 */
	virtual void Interact(APawn* InstigatorPawn) override;

	virtual FText GetInteractionText() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UStaticMeshComponent> ExitMesh;

	// 이 출구가 플레이어를 보낼 목적지. 두 종류를 한 아레나에 배치해 클리어 후 선택지를 만든다
	UPROPERTY(EditAnywhere, Category = "Interaction")
	EACStageExitDestination Destination = EACStageExitDestination::NextStage;

	// 상호작용 프롬프트에 표시할 문구 (일반 보스 / 최종 보스)
	UPROPERTY(EditAnywhere, Category = "Interaction")
	FText InteractionText = FText::FromString(TEXT("제단에 손을 얹는다"));

	UPROPERTY(EditAnywhere, Category = "Interaction")
	FText FinalStageInteractionText = FText::FromString(TEXT("성당을 떠난다"));

	// Destination이 ReturnToLobby일 때 표시할 문구 — 적립분을 확정하고 나간다는 뜻이 드러나야 한다
	UPROPERTY(EditAnywhere, Category = "Interaction")
	FText ReturnToLobbyInteractionText = FText::FromString(TEXT("성흔을 챙겨 돌아간다"));

	// 출구가 활성화되는 순간의 연출(나이아가라/사운드)을 BP에서 붙이기 위한 훅
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void BP_OnActivated();

private:
	UFUNCTION()
	void HandleBossBattleCompleted(bool bInIsFinalBoss);

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

	// 출구를 닫고, 상호작용을 시작한 플레이어의 프롬프트 참조까지 정리한다
	void Deactivate(APawn* InstigatorPawn);

	// 출구의 표시 여부와 콜리전을 함께 토글한다
	void SetActivated(bool bInActivated);

	// 상호작용 가능한 상태인지 여부 (보스 클리어 후 ~ 상호작용 직전까지)
	bool bActivated = false;

	// 이번에 클리어한 보스가 최종 보스였는지 여부 — 프롬프트 문구를 결정한다
	bool bIsFinalBoss = false;
};
