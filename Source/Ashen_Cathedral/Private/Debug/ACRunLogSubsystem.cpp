// 런 단위 통계 누적 + JSON 저장 구현.

#include "Debug/ACRunLogSubsystem.h"

#if AC_WEB_DEBUG

	#include "Character/Player/ACPlayerCharacter.h"
	#include "Debug/ACWebDebugSubsystem.h"
	#include "GameplayTags/ACGameplayTags_Player.h"
	#include "GameplayTags/ACGameplayTags_Shared.h"
	#include "Misc/FileHelper.h"
	#include "Misc/Paths.h"
	#include "Serialization/JsonSerializer.h"
	#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogACRunLog, Log, All);

using FPrettyWriter = TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>;

namespace
{
	/** 무적 종료 후 이 시간을 넘긴 피격은 표본으로 세지 않는다 */
	constexpr double IframeSampleWindowSeconds = 2.0;

	FString GetRunLogDir()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RunLogs"));
	}

	int32 ToMilliseconds(double Seconds)
	{
		return FMath::RoundToInt32(Seconds * 1000.0);
	}
}

void UACRunLogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UACRunLogSubsystem::Deinitialize()
{
	// 게임이 그냥 종료돼도 진행 중이던 런은 남긴다
	if (bRunActive)
	{
		EndRun(TEXT("abandoned"));
	}
	Super::Deinitialize();
}

UACRunLogSubsystem* UACRunLogSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UACRunLogSubsystem>() : nullptr;
}

/* ────────────────────────── 런 / 보스전 수명주기 ────────────────────────── */

void UACRunLogSubsystem::BeginRun(int32 InSeed, const FGameplayTag& InWeapon)
{
	if (bRunActive)
	{
		EndRun(TEXT("abandoned"));
	}

	BossFights.Reset();
	CardsPicked.Reset();
	CurrencyEarned.Reset();
	MetaUpgrades.Reset();

	Seed = InSeed != 0 ? InSeed : FMath::Rand();
	Weapon = InWeapon.IsValid() ? InWeapon.ToString() : FString();
	StartedAtUtc = FDateTime::UtcNow().ToIso8601();
	RunId = FString::Printf(TEXT("run-%05d"), FindNextRunIndex());
	RunStartSeconds = FPlatformTime::Seconds();
	bRunActive = true;
	bFightActive = false;

	ParryAbilityActivatedSeconds = -1.0;
	InvincibleEndedSeconds = -1.0;
	ComboWindowOpenedSeconds = -1.0;
}

int32 UACRunLogSubsystem::BeginBossFight(const FGameplayTag& BossId, int32 Attempt)
{
	if (!bRunActive)
	{
		BeginRun(Seed, FGameplayTag::EmptyTag);
	}

	const FString BossKey = BossId.IsValid() ? BossId.ToString() : FString();
	int32& StoredAttempt = BossAttemptCounts.FindOrAdd(BossKey);
	StoredAttempt = Attempt > 0 ? Attempt : StoredAttempt + 1;

	FACRunBossFight Fight;
	Fight.BossId = BossKey;
	Fight.Attempt = StoredAttempt;
	Fight.StartSeconds = FPlatformTime::Seconds();
	BossFights.Add(MoveTemp(Fight));
	bFightActive = true;

	return StoredAttempt;
}

void UACRunLogSubsystem::EndBossFight(bool bWon, float BossHealthPctAtEnd)
{
	FACRunBossFight* Fight = GetActiveFight();
	if (!Fight)
	{
		return;
	}
	Fight->bWon = bWon;
	Fight->BossHealthPctAtEnd = FMath::Clamp(BossHealthPctAtEnd, 0.f, 1.f);
	Fight->DurationSec = static_cast<float>(FPlatformTime::Seconds() - Fight->StartSeconds);
	bFightActive = false;
}

void UACRunLogSubsystem::EndRun(const FString& Result)
{
	if (!bRunActive)
	{
		return;
	}
	if (bFightActive)
	{
		EndBossFight(false, BossFights.Num() > 0 ? BossFights.Last().BossHealthPctAtEnd : 1.f);
	}

	const float DurationSec = static_cast<float>(FPlatformTime::Seconds() - RunStartSeconds);
	const FString Json = BuildRunJson(Result, DurationSec);
	const FString FullPath = FPaths::Combine(GetRunLogDir(), RunId + TEXT(".json"));

	if (FFileHelper::SaveStringToFile(Json, *FullPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogACRunLog, Display, TEXT("런 로그 저장 — %s (%s, %.1fs, 보스전 %d)"), *FullPath, *Result, DurationSec, BossFights.Num());
	}
	else
	{
		UE_LOG(LogACRunLog, Warning, TEXT("런 로그 저장 실패 — %s"), *FullPath);
	}

	bRunActive = false;
	bFightActive = false;
}

int32 UACRunLogSubsystem::FindNextRunIndex()
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(GetRunLogDir(), TEXT("run-*.json")), true, false);

	int32 Highest = 0;
	for (const FString& File : Files)
	{
		FString Base = FPaths::GetBaseFilename(File);
		Base.RemoveFromStart(TEXT("run-"));
		Highest = FMath::Max(Highest, FCString::Atoi(*Base));
	}
	return Highest + 1;
}

/* ────────────────────────────── 지표 누적 ────────────────────────────── */

void UACRunLogSubsystem::NotifyDamageTaken(const AActor* Target, float Damage, const FGameplayTagContainer& AttackTags, const FString& SourceAbility, float PlayerHealthPctBefore)
{
	FACRunBossFight* Fight = GetActiveFight();
	if (!Fight || Damage <= 0.f)
	{
		return;
	}

	const bool bPlayerTarget = UACWebDebugSubsystem::ResolveSource(Target) == EACWebDebugSource::Player;
	if (!bPlayerTarget)
	{
		Fight->TotalDamageDealt += Damage;
		return;
	}

	Fight->TotalDamageTaken += Damage;

	// 어떤 공격이 얼마나 깎았는지 — Enemy.Ability.* 태그를 우선 집계 키로 쓴다
	FString Key = SourceAbility;
	for (const FGameplayTag& Tag : AttackTags)
	{
		const FString TagName = Tag.ToString();
		if (TagName.StartsWith(TEXT("Enemy.Ability")))
		{
			Key = TagName;
			break;
		}
	}
	if (!Key.IsEmpty())
	{
		Fight->DamageTakenByAttackTag.FindOrAdd(Key) += Damage;
	}

	// 사망타는 마지막 것으로 덮어쓴다 — EndRun 시점에 남아 있는 값이 실제 사망 원인이다
	Fight->bHasDeathCause = true;
	Fight->DeathAttackTag = Key;
	Fight->DeathBossAbility = SourceAbility;
	Fight->DeathPlayerHealthPctBefore = PlayerHealthPctBefore;
	Fight->DeathDamage = Damage;

	// 무적 종료 직후 피격이면 표본으로 남긴다
	if (InvincibleEndedSeconds > 0.0)
	{
		const double Delta = FPlatformTime::Seconds() - InvincibleEndedSeconds;
		if (Delta >= 0.0 && Delta <= IframeSampleWindowSeconds)
		{
			Fight->TimingSamples.HitAfterIframeEndMs.Add(ToMilliseconds(Delta));
		}
		InvincibleEndedSeconds = -1.0;
	}
}

void UACRunLogSubsystem::NotifyGuardBreak(const AActor* Target)
{
	if (FACRunBossFight* Fight = GetActiveFight())
	{
		if (UACWebDebugSubsystem::ResolveSource(Target) == EACWebDebugSource::Player)
		{
			++Fight->GuardBreaksTaken;
		}
		else
		{
			++Fight->GuardBreaksInflicted;
		}
	}
}

void UACRunLogSubsystem::NotifyPostureBreak(const AActor* Target)
{
	if (FACRunBossFight* Fight = GetActiveFight())
	{
		if (UACWebDebugSubsystem::ResolveSource(Target) == EACWebDebugSource::Player)
		{
			++Fight->PostureBreaksTaken;
		}
		else
		{
			++Fight->PostureBreaksInflicted;
		}
	}
}

void UACRunLogSubsystem::NotifyCriticalAttack(const AActor* Instigator)
{
	if (FACRunBossFight* Fight = GetActiveFight())
	{
		if (UACWebDebugSubsystem::ResolveSource(Instigator) == EACWebDebugSource::Player)
		{
			++Fight->CriticalAttacksLanded;
		}
	}
}

void UACRunLogSubsystem::NotifyBlockSuccess()
{
	if (FACRunBossFight* Fight = GetActiveFight())
	{
		++Fight->Block.Attempts;
		++Fight->Block.Successes;
	}
}

void UACRunLogSubsystem::NotifyParryAttempt()
{
	if (FACRunBossFight* Fight = GetActiveFight())
	{
		++Fight->Parry.Attempts;
	}
}

void UACRunLogSubsystem::NotifyParrySuccess()
{
	if (FACRunBossFight* Fight = GetActiveFight())
	{
		++Fight->Parry.Successes;
	}
}

void UACRunLogSubsystem::NotifyDodgeAttempt()
{
	if (FACRunBossFight* Fight = GetActiveFight())
	{
		++Fight->DodgeAttempts;
	}
}

void UACRunLogSubsystem::NotifyIframeNegatedHit()
{
	if (FACRunBossFight* Fight = GetActiveFight())
	{
		++Fight->DodgeIframeSuccesses;
	}
}

void UACRunLogSubsystem::NotifyPlayerHealthPct(float Pct)
{
	if (FACRunBossFight* Fight = GetActiveFight())
	{
		Fight->HealthPctSum += FMath::Clamp(Pct, 0.f, 1.f);
		++Fight->HealthPctSamples;
	}
}

void UACRunLogSubsystem::NotifyCardPicked(const FString& CardId, const FString& Rarity, const FString& Category, int32 StackAfter, const TArray<FString>& OfferedWith, const FGameplayTag& AfterBossId)
{
	FACRunCardPick Pick;
	Pick.CardId = CardId;
	Pick.Rarity = Rarity;
	Pick.Category = Category;
	Pick.StackAfter = StackAfter;
	Pick.OfferedWith = OfferedWith;
	Pick.AfterBossId = AfterBossId.IsValid() ? AfterBossId.ToString() : FString();
	Pick.PickIndex = CardsPicked.Num();
	CardsPicked.Add(MoveTemp(Pick));
}

void UACRunLogSubsystem::NotifyCurrencyEarned(const FGameplayTag& CurrencyTag, int32 Amount)
{
	if (CurrencyTag.IsValid())
	{
		CurrencyEarned.FindOrAdd(CurrencyTag.ToString()) += Amount;
	}
}

void UACRunLogSubsystem::NotifyMetaUpgrade(const FString& UpgradeName, int32 Level)
{
	MetaUpgrades.FindOrAdd(UpgradeName) = Level;
}

/* ────────────────────────────── 타이밍 표본 ────────────────────────────── */

void UACRunLogSubsystem::NotifyParryAbilityActivated()
{
	ParryAbilityActivatedSeconds = FPlatformTime::Seconds();
	NotifyParryAttempt();
}

void UACRunLogSubsystem::NotifyAttackInput()
{
	if (ComboWindowOpenedSeconds < 0.0)
	{
		return;
	}
	if (FACRunBossFight* Fight = GetActiveFight())
	{
		const double Delta = FPlatformTime::Seconds() - ComboWindowOpenedSeconds;
		Fight->TimingSamples.ComboWindowInputOffsetMs.Add(ToMilliseconds(Delta));
	}
	ComboWindowOpenedSeconds = -1.0;
}

void UACRunLogSubsystem::OnTrackedTagChanged(const AActor* Owner, const FGameplayTag& Tag, bool bAdded)
{
	// 타이밍 표본은 플레이어 기준으로만 센다
	if (UACWebDebugSubsystem::ResolveSource(Owner) != EACWebDebugSource::Player)
	{
		return;
	}

	if (Tag == ACGameplayTags::Shared_Status_Parry)
	{
		if (bAdded && ParryAbilityActivatedSeconds > 0.0)
		{
			// 판정 윈도우 시작(T1) 대비 입력(T0). 음수면 윈도우가 열리기 전에 눌렀다는 뜻이다
			if (FACRunBossFight* Fight = GetActiveFight())
			{
				Fight->TimingSamples.ParryInputOffsetMs.Add(ToMilliseconds(ParryAbilityActivatedSeconds - FPlatformTime::Seconds()));
			}
			ParryAbilityActivatedSeconds = -1.0;
		}
		return;
	}

	if (Tag == ACGameplayTags::Shared_Status_Invincible)
	{
		if (!bAdded)
		{
			InvincibleEndedSeconds = FPlatformTime::Seconds();
		}
		return;
	}

	if (Tag == ACGameplayTags::Player_Status_ComboWindow)
	{
		ComboWindowOpenedSeconds = bAdded ? FPlatformTime::Seconds() : -1.0;
		return;
	}

	if (Tag == ACGameplayTags::Player_Status_Rolling && bAdded)
	{
		NotifyDodgeAttempt();
	}
}

/* ────────────────────────────── 직렬화 ────────────────────────────── */

FString UACRunLogSubsystem::BuildRunJson(const FString& Result, float DurationSec) const
{
	FString Json;
	const TSharedRef<FPrettyWriter> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Json);

	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("schema"), 1);
	Writer->WriteValue(TEXT("runId"), RunId);
	Writer->WriteValue(TEXT("seed"), Seed);
	Writer->WriteValue(TEXT("startedAtUtc"), StartedAtUtc);
	Writer->WriteValue(TEXT("durationSec"), DurationSec);
	Writer->WriteValue(TEXT("weapon"), Weapon);
	Writer->WriteValue(TEXT("result"), Result);

	Writer->WriteObjectStart(TEXT("metaUpgrades"));
	for (const TPair<FString, int32>& Pair : MetaUpgrades)
	{
		Writer->WriteValue(Pair.Key, Pair.Value);
	}
	Writer->WriteObjectEnd();

	Writer->WriteObjectStart(TEXT("currencyEarned"));
	for (const TPair<FString, int32>& Pair : CurrencyEarned)
	{
		Writer->WriteValue(Pair.Key, Pair.Value);
	}
	Writer->WriteObjectEnd();

	Writer->WriteArrayStart(TEXT("cardsPicked"));
	for (const FACRunCardPick& Pick : CardsPicked)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("cardId"), Pick.CardId);
		Writer->WriteValue(TEXT("rarity"), Pick.Rarity);
		Writer->WriteValue(TEXT("category"), Pick.Category);
		Writer->WriteValue(TEXT("stackAfter"), Pick.StackAfter);
		Writer->WriteArrayStart(TEXT("offeredWith"));
		for (const FString& Other : Pick.OfferedWith)
		{
			Writer->WriteValue(Other);
		}
		Writer->WriteArrayEnd();
		Writer->WriteValue(TEXT("afterBossId"), Pick.AfterBossId);
		Writer->WriteValue(TEXT("pickIndex"), Pick.PickIndex);
		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();

	Writer->WriteArrayStart(TEXT("bossFights"));
	for (const FACRunBossFight& Fight : BossFights)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("bossId"), Fight.BossId);
		Writer->WriteValue(TEXT("attempt"), Fight.Attempt);
		Writer->WriteValue(TEXT("result"), Fight.bWon ? TEXT("won") : TEXT("lost"));
		Writer->WriteValue(TEXT("durationSec"), Fight.DurationSec);
		Writer->WriteValue(TEXT("bossHealthPctAtEnd"), Fight.BossHealthPctAtEnd);

		Writer->WriteObjectStart(TEXT("parry"));
		Writer->WriteValue(TEXT("attempts"), Fight.Parry.Attempts);
		Writer->WriteValue(TEXT("successes"), Fight.Parry.Successes);
		Writer->WriteObjectEnd();

		Writer->WriteObjectStart(TEXT("dodge"));
		Writer->WriteValue(TEXT("attempts"), Fight.DodgeAttempts);
		Writer->WriteValue(TEXT("iframeSuccesses"), Fight.DodgeIframeSuccesses);
		Writer->WriteObjectEnd();

		Writer->WriteObjectStart(TEXT("block"));
		Writer->WriteValue(TEXT("attempts"), Fight.Block.Attempts);
		Writer->WriteValue(TEXT("successes"), Fight.Block.Successes);
		Writer->WriteObjectEnd();

		Writer->WriteValue(TEXT("guardBreaksTaken"), Fight.GuardBreaksTaken);
		Writer->WriteValue(TEXT("guardBreaksInflicted"), Fight.GuardBreaksInflicted);
		Writer->WriteValue(TEXT("postureBreaksTaken"), Fight.PostureBreaksTaken);
		Writer->WriteValue(TEXT("postureBreaksInflicted"), Fight.PostureBreaksInflicted);
		Writer->WriteValue(TEXT("criticalAttacksLanded"), Fight.CriticalAttacksLanded);
		Writer->WriteValue(TEXT("totalDamageDealt"), Fight.TotalDamageDealt);
		Writer->WriteValue(TEXT("totalDamageTaken"), Fight.TotalDamageTaken);
		Writer->WriteValue(TEXT("avgPlayerHealthPct"),
			Fight.HealthPctSamples > 0 ? static_cast<float>(Fight.HealthPctSum / Fight.HealthPctSamples) : 0.f);

		Writer->WriteObjectStart(TEXT("damageTakenByAttackTag"));
		for (const TPair<FString, float>& Pair : Fight.DamageTakenByAttackTag)
		{
			Writer->WriteValue(Pair.Key, Pair.Value);
		}
		Writer->WriteObjectEnd();

		if (Fight.bHasDeathCause && !Fight.bWon)
		{
			Writer->WriteObjectStart(TEXT("deathCause"));
			Writer->WriteValue(TEXT("attackTag"), Fight.DeathAttackTag);
			Writer->WriteValue(TEXT("bossAbility"), Fight.DeathBossAbility);
			Writer->WriteValue(TEXT("playerHealthPctBefore"), Fight.DeathPlayerHealthPctBefore);
			Writer->WriteValue(TEXT("damage"), Fight.DeathDamage);
			Writer->WriteObjectEnd();
		}

		Writer->WriteObjectStart(TEXT("timingSamples"));
		Writer->WriteArrayStart(TEXT("parryInputOffsetMs"));
		for (int32 Value : Fight.TimingSamples.ParryInputOffsetMs)
		{
			Writer->WriteValue(Value);
		}
		Writer->WriteArrayEnd();
		Writer->WriteArrayStart(TEXT("hitAfterIframeEndMs"));
		for (int32 Value : Fight.TimingSamples.HitAfterIframeEndMs)
		{
			Writer->WriteValue(Value);
		}
		Writer->WriteArrayEnd();
		Writer->WriteArrayStart(TEXT("comboWindowInputOffsetMs"));
		for (int32 Value : Fight.TimingSamples.ComboWindowInputOffsetMs)
		{
			Writer->WriteValue(Value);
		}
		Writer->WriteArrayEnd();
		Writer->WriteObjectEnd();

		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();

	Writer->WriteObjectEnd();
	Writer->Close();
	return Json;
}

#else // !AC_WEB_DEBUG

void UACRunLogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UACRunLogSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

#endif // AC_WEB_DEBUG
