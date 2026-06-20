// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/RewardCard/ACRewardCardComponent.h"
#include "Widget/ACRewardCardSelectionWidget.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Character/ACCharacterBase.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"

UACRewardCardComponent::UACRewardCardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACRewardCardComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UACRewardCardComponent::RegisterBossCharacter(AActor* BossActor)
{
	AACCharacterBase* BossCharacter = Cast<AACCharacterBase>(BossActor);
	if (!BossCharacter)
	{
		return;
	}

	// 이미 바인딩된 경우 중복 등록 방지
	if (!BossCharacter->OnDeathDelegate.IsAlreadyBound(this, &ThisClass::OnBossDeathReceived))
	{
		BossCharacter->OnDeathDelegate.AddDynamic(this, &ThisClass::OnBossDeathReceived);
	}
}

void UACRewardCardComponent::TriggerBossCleared()
{
	OnBossDeathReceived(nullptr);
}

void UACRewardCardComponent::InitializeForNewRun()
{
	CleanupRunEffects();
}

void UACRewardCardComponent::CleanupRunEffects()
{
	UACAbilitySystemComponent* ASC = GetPlayerASC();

	if (ASC)
	{
		for (const FActiveGameplayEffectHandle& Handle : ActiveEffectHandles)
		{
			if (Handle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(Handle);
			}
		}

		for (const FGameplayAbilitySpecHandle& Handle : ActiveAbilityHandles)
		{
			if (Handle.IsValid())
			{
				ASC->ClearAbility(Handle);
			}
		}
	}

	ActiveEffectHandles.Empty();
	ActiveAbilityHandles.Empty();
	AcquiredStacks.Empty();
	bLegendaryUsedThisRun = false;
	bSelectionActive = false;

	CloseSelectionUI();
}

void UACRewardCardComponent::OnCardSelected(FName CardID)
{
	if (!CardDataTable)
	{
		return;
	}

	const FACRewardCardData* FoundCard = CardDataTable->FindRow<FACRewardCardData>(CardID, TEXT("OnCardSelected"));

	if (!FoundCard)
	{
		CloseSelectionUI();
		return;
	}

	// 중첩 수 증가
	int32& Stack = AcquiredStacks.FindOrAdd(CardID);
	Stack++;

	if (FoundCard->Rarity == EACCardRarity::Legendary)
	{
		bLegendaryUsedThisRun = true;
	}

	// GameplayEffect, Ability 적용
	UACAbilitySystemComponent* ASC = GetPlayerASC();
	ApplyCardEffects(*FoundCard, ASC);

	CloseSelectionUI();
}

int32 UACRewardCardComponent::GetCurrentStack(FName CardID) const
{
	const int32* Stack = AcquiredStacks.Find(CardID);
	return Stack ? *Stack : 0;
}

void UACRewardCardComponent::OnBossDeathReceived(AACCharacterBase* DeadCharacter)
{
	// 이미 선택 UI가 활성화된 경우 중복 진입 방지
	if (bSelectionActive)
	{
		return;
	}

	TArray<FACRewardCardData> Candidates = GenerateCandidateCards();

	if (Candidates.IsEmpty())
	{
		// 제공할 카드가 없으면 선택 UI 없이 진행
		return;
	}

	ShowSelectionUI(Candidates);
}

TArray<FACRewardCardData> UACRewardCardComponent::GenerateCandidateCards() const
{
	if (!CardDataTable)
	{
		return {};
	}

	// DataTable 전체 행을 순회하며 제공 가능한 카드 풀 구성
	// CardID는 RowName으로 자동 채워 런타임 키로 활용한다
	TArray<FACRewardCardData> EligiblePool;
	for (const auto& Pair : CardDataTable->GetRowMap())
	{
		const FACRewardCardData* Row = reinterpret_cast<const FACRewardCardData*>(Pair.Value);
		if (!Row)
		{
			continue;
		}

		FACRewardCardData Card = *Row;
		Card.CardID = Pair.Key; // RowName → CardID

		if (CanCardBeOffered(Card))
		{
			EligiblePool.Add(Card);
		}
	}

	if (EligiblePool.IsEmpty())
	{
		return {};
	}

	TArray<FACRewardCardData> Result;
	TArray<FACRewardCardData> RemainingPool = EligiblePool;
	const bool bLegendaryAvailable = !bLegendaryUsedThisRun;
	const int32 NumToPick = FMath::Min(3, RemainingPool.Num());

	for (int32 i = 0; i < NumToPick && RemainingPool.Num() > 0; ++i)
	{
		const EACCardRarity TargetRarity = SelectRarity(bLegendaryAvailable);

		// 목표 희귀도에 해당하는 카드 필터링
		TArray<int32> RarityIndices;
		for (int32 j = 0; j < RemainingPool.Num(); ++j)
		{
			if (RemainingPool[j].Rarity == TargetRarity)
			{
				RarityIndices.Add(j);
			}
		}

		int32 PickedIndex;
		if (RarityIndices.IsEmpty())
		{
			// 해당 희귀도 카드가 없으면 전체 풀에서 랜덤 선택
			PickedIndex = FMath::RandRange(0, RemainingPool.Num() - 1);
		}
		else
		{
			const int32 RandomIdx = FMath::RandRange(0, RarityIndices.Num() - 1);
			PickedIndex = RarityIndices[RandomIdx];
		}

		Result.Add(RemainingPool[PickedIndex]);
		RemainingPool.RemoveAt(PickedIndex);
	}

	return Result;
}

EACCardRarity UACRewardCardComponent::SelectRarity(bool bLegendaryAvailable) const
{
	float Total = CommonWeight + UncommonWeight + RareWeight;
	if (bLegendaryAvailable)
	{
		Total += LegendaryWeight;
	}

	const float Roll = FMath::FRandRange(0.f, Total);
	float Cumulative = CommonWeight;

	if (Roll < Cumulative)
	{
		return EACCardRarity::Common;
	}

	Cumulative += UncommonWeight;
	if (Roll < Cumulative)
	{
		return EACCardRarity::Uncommon;
	}

	Cumulative += RareWeight;
	if (Roll < Cumulative)
	{
		return EACCardRarity::Rare;
	}

	return EACCardRarity::Legendary;
}

bool UACRewardCardComponent::CanCardBeOffered(const FACRewardCardData& Card) const
{
	if (!Card.IsValid())
	{
		return false;
	}

	// MaxStack 도달 여부 확인
	const int32* CurrentStack = AcquiredStacks.Find(Card.CardID);
	if (CurrentStack && *CurrentStack >= Card.MaxStack)
	{
		return false;
	}

	// Run당 전설 카드 1회 제한
	if (Card.Rarity == EACCardRarity::Legendary && bLegendaryUsedThisRun)
	{
		return false;
	}

	return true;
}

void UACRewardCardComponent::ApplyCardEffects(const FACRewardCardData& Card, UACAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	// GameplayEffect 적용
	if (Card.GameplayEffectClass)
	{
		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddSourceObject(GetOwner());

		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
			Card.GameplayEffectClass, 1.f, ContextHandle);

		if (SpecHandle.IsValid())
		{
			const FActiveGameplayEffectHandle EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			if (EffectHandle.IsValid())
			{
				ActiveEffectHandles.Add(EffectHandle);
			}
		}
	}

	// Passive Ability 부여
	if (Card.GrantedAbilityClass)
	{
		// TSubclassOf<UACGameplayAbility> → TSubclassOf<UGameplayAbility> 명시 변환 (MSVC 암시 변환 거부 대응)
		const TSubclassOf<UGameplayAbility> AbilityClass = Card.GrantedAbilityClass;
		FGameplayAbilitySpec AbilitySpec(AbilityClass);
		AbilitySpec.SourceObject = ASC->GetAvatarActor();
		const FGameplayAbilitySpecHandle AbilityHandle = ASC->GiveAbility(AbilitySpec);
		if (AbilityHandle.IsValid())
		{
			ActiveAbilityHandles.Add(AbilityHandle);
		}
	}
}

void UACRewardCardComponent::ShowSelectionUI(const TArray<FACRewardCardData>& Candidates)
{
	if (bSelectionActive || Candidates.IsEmpty())
	{
		return;
	}

	if (!SelectionWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACRewardCardComponent] SelectionWidgetClass가 설정되지 않았습니다."));
		return;
	}

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	bSelectionActive = true;

	// 카드 데이터 → 표시 정보 변환
	TArray<FACRewardCardDisplayInfo> DisplayInfos;
	DisplayInfos.Reserve(Candidates.Num());
	for (const FACRewardCardData& Card : Candidates)
	{
		FACRewardCardDisplayInfo Info;
		Info.CardData = Card;
		const int32* Stack = AcquiredStacks.Find(Card.CardID);
		Info.CurrentStack = Stack ? *Stack : 0;
		DisplayInfos.Add(Info);
	}

	// 위젯 생성 및 뷰포트 추가
	ActiveWidget = CreateWidget<UACRewardCardSelectionWidget>(PC, SelectionWidgetClass);
	if (!ActiveWidget)
	{
		bSelectionActive = false;
		return;
	}

	ActiveWidget->OnCardSelectedDelegate.BindUObject(this, &ThisClass::OnCardSelected);
	ActiveWidget->AddToViewport();
	ActiveWidget->SetupCards(DisplayInfos);

	// UI Only 입력 모드로 전환 (전투 입력 차단)
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ActiveWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
}

void UACRewardCardComponent::CloseSelectionUI()
{
	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr;
	}

	bSelectionActive = false;

	// 게임 입력 모드 복원
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(false);
		}
	}
}

UACAbilitySystemComponent* UACRewardCardComponent::GetPlayerASC() const
{
	if (const AActor* Owner = GetOwner())
	{
		if (const IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Owner))
		{
			return Cast<UACAbilitySystemComponent>(ASCInterface->GetAbilitySystemComponent());
		}
	}
	return nullptr;
}