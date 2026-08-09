#include "Validation/ACMontageAudit.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_MotionWarping.h"
#include "Animation/AnimNotify/ACAnimNotify_IncomingAttackWarning.h"
#include "Animation/AnimNotify/ACAnimNotifyState_AddGameplayTag.h"
#include "Animation/AnimNotify/ACAnimNotifyState_PlayWeaponTrail.h"
#include "AssetRegistry/AssetRegistryModule.h"

namespace ACMontageAuditInternal
{
	/** 에셋을 로드하기 전에 이름과 경로만으로 공격 몽타주인지 추린다 */
	bool LooksLikeAttackMontage(const FAssetData& AssetData)
	{
		const FString Name = AssetData.AssetName.ToString();
		const FString PackageName = AssetData.PackageName.ToString();

		return Name.Contains(TEXT("Attack"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Combo"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Counter"), ESearchCase::IgnoreCase)
			|| PackageName.Contains(TEXT("/Attack/"), ESearchCase::IgnoreCase);
	}

	/**
	 * 처형·치명타 연출은 체간 붕괴 이후 확정 재생이라 플레이어가 방어할 여지가 없다.
	 * 당하는 쪽 몽타주까지 포함되므로 공격 예고 요구에서 제외한다.
	 */
	bool IsExecutionMontage(const FString& AssetName)
	{
		return AssetName.Contains(TEXT("CriticalAttack"), ESearchCase::IgnoreCase)
			|| AssetName.Contains(TEXT("Execution"), ESearchCase::IgnoreCase);
	}
}

TArray<FACAssetReport> ACMontageAudit::AuditAll()
{
	using namespace ACMontageAuditInternal;

	FARFilter Filter;
	Filter.ClassPaths.Add(UAnimMontage::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(TEXT("/Game/Enemy"));
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	TArray<FAssetData> MontageAssets;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().GetAssets(Filter, MontageAssets);

	MontageAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.AssetName.LexicalLess(Right.AssetName);
	});

	TArray<FACAssetReport> Reports;
	for (const FAssetData& AssetData : MontageAssets)
	{
		if (!LooksLikeAttackMontage(AssetData))
		{
			continue;
		}

		const UAnimMontage* Montage = Cast<UAnimMontage>(AssetData.GetAsset());
		if (!Montage)
		{
			continue;
		}

		bool bHasAttackWarning = false;
		bool bHasMotionWarping = false;
		bool bHasGameplayTagWindow = false;
		bool bHasWeaponTrail = false;
		for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			bHasAttackWarning |= NotifyEvent.Notify && NotifyEvent.Notify->IsA<UACAnimNotify_IncomingAttackWarning>();
			bHasMotionWarping |= NotifyEvent.NotifyStateClass && NotifyEvent.NotifyStateClass->IsA<UAnimNotifyState_MotionWarping>();
			bHasGameplayTagWindow |= NotifyEvent.NotifyStateClass && NotifyEvent.NotifyStateClass->IsA<UACAnimNotifyState_AddGameplayTag>();
			bHasWeaponTrail |= NotifyEvent.NotifyStateClass && NotifyEvent.NotifyStateClass->IsA<UACAnimNotifyState_PlayWeaponTrail>();
		}

		FACAssetReport& Report = Reports.AddDefaulted_GetRef();
		Report.AssetName = AssetData.AssetName.ToString();
		Report.AssetPath = AssetData.GetSoftObjectPath();

		if (!bHasAttackWarning && !IsExecutionMontage(Report.AssetName))
		{
			Report.Add(EACAssetIssueSeverity::Error, TEXT("공격 예고 노티파이가 없습니다. 플레이어가 방어 타이밍을 볼 수 없습니다."));
		}

		// ANS_ComboWindow는 검사하지 않는다. Player.Status.ComboWindow를 부여해 플레이어 입력 버퍼를 여는
		// 노티파이라, 어빌리티와 StateTree가 직접 콤보를 이어붙이는 적 몽타주에는 필요가 없다.

		if (!bHasGameplayTagWindow)
		{
			// 어빌리티 수명 전체에 걸리는 상태 태그는 ActivationOwnedTags가 담당한다.
			// 이 노티파이는 몽타주 중간 구간(Attacking.Swing 등)에만 필요해서 자동으로 필수 여부를 가릴 수 없다.
			Report.Add(EACAssetIssueSeverity::Info, TEXT("GameplayTag 구간 노티파이 없음 — 중간 구간 태그가 필요한 몽타주인지 확인하세요."));
		}

		if (!bHasWeaponTrail)
		{
			Report.Add(EACAssetIssueSeverity::Info, TEXT("WeaponTrail 없음"));
		}

		if (!Montage->HasRootMotion())
		{
			Report.Add(EACAssetIssueSeverity::Info, TEXT("RootMotion 없음"));
		}
		else if (!bHasMotionWarping)
		{
			Report.Add(EACAssetIssueSeverity::Info, TEXT("RootMotion은 있는데 MotionWarping 없음"));
		}
	}

	return Reports;
}
