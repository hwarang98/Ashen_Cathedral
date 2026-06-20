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
class AACCharacterBase;

/**
 * @brief 로그라이크 카드 보상 컴포넌트.
 *
 * AACPlayerCharacter에 부착하여 사용한다.
 * 보스 사망 델리게이트에 바인딩하거나 RegisterBossCharacter()로 보스를 등록하면
 * 보스 클리어 시 자동으로 카드 3장 추첨 -> UI 표시 -> 효과 적용 흐름이 실행된다.
 *
 * Run 종료 시 CleanupRunEffects()를 호출해 적용된 GE와 Ability를 제거한다.
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
	 * @brief Run 종료 / 사망 / 로비 복귀 시 호출 — 적용된 GE·Ability를 모두 제거하고 스택 초기화
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

	// Run 중 획득한 카드 ID → 현재 중첩 수
	TMap<FName, int32> AcquiredStacks;

	// 적용된 Infinite GE 핸들 목록 (Run 종료 시 일괄 제거)
	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;

	// 부여된 Ability 핸들 목록 (Run 종료 시 일괄 제거)
	TArray<FGameplayAbilitySpecHandle> ActiveAbilityHandles;

	// 이번 Run에서 전설 카드를 이미 획득했는지 여부
	bool bLegendaryUsedThisRun = false;

	// 카드 선택 UI가 현재 표시 중인지 여부 (중복 표시 방지)
	bool bSelectionActive = false;

	UPROPERTY()
	TObjectPtr<UACRewardCardSelectionWidget> ActiveWidget;
};