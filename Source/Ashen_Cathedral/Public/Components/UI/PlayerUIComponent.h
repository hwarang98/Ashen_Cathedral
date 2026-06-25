// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/UI/PawnUIComponent.h"
#include "PlayerUIComponent.generated.h"

class UACBossClearWidget;

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

	UPROPERTY(BlueprintAssignable, BlueprintAssignable)
	FOnEquippedWeaponChangedDelegate OnEquippedWeaponChangedDelegate;

protected:
	// 보스 클리어 후 표시할 위젯 클래스. BP에서 WBP_BossClear를 할당한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossClear")
	TSubclassOf<UACBossClearWidget> BossClearWidgetClass;

private:
	// GameMode의 보스 전투 완료(Completed) 델리게이트 콜백. bIsFinalBoss로 진행 버튼 문구/초기 활성 상태를 결정
	UFUNCTION()
	void OnBossBattleCompletedReceived(bool bIsFinalBoss);

	// 타겟 락 해제 후, 카드 선택이 진행 중이 아니면 Boss Clear UI를 바로 띄우고, 진행 중이면 선택이 끝날 때까지 미룬다
	void ShowBossClearUI(bool bIsFinalBoss);

	// Boss Clear UI 위젯을 생성해 뷰포트에 추가하고 입력 모드를 전환
	void CreateAndShowBossClearWidget(bool bIsFinalBoss);

	// 보상 카드 선택이 끝났을 때 호출 — 미뤄둔 Boss Clear UI를 표시
	void OnAbilitySelectionClosed();

	// Blueprint(WBP_BossClear)에서 진행 버튼 클릭 시 호출됨
	UFUNCTION()
	void OnNextButtonClicked();

	UPROPERTY()
	TObjectPtr<UACBossClearWidget> ActiveBossClearWidget;

	// Boss Clear UI가 이미 표시된 적이 있는지 여부 (중복 표시 방지)
	bool bClearUIShown = false;

	// 진행 버튼이 이미 클릭됐는지 여부 (중복 클릭 방지)
	bool bProgressRequested = false;
};
