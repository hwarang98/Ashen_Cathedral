#include "Validation/ACBossAssetValidator.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "Components/Combat/ACBossPhaseComponent.h"
#include "Controllers/ACStateTreeController.h"
#include "DataAssets/MetaProgression/ACDataAsset_BossReward.h"
#include "Engine/Blueprint.h"

namespace ACBossAssetValidatorInternal
{
	void ValidatePhases(const AACEnemyCharacter& BossCDO, FACAssetReport& Report)
	{
		const UACBossPhaseComponent* PhaseComponent = BossCDO.FindComponentByClass<UACBossPhaseComponent>();
		if (!PhaseComponent)
		{
			Report.Add(EACAssetIssueSeverity::Info, TEXT("BossPhaseComponent가 없는 단일 페이즈 보스입니다."));
			return;
		}

		const TArray<FACBossPhaseTransition>& Transitions = PhaseComponent->GetPhaseTransitions();
		Report.Add(
			EACAssetIssueSeverity::Info,
			FString::Printf(TEXT("Phase: %d개 전환 / 최대 %.1f초"), Transitions.Num(), PhaseComponent->GetMaxTransitionDuration()));

		for (int32 Index = 0; Index < Transitions.Num(); ++Index)
		{
			const FACBossPhaseTransition& Transition = Transitions[Index];

			TArray<FString> Problems;
			if (!Transition.TransitionSequence)
			{
				Problems.Add(TEXT("전환 컷신"));
			}
			if (!Transition.TransitionAbility && !Transition.PhaseStateTag.IsValid())
			{
				Problems.Add(TEXT("페이즈 적용 수단(Ability 또는 PhaseTag)"));
			}

			if (Problems.IsEmpty())
			{
				Report.Add(EACAssetIssueSeverity::Info, FString::Printf(TEXT("Phase %d→%d: 정상"), Index + 1, Index + 2));
			}
			else
			{
				Report.Add(
					EACAssetIssueSeverity::Warning,
					FString::Printf(TEXT("Phase %d→%d: 누락 [%s]"), Index + 1, Index + 2, *FString::Join(Problems, TEXT(", "))));
			}
		}
	}
}

TArray<FACAssetReport> ACBossAssetValidator::ValidateAll()
{
	using namespace ACBossAssetValidatorInternal;

	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(TEXT("/Game/Enemy"));
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	TArray<FAssetData> BlueprintAssets;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().GetAssets(Filter, BlueprintAssets);

	BlueprintAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.AssetName.LexicalLess(Right.AssetName);
	});

	TArray<FACAssetReport> Reports;
	for (const FAssetData& AssetData : BlueprintAssets)
	{
		// 에셋 레지스트리 태그로 먼저 걸러낸다. 이걸 안 하면 /Game/Enemy의 AnimBlueprint까지 전부 동기 로드된다
		const UClass* NativeParent = UBlueprint::GetBlueprintParentClassFromAssetTags(AssetData);
		if (!NativeParent || !NativeParent->IsChildOf(AACEnemyCharacter::StaticClass()))
		{
			continue;
		}

		const UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (!Blueprint || !Blueprint->GeneratedClass || !Blueprint->GeneratedClass->IsChildOf(AACEnemyCharacter::StaticClass()))
		{
			continue;
		}

		const AACEnemyCharacter* BossCDO = Cast<AACEnemyCharacter>(Blueprint->GeneratedClass->GetDefaultObject());
		if (!BossCDO || (!BossCDO->GetBossIdentityTag().IsValid() && !BossCDO->GetBossRewardData() && !BossCDO->FindComponentByClass<UACBossPhaseComponent>()))
		{
			continue;
		}

		FACAssetReport& Report = Reports.AddDefaulted_GetRef();
		Report.AssetName = AssetData.AssetName.ToString();
		Report.AssetPath = AssetData.GetSoftObjectPath();
		Report.Subtitle = BossCDO->GetBossIdentityTag().ToString();

		if (BossCDO->GetBossIdentityTag().IsValid())
		{
			Report.Add(EACAssetIssueSeverity::Info, TEXT("BossIdentityTag"));
		}
		else
		{
			Report.Add(EACAssetIssueSeverity::Error, TEXT("BossIdentityTag가 없습니다."));
		}

		if (BossCDO->GetCharacterStartUpData().IsNull())
		{
			Report.Add(EACAssetIssueSeverity::Error, TEXT("CharacterStartUpData가 없습니다."));
		}
		else
		{
			Report.Add(EACAssetIssueSeverity::Info, FString::Printf(TEXT("StartupData: %s"), *BossCDO->GetCharacterStartUpData().ToString()));
		}

		if (BossCDO->GetBossRewardData())
		{
			Report.Add(EACAssetIssueSeverity::Info, FString::Printf(TEXT("Reward: %s"), *BossCDO->GetBossRewardData()->GetName()));
		}
		else
		{
			Report.Add(EACAssetIssueSeverity::Warning, TEXT("BossRewardData가 없습니다."));
		}

		if (!BossCDO->AIControllerClass)
		{
			Report.Add(EACAssetIssueSeverity::Error, TEXT("AIControllerClass가 없습니다."));
		}
		else
		{
			const bool bStateTreeController = BossCDO->AIControllerClass->IsChildOf(AACStateTreeController::StaticClass());
			Report.Add(
				EACAssetIssueSeverity::Info,
				FString::Printf(TEXT("AIController: %s%s"), *BossCDO->AIControllerClass->GetName(), bStateTreeController ? TEXT("") : TEXT(" (StateTree 컨트롤러가 아님)")));
		}

		ValidatePhases(*BossCDO, Report);
	}

	return Reports;
}
