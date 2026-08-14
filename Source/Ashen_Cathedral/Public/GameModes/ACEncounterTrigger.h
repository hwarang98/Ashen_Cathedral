// 아레나 입구에 배치해 플레이어가 밟으면 조우 컷신을 재생하고, 끝나면 보스 AI를 시작시키는 1회용 트리거

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACEncounterTrigger.generated.h"

class UBoxComponent;
class ALevelSequenceActor;

/**
 * @brief 보스 조우 연출의 시작점.
 *
 * 보스의 AIController(StateTreeAIComponent)에서 Start Logic Automatically를 꺼 두면 보스는
 * 이 트리거가 발동하기 전까지 완전히 대기한다. 플레이어가 트리거를 밟으면 지정된 컷신을 재생하고,
 * 컷신이 끝나는 시점에 GameState에 등록된 보스의 컨트롤러에 StartEncounter()를 호출해 전투를 연다.
 * 컷신을 지정하지 않으면 연출 없이 즉시 전투가 시작된다(안전 폴백).
 *
 * @note 입력 잠금은 코드가 아니라 배치된 LevelSequenceActor의 Playback 설정
 *       (Disable Movement Input / Disable Look Input)으로 처리한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACEncounterTrigger : public AActor
{
	GENERATED_BODY()

public:
	AACEncounterTrigger();

	/**
	 * @brief 박스 크기가 바뀌어도 아랫면이 액터 원점에 붙어 있도록 위치를 다시 맞춘다.
	 *
	 * @param Transform 액터의 새 트랜스폼
	 * @note 바닥에 놓고 위로만 키우는 배치 방식을 위한 것이다. 박스는 중심 기준이라 그냥 두면
	 *       크기를 키울 때 바닥 아래로도 절반이 파고든다.
	 */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	// 액터 원점 = 박스의 아랫면. 바닥에 배치한 뒤 스케일을 올리면 위쪽으로만 자란다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter")
	TObjectPtr<UBoxComponent> TriggerBox;

	// 재생할 조우 컷신 — 같은 레벨에 배치된 LevelSequenceActor를 지정한다. 비워 두면 컷신 없이 즉시 전투가 시작된다
	UPROPERTY(EditInstanceOnly, Category = "Encounter")
	TObjectPtr<ALevelSequenceActor> EncounterSequence;

private:
	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);

	// 컷신 재생이 끝나면 호출되어 전투를 연다
	UFUNCTION()
	void HandleSequenceFinished();

	// GameState에 등록된 보스의 컨트롤러를 찾아 StartEncounter를 호출한다
	void StartBossEncounter();

	// 이미 발동했는지 여부 (1회용)
	bool bTriggered = false;
};
