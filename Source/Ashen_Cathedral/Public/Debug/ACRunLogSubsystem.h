// 런 단위 통계를 누적하고 종료 시 Saved/RunLogs/run-XXXXX.json 으로 저장하는 서브시스템.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "ACRunLogSubsystem.generated.h"

#if AC_WEB_DEBUG

/** 패링/블록처럼 시도 대비 성공을 세는 지표 */
struct FACRunAttemptStat
{
	int32 Attempts = 0;
	int32 Successes = 0;
};

/**
 * 이 도구의 핵심 가치. 세 배열 모두 그 보스전 동안의 전체 표본을 담는다.
 * 부호 규약은 Tools/WebDebug/src/lib/schema.ts 의 TimingSamples 와 같다.
 */
struct FACRunTimingSamples
{
	/** 패링 판정 윈도우 시작 대비 입력 시각. 음수 = 일찍 누름 */
	TArray<int32> ParryInputOffsetMs;
	/** 무적 종료 대비 피격 시각. 양수 = 무적이 끝난 뒤. 2초 이내 표본만 유효 */
	TArray<int32> HitAfterIframeEndMs;
	/** 콤보 윈도우 시작 대비 입력 시각. 음수 = 일찍 누름 */
	TArray<int32> ComboWindowInputOffsetMs;
};

/** 보스전 1회 분량의 지표 */
struct FACRunBossFight
{
	FString BossId;
	int32 Attempt = 0;
	bool bWon = false;
	float DurationSec = 0.f;
	float BossHealthPctAtEnd = 1.f;

	FACRunAttemptStat Parry;
	int32 DodgeAttempts = 0;
	int32 DodgeIframeSuccesses = 0;
	FACRunAttemptStat Block;

	int32 GuardBreaksTaken = 0;
	int32 GuardBreaksInflicted = 0;
	int32 PostureBreaksTaken = 0;
	int32 PostureBreaksInflicted = 0;
	int32 CriticalAttacksLanded = 0;
	float TotalDamageDealt = 0.f;
	float TotalDamageTaken = 0.f;

	/** 평균 플레이어 체력 비율 계산용 누계 */
	double HealthPctSum = 0.0;
	int32 HealthPctSamples = 0;

	TMap<FString, float> DamageTakenByAttackTag;

	bool bHasDeathCause = false;
	FString DeathAttackTag;
	FString DeathBossAbility;
	float DeathPlayerHealthPctBefore = 0.f;
	float DeathDamage = 0.f;

	FACRunTimingSamples TimingSamples;

	double StartSeconds = 0.0;
};

/** 카드 1장 선택 기록 */
struct FACRunCardPick
{
	FString CardId;
	FString Rarity;
	FString Category;
	int32 StackAfter = 0;
	TArray<FString> OfferedWith;
	FString AfterBossId;
	int32 PickIndex = 0;
};

#endif // AC_WEB_DEBUG

/**
 * @brief 런 시작부터 종료까지의 전투 지표를 모아 JSON 으로 남긴다.
 *
 * Shipping 빌드에서는 AC_WEB_DEBUG=0 이므로 본문이 통째로 비어 있다.
 * 런 종료(사망 / 클리어 / 로비 복귀) 시 EndRun() 을 호출하면 파일이 떨어진다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACRunLogSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#if AC_WEB_DEBUG
	static UACRunLogSubsystem* Get(const UObject* WorldContextObject);

	#pragma region 런 / 보스전 수명주기
	/** 새 런을 시작한다. 이전 런이 열려 있으면 abandoned 로 닫는다. Seed 가 0 이면 무작위로 하나 만든다. */
	void BeginRun(int32 InSeed, const FGameplayTag& InWeapon);

	/** 이번 런의 시드. 타임라인 세션 헤더가 같은 값을 쓴다 */
	int32 GetSeed() const { return Seed; }

	/**
	 * @brief 보스전을 시작한다. 런이 열려 있지 않으면 자동으로 하나 연다.
	 * @param BossId 보스 식별 태그
	 * @param Attempt 이 보스에 대한 시도 회차. 0 이하를 넘기면 세션 내 누적 횟수를 직접 센다
	 * @return 실제로 기록된 시도 회차
	 */
	int32 BeginBossFight(const FGameplayTag& BossId, int32 Attempt = 0);

	/**
	 * @brief 진행 중인 보스전을 닫는다.
	 * @param bWon 플레이어가 이겼는지 여부
	 * @param BossHealthPctAtEnd 종료 시점 보스 잔여 체력 비율 (0~1)
	 */
	void EndBossFight(bool bWon, float BossHealthPctAtEnd);

	/**
	 * @brief 런을 닫고 Saved/RunLogs/run-XXXXX.json 을 저장한다.
	 * @param Result "died" / "cleared" / "abandoned"
	 */
	void EndRun(const FString& Result);
	#pragma endregion

	#pragma region 지표 누적 기존 게임플레이 코드가 한 줄씩 호출한다
	void NotifyDamageTaken(const AActor* Target, float Damage, const FGameplayTagContainer& AttackTags, const FString& SourceAbility, float PlayerHealthPctBefore);
	void NotifyGuardBreak(const AActor* Target);
	void NotifyPostureBreak(const AActor* Target);
	void NotifyCriticalAttack(const AActor* Instigator);
	void NotifyBlockSuccess();
	void NotifyParryAttempt();
	void NotifyParrySuccess();
	void NotifyDodgeAttempt();
	/** 무적 태그 덕분에 피해가 완전히 무효화된 순간 — 회피 성공(i-frame) 표본이 된다 */
	void NotifyIframeNegatedHit();
	void NotifyPlayerHealthPct(float Pct);
	void NotifyCardPicked(const FString& CardId, const FString& Rarity, const FString& Category, int32 StackAfter, const TArray<FString>& OfferedWith, const FGameplayTag& AfterBossId);
	void NotifyCurrencyEarned(const FGameplayTag& CurrencyTag, int32 Amount);
	void NotifyMetaUpgrade(const FString& UpgradeName, int32 Level);
	#pragma endregion

	#pragma region 타이밍 표본
	/** 패링 어빌리티가 활성화된 시각(T0)을 기록한다. 뒤이어 Parry 태그가 붙으면 오프셋이 확정된다. */
	void NotifyParryAbilityActivated();

	/** 공격 입력이 들어온 시각(T1). 콤보 윈도우가 열려 있었다면 오프셋 표본을 남긴다. */
	void NotifyAttackInput();

	/**
	 * @brief WebDebug 가 구독한 상태 태그 변화를 그대로 넘겨받아 타이밍 표본을 수집한다.
	 * 태그 구독은 UACWebDebugSubsystem 한 곳에만 두고, 여기서는 필요한 태그만 골라 쓴다.
	 */
	void OnTrackedTagChanged(const AActor* Owner, const FGameplayTag& Tag, bool bAdded);
	#pragma endregion

private:
	FString BuildRunJson(const FString& Result, float DurationSec) const;
	/** Saved/RunLogs 를 훑어 다음 런 번호를 정한다 */
	static int32 FindNextRunIndex();

	FACRunBossFight* GetActiveFight() { return BossFights.Num() > 0 && bFightActive ? &BossFights.Last() : nullptr; }

	TArray<FACRunBossFight> BossFights;
	/** 보스별 누적 시도 횟수 — 런을 새로 시작해도 초기화하지 않는다(학습 곡선의 x축) */
	TMap<FString, int32> BossAttemptCounts;
	TArray<FACRunCardPick> CardsPicked;
	TMap<FString, int32> CurrencyEarned;
	TMap<FString, int32> MetaUpgrades;

	FString RunId;
	FString StartedAtUtc;
	FString Weapon;
	int32 Seed = 0;
	double RunStartSeconds = 0.0;
	bool bRunActive = false;
	bool bFightActive = false;

	/** 패링 어빌리티 활성 시각 — Parry 태그가 붙는 순간과의 차이가 표본이 된다 */
	double ParryAbilityActivatedSeconds = -1.0;
	/** 무적 태그가 제거된 시각 — 다음 피격까지의 차이가 표본이 된다 */
	double InvincibleEndedSeconds = -1.0;
	/** 콤보 윈도우가 열린 시각 — 다음 공격 입력까지의 차이가 표본이 된다 */
	double ComboWindowOpenedSeconds = -1.0;
#endif // AC_WEB_DEBUG
};
