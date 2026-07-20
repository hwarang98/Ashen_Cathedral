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

	// Boss Clear 연출이 이미 표시 중인지 여부 (중복 표시 방지)
	bool bClearUIShown = false;

	// Boss Clear 연출을 자동으로 제거하기 위한 타이머 핸들
	FTimerHandle ClearWidgetHideTimerHandle;
};
