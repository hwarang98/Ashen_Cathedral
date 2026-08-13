// 보스 클리어 후 보상 카드 추첨·적용·정리를 담당하는 컴포넌트 — AACPlayerCharacter에 부착

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpec.h"
#include "Structs/ACStructTypes.h"
#include "ACRewardCardComponent.generated.h"

class UDataTable;
class UACRewardCardSelectionWidget;
class UACAbilitySystemComponent;
class UACRunStateSubsystem;
class AACCharacterBase;

// 카드 선택 UI가 닫혔을 때 1회 전달되는 콜백 — 외부 시스템이 선택 완료를 기다릴 때 바인딩
DECLARE_DELEGATE(FOnSelectionClosed);

// 카드 선택 UI가 닫힐 때마다 브로드캐스트되는 알림 — 단일 캐스트인 FOnSelectionClosed는 Boss Clear UI가 점유하므로 별도로 둔다
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCardSelectionFinished);

/**
 * @brief 로그라이크 카드 보상 컴포넌트.
 *
 * AACPlayerCharacter에 부착하여 사용한다.
 * 보스 사망 델리게이트에 바인딩하거나 RegisterBossCharacter()로 보스를 등록하면
 * 보스 클리어 시 자동으로 카드 3장 추첨 -> UI 표시 -> 효과 적용 흐름이 실행된다.
 *
 * 획득 카드 목록 자체는 이 컴포넌트가 아니라 UACRunStateSubsystem이 보유한다.
 * 이 컴포넌트는 플레이어 캐릭터와 함께 레벨 전환 때 파괴되므로, 새 레벨의 BeginPlay에서
 * 서브시스템에 남아있는 중첩 수만큼 GE·Ability를 다시 적용해 카드 효과를 이어붙인다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ASHEN_CATHEDRAL_API UACRewardCardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACRewardCardComponent();

	virtual void BeginPlay() override;

	/**
	 * @brief 보스 액터를 등록하여 사망 시 카드 선택 UI가 뜨도록 바인딩.
	 * Level Blueprint에서 레벨에 배치된 보스 액터를 그대로 연결할 수 있도록 AActor* 로 받는다.
	 * @param BossActor AACCharacterBase를 상속하는 보스 액터
	 */
	UFUNCTION(BlueprintCallable, Category = "RewardCard")
	void RegisterBossCharacter(AActor* BossActor);

	/**
	 * @brief Blueprint / GameMode에서 직접 클리어 흐름을 시작할 때 호출 (보스 등록 없이 사용 가능)
	 */
	UFUNCTION(BlueprintCallable, Category = "RewardCard")
	void TriggerBossCleared();

	/**
	 * @brief 새 Run 시작 시 호출 — 이전 Run의 카드 효과를 모두 제거하고 상태를 초기화
	 */
	UFUNCTION(BlueprintCallable, Category = "RewardCard")
	void InitializeForNewRun();

	/**
	 * @brief 이 캐릭터에 적용돼 있는 카드 GE·Ability를 모두 제거한다.
	 * 획득 카드 목록은 UACRunStateSubsystem이 관리하므로 여기서 지우지 않는다 —
	 * Run 자체의 종료는 AACGameMode가 서브시스템에 알린다.
	 */
	UFUNCTION(BlueprintCallable, Category = "RewardCard")
	void CleanupRunEffects();

	/**
	 * @brief 위젯에서 플레이어가 카드를 선택했을 때 호출 (NotifyCardSelected를 통해 전달됨)
	 * @param CardID 선택된 카드 ID
	 */
	UFUNCTION(BlueprintCallable, Category = "RewardCard")
	void OnCardSelected(FName CardID);

	// 현재 Run에서 획득한 카드와 중첩 수 (Blueprint에서 읽기 가능)
	UFUNCTION(BlueprintPure, Category = "RewardCard")
	int32 GetCurrentStack(FName CardID) const;

	// 카드 선택 UI가 현재 표시 중인지 여부 — 외부 시스템(Boss Clear UI 등)이 선택 완료를 기다릴 때 사용
	FORCEINLINE bool IsSelectionActive() const { return bSelectionActive; }

	// 카드 선택 UI가 닫혔을 때 1회 호출됨 — CloseSelectionUI() 호출 시점에 바인딩된 콜백을 실행하고 즉시 Unbind
	FOnSelectionClosed OnSelectionClosedDelegate;

	// 카드 선택 UI가 닫힐 때(선택·취소 모두) 브로드캐스트된다. 자동 스테이지 진행처럼 선택 종료를 기다려야 하는 시스템이 구독한다
	UPROPERTY(BlueprintAssignable, Category = "RewardCard")
	FOnCardSelectionFinished OnCardSelectionFinishedDelegate;

protected:
	/**
	 * 카드 목록 DataTable (RowStruct: FACRewardCardData).
	 * DT_RewardCardList 를 할당한다.
	 * CSV 임포트: CardName·Description·Rarity·MaxStack·Weight 컬럼을 엑셀에서 관리.
	 * 에디터 수동 설정: GameplayEffectClass·GrantedAbilityClass·Icon.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RewardCard|Config")
	TObjectPtr<UDataTable> CardDataTable;

	// 카드 선택 위젯 Blueprint 클래스 — WBP_RewardCardSelection을 할당
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RewardCard|Config")
	TSubclassOf<UACRewardCardSelectionWidget> SelectionWidgetClass;

	// 켜면 bIsMVP가 true인 카드만 후보로 제공한다 (테스트/우선 검증용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RewardCard|Config")
	bool bOnlyOfferMVPCards = false;

	#pragma region Rarity Weights
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RewardCard|Rarity", meta = (ClampMin = 0.f))
	float CommonWeight = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RewardCard|Rarity", meta = (ClampMin = 0.f))
	float UncommonWeight = 30.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RewardCard|Rarity", meta = (ClampMin = 0.f))
	float RareWeight = 10.f;

	// Run당 최대 1회 등장하도록 제한됨
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RewardCard|Rarity", meta = (ClampMin = 0.f))
	float LegendaryWeight = 3.f;
	#pragma endregion

private:
	// 보스 사망 델리게이트 콜백
	UFUNCTION()
	void OnBossDeathReceived(AACCharacterBase* DeadCharacter);

	/**
	 * @brief 희귀도 가중치에 따라 제공할 카드 3장을 추첨
	 * @return 제공할 카드 배열 (가용 카드가 3장 미만이면 그보다 적을 수 있음)
	 */
	TArray<FACRewardCardData> GenerateCandidateCards() const;

	/**
	 * @brief 가중치 기반으로 희귀도 하나를 선택
	 * @param bLegendaryAvailable 전설 카드를 후보에 포함할지 여부
	 */
	EACCardRarity SelectRarity(bool bLegendaryAvailable) const;

	// 해당 카드가 현재 제공 가능한 상태인지 확인 (MaxStack·전설 제한 등)
	bool CanCardBeOffered(const FACRewardCardData& Card) const;

	/**
	 * @brief 선택한 카드의 GE·Ability를 플레이어 ASC에 적용
	 * @param Card 적용할 카드 데이터
	 * @param ASC 플레이어 AbilitySystemComponent
	 */
	void ApplyCardEffects(const FACRewardCardData& Card, UACAbilitySystemComponent* ASC);

	// 카드 선택 UI를 생성하고 뷰포트에 추가
	void ShowSelectionUI(const TArray<FACRewardCardData>& Candidates);

	// 카드 선택 UI를 닫고 게임 입력 모드를 복원
	void CloseSelectionUI();

	// 오너(AACPlayerCharacter)의 ASC를 반환 — 없으면 nullptr
	UACAbilitySystemComponent* GetPlayerASC() const;

	// 이 Run의 카드 상태를 보유한 Subsystem을 반환 — 없으면 nullptr
	UACRunStateSubsystem* GetRunState() const;

	/**
	 * @brief 런 상태에 남아있는 카드 중첩만큼 GE·Ability를 다시 적용한다.
	 * 레벨을 넘어오면 ASC가 새로 생성되어 이전 핸들이 전부 무효가 되므로, BeginPlay에서 한 번 복원해야 한다.
	 * @note 같은 레벨에서 다음 보스를 스폰하는 경로는 컴포넌트가 살아있어 BeginPlay가 다시 불리지 않으므로 이중 적용되지 않는다.
	 */
	void RestoreCardsFromRunState();

#if AC_WEB_DEBUG
	// 이번에 제시된 카드 ID 목록 — 런 로그의 픽률(offeredWith) 집계에 쓴다
	TArray<FName> LastOfferedCardIds;
#endif

	// 적용된 Infinite GE 핸들 목록 (Run 종료 시 일괄 제거)
	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;

	// 부여된 Ability 핸들 목록 (Run 종료 시 일괄 제거)
	TArray<FGameplayAbilitySpecHandle> ActiveAbilityHandles;

	// 카드 선택 UI가 현재 표시 중인지 여부 (중복 표시 방지)
	bool bSelectionActive = false;

	UPROPERTY()
	TObjectPtr<UACRewardCardSelectionWidget> ActiveWidget;
};