// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/RewardCard/ACRewardCardComponent.h"
#include "Widget/ACRewardCardSelectionWidget.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Subsystems/ACRunStateSubsystem.h"
#include "Character/ACCharacterBase.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"

#if AC_WEB_DEBUG
	#include "Debug/ACRunLogSubsystem.h"

namespace
{
	// 런 로그 JSON 에 그대로 들어가는 문자열 — 리플렉션 대신 직접 매핑해 스키마 값을 고정한다
	const TCHAR* CardRarityToString(EACCardRarity Rarity)
	{
		switch (Rarity)
		{
			case EACCardRarity::Uncommon:
				return TEXT("Uncommon");
			case EACCardRarity::Rare:
				return TEXT("Rare");
			case EACCardRarity::Legendary:
				return TEXT("Legendary");
			default:
				return TEXT("Common");
		}
	}

	const TCHAR* CardCategoryToString(EACCardCategory Category)
	{
		switch (Category)
		{
			case EACCardCategory::Defense:
				return TEXT("Defense");
			case EACCardCategory::Mobility:
				return TEXT("Mobility");
			case EACCardCategory::Parry:
				return TEXT("Parry");
			case EACCardCategory::Resource:
				return TEXT("Resource");
			default:
				return TEXT("Attack");
		}
	}
}
#endif

UACRewardCardComponent::UACRewardCardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACRewardCardComponent::BeginPlay()
{
	Super::BeginPlay();

	RestoreCardsFromRunState();
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
	bSelectionActive = false;

	CloseSelectionUI();
}

void UACRewardCardComponent::RestoreCardsFromRunState()
{
	const UACRunStateSubsystem* RunState = GetRunState();
	if (!RunState || !CardDataTable)
	{
		return;
	}

	const TMap<FName, int32>& AcquiredStacks = RunState->GetAcquiredCardStacks();
	if (AcquiredStacks.IsEmpty())
	{
		return;
	}

	UACAbilitySystemComponent* ASC = GetPlayerASC();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACRewardCardComponent] ASC가 없어 이전 레벨의 카드 효과를 복원하지 못했습니다."));
		return;
	}

	// 중첩은 획득할 때마다 GE를 한 번씩 더 적용하는 방식이므로, 복원도 중첩 수만큼 반복 적용해야 원래 상태와 같아진다
	for (const TPair<FName, int32>& Pair : AcquiredStacks)
	{
		const FACRewardCardData* FoundCard = CardDataTable->FindRow<FACRewardCardData>(Pair.Key, TEXT("RestoreCardsFromRunState"));
		if (!FoundCard)
		{
			continue;
		}

		for (int32 StackIndex = 0; StackIndex < Pair.Value; ++StackIndex)
		{
			ApplyCardEffects(*FoundCard, ASC);
		}
	}
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

	// 중첩 수 증가 — 레벨을 넘어가도 유지되어야 하므로 런 상태 Subsystem이 보관한다
	if (UACRunStateSubsystem* RunState = GetRunState())
	{
		RunState->AddCardStack(CardID, FoundCard->Rarity == EACCardRarity::Legendary);
	}

#if AC_WEB_DEBUG
	// 런 로그 카드 픽 기록 — 선택된 카드와 함께 제시된 카드를 같이 남겨야 픽률을 낼 수 있다
	if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(GetOwner()))
	{
		TArray<FString> OfferedWith;
		for (const FName& OfferedId : LastOfferedCardIds)
		{
			if (OfferedId != CardID)
			{
				OfferedWith.Add(OfferedId.ToString());
			}
		}
		RunLog->NotifyCardPicked(
			CardID.ToString(),
			CardRarityToString(FoundCard->Rarity),
			CardCategoryToString(FoundCard->Category),
			GetCurrentStack(CardID),
			OfferedWith,
			FGameplayTag::EmptyTag);
	}
#endif

	// GameplayEffect, Ability 적용
	UACAbilitySystemComponent* ASC = GetPlayerASC();
	ApplyCardEffects(*FoundCard, ASC);

	CloseSelectionUI();
}

int32 UACRewardCardComponent::GetCurrentStack(FName CardID) const
{
	const UACRunStateSubsystem* RunState = GetRunState();
	return RunState ? RunState->GetCardStack(CardID) : 0;
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

	const UACRunStateSubsystem* RunState = GetRunState();

	TArray<FACRewardCardData> Result;
	TArray<FACRewardCardData> RemainingPool = EligiblePool;
	const bool bLegendaryAvailable = !RunState || !RunState->IsLegendaryUsed();
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

	if (bOnlyOfferMVPCards && !Card.bIsMVP)
	{
		return false;
	}

	const UACRunStateSubsystem* RunState = GetRunState();

	// MaxStack 도달 여부 확인
	if (RunState && RunState->GetCardStack(Card.CardID) >= Card.MaxStack)
	{
		return false;
	}

	// Run당 전설 카드 1회 제한
	if (Card.Rarity == EACCardRarity::Legendary && RunState && RunState->IsLegendaryUsed())
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

#if AC_WEB_DEBUG
	// 픽률의 분모 — 무엇이 함께 제시됐는지 기억해 둔다
	LastOfferedCardIds.Reset();
	for (const FACRewardCardData& Card : Candidates)
	{
		LastOfferedCardIds.Add(Card.CardID);
	}
#endif

	// 카드 데이터 → 표시 정보 변환
	TArray<FACRewardCardDisplayInfo> DisplayInfos;
	DisplayInfos.Reserve(Candidates.Num());
	for (const FACRewardCardData& Card : Candidates)
	{
		FACRewardCardDisplayInfo Info;
		Info.CardData = Card;
		Info.CurrentStack = GetCurrentStack(Card.CardID);
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

	// 카드 선택 완료를 기다리던 외부 시스템(Boss Clear UI 등)에 1회 통지
	if (OnSelectionClosedDelegate.IsBound())
	{
		OnSelectionClosedDelegate.Execute();
		OnSelectionClosedDelegate.Unbind();
	}

	// 위 단일 캐스트는 Boss Clear UI가 점유하므로, 그 밖의 구독자에게는 이쪽으로 통지한다
	OnCardSelectionFinishedDelegate.Broadcast();
}

UACRunStateSubsystem* UACRewardCardComponent::GetRunState() const
{
	return UACRunStateSubsystem::Get(GetOwner());
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