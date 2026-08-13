#include "Validation/ACRunAssetValidator.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "DataAssets/Run/ACDataAsset_RunDefinition.h"
#include "DataAssets/Run/ACDataAsset_StageDefinition.h"

namespace ACRunAssetValidatorInternal
{
	// 소프트 참조가 가리키는 레벨이 실제로 존재하는지 에셋 레지스트리로 확인한다
	bool DoesLevelExist(const IAssetRegistry& AssetRegistry, const TSoftObjectPtr<UWorld>& LevelAsset)
	{
		if (LevelAsset.IsNull())
		{
			return false;
		}

		return AssetRegistry.GetAssetByObjectPath(LevelAsset.ToSoftObjectPath()).IsValid();
	}

	void ValidateStage(const IAssetRegistry& AssetRegistry, const UACDataAsset_StageDefinition& Stage, const int32 Index, const bool bIsLastStage, TSet<FName>& InOutSeenStageIDs, FACAssetReport& Report)
	{
		const FString StageLabel = FString::Printf(TEXT("%d번 스테이지 '%s'"), Index, *Stage.StageID.ToString());

		if (Stage.StageID.IsNone())
		{
			Report.Add(EACAssetIssueSeverity::Error, FString::Printf(TEXT("%s: StageID가 비어 있습니다."), *StageLabel));
		}
		else
		{
			bool bAlreadySeen = false;
			InOutSeenStageIDs.Add(Stage.StageID, &bAlreadySeen);
			if (bAlreadySeen)
			{
				Report.Add(EACAssetIssueSeverity::Error, FString::Printf(TEXT("%s: StageID가 중복됩니다."), *StageLabel));
			}
		}

		if (Stage.LevelAsset.IsNull())
		{
			Report.Add(EACAssetIssueSeverity::Error, FString::Printf(TEXT("%s: LevelAsset이 지정되지 않았습니다."), *StageLabel));
		}
		else if (!DoesLevelExist(AssetRegistry, Stage.LevelAsset))
		{
			Report.Add(EACAssetIssueSeverity::Error, FString::Printf(TEXT("%s: LevelAsset '%s'을(를) 찾을 수 없습니다."), *StageLabel, *Stage.LevelAsset.ToString()));
		}

		if (!Stage.ExpectedBossID.IsValid())
		{
			Report.Add(EACAssetIssueSeverity::Warning, FString::Printf(TEXT("%s: ExpectedBossID가 비어 있어 배치 보스를 검증하지 않습니다."), *StageLabel));
		}
		else if (!Stage.ExpectedBossID.ToString().StartsWith(TEXT("Enemy.Boss")))
		{
			Report.Add(EACAssetIssueSeverity::Warning, FString::Printf(TEXT("%s: ExpectedBossID '%s'는 Enemy.Boss 하위 태그가 아닙니다."), *StageLabel, *Stage.ExpectedBossID.ToString()));
		}

		if (Stage.RewardMultiplier < 0.f)
		{
			Report.Add(EACAssetIssueSeverity::Error, FString::Printf(TEXT("%s: RewardMultiplier가 음수입니다."), *StageLabel));
		}

		// 이어질 스테이지가 없는데 자동 진행을 요구하면 플레이어가 아레나에서 나갈 방법이 없어진다
		if (bIsLastStage && Stage.ExitPolicy == EACStageExitPolicy::AutoNextStage)
		{
			Report.Add(EACAssetIssueSeverity::Error, FString::Printf(TEXT("%s: 마지막 스테이지에는 AutoNextStage를 쓸 수 없습니다. 런타임에는 로비 복귀로 폴백됩니다."), *StageLabel));
		}
	}
}

TArray<FACAssetReport> ACRunAssetValidator::ValidateAll()
{
	using namespace ACRunAssetValidatorInternal;

	FARFilter Filter;
	Filter.ClassPaths.Add(UACDataAsset_RunDefinition::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	const IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> RunAssets;
	AssetRegistry.GetAssets(Filter, RunAssets);

	RunAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.AssetName.LexicalLess(Right.AssetName);
	});

	TArray<FACAssetReport> Reports;
	for (const FAssetData& AssetData : RunAssets)
	{
		const UACDataAsset_RunDefinition* RunDefinition = Cast<UACDataAsset_RunDefinition>(AssetData.GetAsset());
		if (!RunDefinition)
		{
			continue;
		}

		FACAssetReport& Report = Reports.AddDefaulted_GetRef();
		Report.AssetName = AssetData.AssetName.ToString();
		Report.AssetPath = AssetData.GetSoftObjectPath();
		Report.Subtitle = RunDefinition->RunID.ToString();

		if (RunDefinition->LobbyLevel.IsNull())
		{
			Report.Add(EACAssetIssueSeverity::Error, TEXT("LobbyLevel이 지정되지 않았습니다."));
		}
		else if (!DoesLevelExist(AssetRegistry, RunDefinition->LobbyLevel))
		{
			Report.Add(EACAssetIssueSeverity::Error, FString::Printf(TEXT("LobbyLevel '%s'을(를) 찾을 수 없습니다."), *RunDefinition->LobbyLevel.ToString()));
		}

		if (RunDefinition->OrderedStages.IsEmpty())
		{
			Report.Add(EACAssetIssueSeverity::Error, TEXT("OrderedStages가 비어 있어 런을 시작할 수 없습니다."));
			continue;
		}

		TSet<FName> SeenStageIDs;
		const int32 LastIndex = RunDefinition->OrderedStages.Num() - 1;
		for (int32 Index = 0; Index <= LastIndex; ++Index)
		{
			const UACDataAsset_StageDefinition* Stage = RunDefinition->OrderedStages[Index];
			if (!Stage)
			{
				Report.Add(EACAssetIssueSeverity::Error, FString::Printf(TEXT("%d번 스테이지가 비어 있습니다. 하나라도 비어 있으면 런 시작이 전체 실패합니다."), Index));
				continue;
			}

			ValidateStage(AssetRegistry, *Stage, Index, Index == LastIndex, SeenStageIDs, Report);
		}

		if (!Report.HasErrors())
		{
			Report.Add(EACAssetIssueSeverity::Info, FString::Printf(TEXT("스테이지 %d개 배선 정상"), RunDefinition->OrderedStages.Num()));
		}
	}

	return Reports;
}
