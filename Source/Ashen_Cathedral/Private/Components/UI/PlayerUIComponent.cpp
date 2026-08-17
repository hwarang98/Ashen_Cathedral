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
#include "Widget/ACInteractionPromptWidget.h"

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

	CreateInteractionPromptWidget();
}

void UPlayerUIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 레벨 전환 중 타이머가 파괴된 위젯을 건드리지 않도록 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ClearWidgetHideTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
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

	// 일반 보스는 카드 선택이 끝날 때까지 Boss Clear 연출 자체를 미룬다 (두 UI가 동시에 뜨는 것을 방지)
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

	ActiveBossClearWidget->AddToViewport();
	ActiveBossClearWidget->PlayClearSequence(bIsFinalBoss ? FinalClearText : BossClearText);

	// 연출 전용 UI이므로 입력 모드를 바꾸지 않는다 — 플레이어는 연출 중에도 스테이지 출구까지 이동할 수 있어야 한다.
	GetWorld()->GetTimerManager().SetTimer(ClearWidgetHideTimerHandle, this, &ThisClass::RemoveBossClearWidget, ClearWidgetDisplayDuration, false);
}

void UPlayerUIComponent::RemoveBossClearWidget()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ClearWidgetHideTimerHandle);
	}

	if (ActiveBossClearWidget)
	{
		ActiveBossClearWidget->RemoveFromParent();
		ActiveBossClearWidget = nullptr;
	}

	// 다음 보스를 클리어했을 때 연출이 다시 표시되도록 초기화
	bClearUIShown = false;
}

void UPlayerUIComponent::OnAbilitySelectionClosed()
{
	// 카드 선택이 끝난 시점에 비로소 Boss Clear 연출을 띄운다 (최종 보스는 이 경로를 타지 않으므로 false로 고정)
	CreateAndShowBossClearWidget(false);
}

void UPlayerUIComponent::CreateInteractionPromptWidget()
{
	if (!InteractionPromptWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerUIComponent] InteractionPromptWidgetClass가 설정되지 않았습니다."));
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	ActiveInteractionPromptWidget = CreateWidget<UACInteractionPromptWidget>(PlayerController, InteractionPromptWidgetClass);
	if (!ActiveInteractionPromptWidget)
	{
		return;
	}

	// 상호작용 대상은 수시로 바뀌므로 위젯을 한 번만 만들어 올려두고 이후에는 가시성만 토글한다.
	ActiveInteractionPromptWidget->AddToViewport();
	ActiveInteractionPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void UPlayerUIComponent::ShowInteractionPrompt(const FText& InText)
{
	if (!ActiveInteractionPromptWidget)
	{
		return;
	}

	// 문구를 제공하지 않는 상호작용 대상은 프롬프트를 띄우지 않는다.
	if (InText.IsEmpty())
	{
		HideInteractionPrompt();
		return;
	}

	ActiveInteractionPromptWidget->SetPromptText(InText);
	ActiveInteractionPromptWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPlayerUIComponent::HideInteractionPrompt()
{
	if (!ActiveInteractionPromptWidget)
	{
		return;
	}

	ActiveInteractionPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void UPlayerUIComponent::RegisterHUDWidget(UUserWidget* InWidget)
{
	if (!InWidget)
	{
		return;
	}

	RegisteredHUDWidgets.AddUnique(InWidget);

	// 이미 HUD가 숨겨진 상태에서 새로 등록되면 그 위젯만 튀어나오므로 함께 숨긴다
	if (bHUDHidden)
	{
		SavedHUDVisibilities.Add(InWidget, InWidget->GetVisibility());
		InWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPlayerUIComponent::SetHUDVisible(bool bVisible)
{
	if (bHUDHidden != bVisible)
	{
		return;
	}

	bHUDHidden = !bVisible;

	// 파괴된 위젯이 남아 있을 수 있으므로 순회 전에 정리한다
	RegisteredHUDWidgets.RemoveAll([](const TObjectPtr<UUserWidget>& Widget)
	{
		return Widget == nullptr;
	});

	for (const TObjectPtr<UUserWidget>& Widget : RegisteredHUDWidgets)
	{
		if (!bVisible)
		{
			SavedHUDVisibilities.Add(Widget, Widget->GetVisibility());
			Widget->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		// Visible로 일괄 복원하면 SelfHitTestInvisible 같은 원래 설정이 사라지므로 저장해 둔 값을 되돌린다
		const ESlateVisibility* Saved = SavedHUDVisibilities.Find(Widget);
		Widget->SetVisibility(Saved ? *Saved : ESlateVisibility::Visible);
	}

	if (bVisible)
	{
		SavedHUDVisibilities.Empty();
	}
	else if (ActiveInteractionPromptWidget)
	{
		// 상호작용 프롬프트는 별도로 소유하므로 함께 처리한다 — 컷신 중 "제단에 손을 얹는다" 같은 문구가 남으면 안 된다
		ActiveInteractionPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}
