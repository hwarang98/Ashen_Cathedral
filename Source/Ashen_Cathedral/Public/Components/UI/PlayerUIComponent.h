// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/UI/PawnUIComponent.h"
#include "PlayerUIComponent.generated.h"

class UACBossClearWidget;
class UACInteractionPromptWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedWeaponChangedDelegate, TSoftObjectPtr<UTexture2D>, SoftWeaponIcon);

/**
 *
 */
UCLASS()
class ASHEN_CATHEDRAL_API UPlayerUIComponent : public UPawnUIComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * @brief 상호작용 프롬프트를 표시한다. 플레이어가 상호작용 대상을 등록할 때 호출된다.
	 * @param InText 표시할 문구. 비어 있으면 프롬프트를 숨긴다.
	 */
	void ShowInteractionPrompt(const FText& InText);

	// 상호작용 프롬프트를 숨긴다. 플레이어가 상호작용 대상에서 벗어날 때 호출된다.
	void HideInteractionPrompt();

	/**
	 * @brief 컷신 동안 숨겨야 하는 게임플레이 HUD 위젯을 등록한다.
	 *
	 * @param InWidget 뷰포트에 올린 HUD 위젯
	 * @note 플레이어 오버레이는 BP 어빌리티(GA_Player_DrawOverlayWidget)가 만들어 올리므로 C++이 참조를
	 *       갖지 못한다. AddToViewport 직후 이 함수로 넘겨야 SetHUDVisible이 그 위젯까지 제어할 수 있다.
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void RegisterHUDWidget(UUserWidget* InWidget);

	/**
	 * @brief 등록된 게임플레이 HUD를 한꺼번에 숨기거나 되돌린다.
	 *
	 * @param bVisible false면 숨기고, true면 숨기기 직전의 표시 상태로 복원한다
	 * @note 조우 컷신·페이즈 전환 컷신처럼 연출 동안 HUD를 걷어내야 하는 곳에서 사용한다.
	 *       레벨 시퀀스의 Hide HUD 옵션은 레거시 AHUD만 끄기 때문에 UMG 위젯에는 효과가 없다.
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetHUDVisible(bool bVisible);

	UPROPERTY(BlueprintAssignable, BlueprintAssignable)
	FOnEquippedWeaponChangedDelegate OnEquippedWeaponChangedDelegate;

protected:
	// 보스 클리어 후 표시할 위젯 클래스. BP에서 WBP_BossClear를 할당한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossClear")
	TSubclassOf<UACBossClearWidget> BossClearWidgetClass;

	// Boss Clear 연출을 화면에 유지할 시간(초). WBP의 연출 애니메이션 길이보다 길게 잡는다.
	UPROPERTY(EditDefaultsOnly, Category = "BossClear")
	float ClearWidgetDisplayDuration = 4.f;

	// Boss Clear 연출에 표시할 문구 (일반 보스 / 최종 보스)
	UPROPERTY(EditDefaultsOnly, Category = "BossClear")
	FText BossClearText = FText::FromString(TEXT("BOSS FELLED"));

	UPROPERTY(EditDefaultsOnly, Category = "BossClear")
	FText FinalClearText = FText::FromString(TEXT("CATHEDRAL CLEARED"));

	// 상호작용 프롬프트 위젯 클래스. BP에서 WBP_InteractionPrompt를 할당한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	TSubclassOf<UACInteractionPromptWidget> InteractionPromptWidgetClass;

private:
	// GameMode의 보스 전투 완료(Completed) 델리게이트 콜백. bIsFinalBoss로 연출 문구를 결정
	UFUNCTION()
	void OnBossBattleCompletedReceived(bool bIsFinalBoss);

	// 타겟 락 해제 후, 카드 선택이 진행 중이 아니면 Boss Clear 연출을 바로 띄우고, 진행 중이면 선택이 끝날 때까지 미룬다
	void ShowBossClearUI(bool bIsFinalBoss);

	// Boss Clear 연출 위젯을 생성해 뷰포트에 추가하고, 표시 시간이 지나면 스스로 사라지도록 타이머를 건다
	void CreateAndShowBossClearWidget(bool bIsFinalBoss);

	// 표시 시간이 끝난 Boss Clear 연출 위젯을 뷰포트에서 제거한다
	void RemoveBossClearWidget();

	// 보상 카드 선택이 끝났을 때 호출 — 미뤄둔 Boss Clear 연출을 표시
	void OnAbilitySelectionClosed();

	// 상호작용 프롬프트 위젯을 생성해 뷰포트에 올려두고 숨김 상태로 대기시킨다
	void CreateInteractionPromptWidget();

	UPROPERTY()
	TObjectPtr<UACBossClearWidget> ActiveBossClearWidget;

	UPROPERTY()
	TObjectPtr<UACInteractionPromptWidget> ActiveInteractionPromptWidget;

	// 컷신 동안 숨길 게임플레이 HUD 위젯들. BP가 RegisterHUDWidget으로 등록한다
	UPROPERTY()
	TArray<TObjectPtr<UUserWidget>> RegisteredHUDWidgets;

	// 숨기기 직전의 표시 상태 — 복원할 때 원래 값으로 되돌리기 위해 보관한다
	UPROPERTY()
	TMap<TObjectPtr<UUserWidget>, ESlateVisibility> SavedHUDVisibilities;

	// 현재 HUD가 숨겨진 상태인지 여부 (중복 호출 방지)
	bool bHUDHidden = false;

	// Boss Clear 연출이 이미 표시 중인지 여부 (중복 표시 방지)
	bool bClearUIShown = false;

	// Boss Clear 연출을 자동으로 제거하기 위한 타이머 핸들
	FTimerHandle ClearWidgetHideTimerHandle;
};
