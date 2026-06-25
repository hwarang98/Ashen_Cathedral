// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/UI/PlayerUIComponent.h"
#include "ACGameplayTags.h"
#include "Blueprint/UserWidget.h"
#include "Character/ACCharacterBase.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/RewardCard/ACRewardCardComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/ACGameMode.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Widget/ACBossClearWidget.h"

void UPlayerUIComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AACGameMode* ACGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AACGameMode>() : nullptr)
	{
		if (!ACGameMode->OnBossBattleCompletedDelegate.IsAlreadyBound(this, &ThisClass::OnBossBattleCompletedReceived))
		{
			ACGameMode->OnBossBattleCompletedDelegate.AddDynamic(this, &ThisClass::OnBossBattleCompletedReceived);
		}
	}
}

void UPlayerUIComponent::OnBossBattleCompletedReceived(bool bIsFinalBoss)
{
	ShowBossClearUI(bIsFinalBoss);
}

void UPlayerUIComponent::ShowBossClearUI(bool bIsFinalBoss)
{
	// Completed 이벤트는 보스당 한 번만 와야 하지만, 방어적으로 중복 표시를 막는다.
	if (bClearUIShown)
	{
		return;
	}
	bClearUIShown = true;
	bProgressRequested = false;

	// 타겟 락 해제 — 보스가 이미 죽어 자동 취소되는 경로가 있지만, 다른 대상을 락온 중인 경우를 방어한다.
	AACCharacterBase* OwnerCharacter = Cast<AACCharacterBase>(GetOwner());
	if (OwnerCharacter)
	{
		if (UACAbilitySystemComponent* ASC = OwnerCharacter->GetACAbilitySystemComponent())
		{
			FGameplayTagContainer TargetLockTag;
			TargetLockTag.AddTag(ACGameplayTags::Player_Ability_TargetLock);
			ASC->CancelAbilities(&TargetLockTag);
		}
	}

	// 일반 보스는 카드 선택이 끝날 때까지 Boss Clear UI 자체를 미룬다 (두 UI가 동시에 뜨는 것을 방지)
	const AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(OwnerCharacter);
	UACRewardCardComponent* RewardCardComponent = PlayerCharacter ? PlayerCharacter->GetRewardCardComponent() : nullptr;

	if (!bIsFinalBoss && RewardCardComponent && RewardCardComponent->IsSelectionActive())
	{
		RewardCardComponent->OnSelectionClosedDelegate.BindUObject(this, &ThisClass::OnAbilitySelectionClosed);
		return;
	}

	CreateAndShowBossClearWidget(bIsFinalBoss);
}

void UPlayerUIComponent::CreateAndShowBossClearWidget(bool bIsFinalBoss)
{
	if (!BossClearWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerUIComponent] BossClearWidgetClass가 설정되지 않았습니다."));
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	ActiveBossClearWidget = CreateWidget<UACBossClearWidget>(PlayerController, BossClearWidgetClass);
	if (!ActiveBossClearWidget)
	{
		return;
	}

	ActiveBossClearWidget->OnNextRequestedDelegate.BindUObject(this, &ThisClass::OnNextButtonClicked);
	ActiveBossClearWidget->AddToViewport();

	AACGameMode* ACGameMode = GetWorld()->GetAuthGameMode<AACGameMode>();
	if (ACGameMode)
	{
		ActiveBossClearWidget->SetNextButtonText(bIsFinalBoss ? ACGameMode->GetReturnToLobbyButtonText() : ACGameMode->GetNextBossButtonText());
	}

	// 카드 선택이 끝난 뒤(또는 선택이 필요 없는 최종 보스)에만 생성되므로 항상 활성화 상태로 띄운다.
	ActiveBossClearWidget->SetNextButtonEnabled(true);

	// UI Only 입력 모드로 전환 (전투 입력 차단)
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ActiveBossClearWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
}

void UPlayerUIComponent::OnAbilitySelectionClosed()
{
	// 카드 선택이 끝난 시점에 비로소 Boss Clear UI를 띄운다 (최종 보스는 이 경로를 타지 않으므로 false로 고정)
	CreateAndShowBossClearWidget(false);
}

void UPlayerUIComponent::OnNextButtonClicked()
{
	if (bProgressRequested)
	{
		return;
	}
	bProgressRequested = true;

	if (ActiveBossClearWidget)
	{
		ActiveBossClearWidget->RemoveFromParent();
		ActiveBossClearWidget = nullptr;
	}

	// 입력 모드를 게임 모드로 복원 — 다음 보스가 스폰되거나 레벨이 전환된 뒤에도 UI 포커스가 남아 입력이 막히는 것을 방지
	if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(false);
	}

	// 실제 진행(다음 보스 스폰 / Lobby 이동)은 GameMode가 책임진다 — UI는 요청만 전달
	if (AACGameMode* ACGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AACGameMode>() : nullptr)
	{
		ACGameMode->RequestProgressAfterBossClear();
	}

	bClearUIShown = false;
}