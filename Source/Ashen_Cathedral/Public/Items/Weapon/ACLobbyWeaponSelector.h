// 로비에 배치해 상호작용하면 지정된 WeaponData의 무기로 교체시키는 무기 선택대

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/InteractableInterface.h"
#include "ACLobbyWeaponSelector.generated.h"

class AACPlayerCharacter;
class AACWeaponBase;
class UACDataAsset_WeaponData;
class UACWeaponSelectionSubsystem;
class USphereComponent;

/**
 * 로비 맵에 배치하는 범용 무기 선택대.
 * 인스턴스마다 WeaponData 하나만 지정하면 동작하며, C++에는 특정 무기 참조가 없다.
 * BeginPlay에서 WeaponData의 클래스로 표시 전용 프리뷰 무기를 스폰한다.
 * 상호작용하면 연출을 시작하고, 연출이 끝나면 UPlayerCombatComponent의 교체 파이프라인을 호출한다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACLobbyWeaponSelector : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	AACLobbyWeaponSelector();

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * @brief 이 선택대의 무기로 교체를 시작한다.
	 * 다른 교체가 진행 중이거나 이미 이 무기를 선택한 상태면 무시한다.
	 * 연출 훅을 호출한 뒤 FlyToPlayerDuration 후에 실제 교체가 커밋된다.
	 * @param InstigatorPawn 상호작용을 시작한 Pawn
	 */
	virtual void Interact(APawn* InstigatorPawn) override;

	virtual FText GetInteractionText() const override;

	FORCEINLINE UACDataAsset_WeaponData* GetWeaponData() const { return WeaponData; }

	// 표시 전용 프리뷰 무기 액터. BP 연출에서 이동/숨김 처리에 사용한다
	UFUNCTION(BlueprintPure, Category = "WeaponSelection")
	FORCEINLINE AACWeaponBase* GetPreviewWeapon() const { return PreviewWeapon; }

	// 연출이 일찍 끝났을 때 BP 타임라인이 호출해 교체를 앞당길 수 있다. 두 번 호출해도 안전하다
	UFUNCTION(BlueprintCallable, Category = "WeaponSelection")
	void CommitWeaponSwap();

protected:
	// 이 선택대가 지급할 무기 데이터. 프리뷰 스폰과 교체 요청 모두 이 값만 사용한다.
	// BP 기본값으로 무기별 자식 BP를 만들거나, 배치된 액터마다 따로 지정할 수 있다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponSelection")
	TObjectPtr<UACDataAsset_WeaponData> WeaponData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<USphereComponent> InteractionSphere;

	// 프리뷰 무기를 붙이는 지점. BP 타임라인이 이 컴포넌트를 부유·회전시키고 플레이어에게 이동시킨다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WeaponSelection")
	TObjectPtr<USceneComponent> PreviewAnchor;

	// 상호작용 프롬프트에 표시할 문구
	UPROPERTY(EditAnywhere, Category = "Interaction")
	FText InteractionText = FText::FromString(TEXT("무기를 손에 쥔다"));

	// 무기가 플레이어에게 날아가는 연출 길이. 이 시간이 지나면 연출과 무관하게 반드시 교체가 커밋된다.
	// 0이면 상호작용 즉시 교체한다 (BP 연출을 붙일 때만 올린다)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponSelection", meta = (ClampMin = "0.0"))
	float FlyToPlayerDuration = 0.f;

	// 교체 시 기존 무기를 집어넣는 해제 몽타주를 재생한다. 끄면 이전 무기가 즉시 사라진다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponSelection")
	bool bPlayUnequipMontage = true;

	// 새 무기를 뽑는 장착 몽타주를 재생한다. 끄면 곧바로 손에 들어온다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponSelection")
	bool bPlayEquipMontage = false;

	// 무기가 플레이어에게 날아가는 연출을 시작한다. BP에서 타임라인으로 구현한다
	UFUNCTION(BlueprintImplementableEvent, Category = "WeaponSelection")
	void BP_OnFlyToPlayerStarted(APawn* TargetPawn);

	// 교체가 끝났을 때(성공/실패 모두) 호출된다. BP에서 프리뷰 무기를 원위치로 복귀·재표시한다
	UFUNCTION(BlueprintImplementableEvent, Category = "WeaponSelection")
	void BP_OnSwapFinished(bool bSuccess);

	// 이 선택대의 무기가 현재 선택된 무기인지 여부가 바뀔 때 호출된다. BP에서 하이라이트를 처리한다
	UFUNCTION(BlueprintImplementableEvent, Category = "WeaponSelection")
	void BP_OnSelectionStateChanged(bool bIsSelected);

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

	UFUNCTION()
	void HandleSelectedWeaponChanged(UACDataAsset_WeaponData* OldWeaponData, UACDataAsset_WeaponData* NewWeaponData);

	UFUNCTION()
	void HandleWeaponSwapFinished(bool bSuccess);

	/**
	 * @brief WeaponData->WeaponClassToSpawn으로 표시 전용 프리뷰 무기를 스폰한다.
	 * CombatComponent에 등록하지 않고 어빌리티도 부여하지 않으며 콜리전과 틱을 모두 끈다.
	 * 실제 무기와 같은 클래스를 쓰므로 시각적으로 동일하게 보인다.
	 */
	void SpawnPreviewWeapon();

	UACWeaponSelectionSubsystem* GetWeaponSelectionSubsystem() const;

	UPROPERTY()
	TObjectPtr<AACWeaponBase> PreviewWeapon;

	TWeakObjectPtr<AACPlayerCharacter> PendingInstigator;

	// 상호작용을 받아 연출~교체가 진행 중인지 여부
	bool bInteractionPending = false;

	// 이번 상호작용에서 교체 파이프라인을 이미 호출했는지 여부
	bool bSwapCommitted = false;

	FTimerHandle FlyToPlayerTimerHandle;
};
