#include "Workbench/SACBossCombatWorkbench.h"

#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_MotionWarping.h"
#include "Animation/AnimNotify/ACAnimNotify_IncomingAttackWarning.h"
#include "Animation/AnimNotify/ACAnimNotifyState_AddGameplayTag.h"
#include "Animation/AnimNotify/ACAnimNotifyState_ComboWindow.h"
#include "Animation/AnimNotify/ACAnimNotifyState_PlayWeaponTrail.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "Character/ACCharacterBase.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "Components/Combat/ACBossPhaseComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Controllers/ACStateTreeController.h"
#include "DataAssets/AI/ACDataAsset_BossTuning.h"
#include "DataAssets/MetaProgression/ACDataAsset_BossReward.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "EditorLevelUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameModes/ACBattleStartPoint.h"
#include "GameModes/ACGameMode.h"
#include "GameModes/ACStageExitPoint.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "HAL/PlatformProcess.h"
#include "LevelEditorSubsystem.h"
#include "LevelSequenceActor.h"
#include "Misc/Paths.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Engine/CurveTable.h"
#include "Validation/ACAttributeAudit.h"
#include "Validation/ACBossAssetValidator.h"
#include "Validation/ACDecayPairValidator.h"
#include "Validation/ACMontageAudit.h"
#include "Validation/ACStateTreeValidator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ACBossCombatWorkbench"

namespace ACBossWorkbench
{
	const FLinearColor GoodColor(0.28f, 0.78f, 0.40f, 1.f);
	const FLinearColor InfoColor(0.42f, 0.68f, 0.95f, 1.f);
	const FLinearColor WarningColor(1.f, 0.64f, 0.12f, 1.f);
	const FLinearColor ErrorColor(1.f, 0.25f, 0.20f, 1.f);
	const FLinearColor MutedColor(0.62f, 0.65f, 0.70f, 1.f);

	// BuildNavigation의 라벨 순서와 PageSwitcher 슬롯 순서에서 각 페이지가 놓인 자리
	constexpr int32 StateTreePageIndex = 5;
	constexpr int32 StatsPageIndex = 6;

	/** 어트리뷰트 이름에서 "ACAttributeSet." 접두사를 떼어 표에 넣기 좋게 만든다 */
	FString MakeShortAttributeName(const FGameplayAttribute& Attribute)
	{
		return Attribute.GetName();
	}

	TSharedRef<STextBlock> MakeText(const FString& Text, const FLinearColor& Color = FLinearColor::White)
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ColorAndOpacity(Color)
			.AutoWrapText(true);
	}

	void AddLine(const TSharedPtr<SVerticalBox>& Box, const FString& Text, const FLinearColor& Color = FLinearColor::White)
	{
		if (!Box.IsValid())
		{
			return;
		}

		Box->AddSlot()
		.AutoHeight()
		.Padding(2.f, 1.f)
		[
			MakeText(Text, Color)
		];
	}

	void AddSeparator(const TSharedPtr<SVerticalBox>& Box)
	{
		if (Box.IsValid())
		{
			Box->AddSlot().AutoHeight().Padding(0.f, 5.f)[SNew(SSeparator)];
		}
	}

	void AddAttributeRow(const TSharedPtr<SVerticalBox>& Box, const FString& Label, float Current, float Maximum, const FLinearColor& Color)
	{
		if (!Box.IsValid())
		{
			return;
		}

		const float Ratio = Maximum > 0.f ? FMath::Clamp(Current / Maximum, 0.f, 1.f) : 0.f;
		Box->AddSlot()
		.AutoHeight()
		.Padding(2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SBox).WidthOverride(92.f)[MakeText(Label, MutedColor)]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SProgressBar).Percent(Ratio).FillColorAndOpacity(Color)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(SBox).WidthOverride(120.f)[MakeText(FString::Printf(TEXT("%.1f / %.1f"), Current, Maximum))]
			]
		];
	}

	void OpenAssetByPath(const FSoftObjectPath AssetPath)
	{
		if (UObject* Asset = AssetPath.TryLoad())
		{
			if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
			{
				AssetEditorSubsystem->OpenEditorForAsset(Asset);
			}
		}
	}

	TArray<FAssetData> GetAssetsByClass(const FTopLevelAssetPath& ClassPath, const FName PackagePath = TEXT("/Game"))
	{
		FARFilter Filter;
		Filter.ClassPaths.Add(ClassPath);
		Filter.PackagePaths.Add(PackagePath);
		Filter.bRecursiveClasses = true;
		Filter.bRecursivePaths = true;

		TArray<FAssetData> Assets;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().GetAssets(Filter, Assets);
		Assets.Sort([](const FAssetData& Left, const FAssetData& Right)
		{
			return Left.AssetName.LexicalLess(Right.AssetName);
		});
		return Assets;
	}

	template <typename ActorType>
	TArray<ActorType*> FindActors(UWorld* World)
	{
		TArray<ActorType*> Result;
		if (!World)
		{
			return Result;
		}

		for (TActorIterator<ActorType> It(World); It; ++It)
		{
			Result.Add(*It);
		}
		return Result;
	}

	/** 보스 판정 — 식별 태그, 보상 데이터, 페이즈 컴포넌트 중 하나라도 있으면 보스로 본다 */
	TArray<AACEnemyCharacter*> FindAllBosses(UWorld* World)
	{
		TArray<AACEnemyCharacter*> Bosses;
		for (AACEnemyCharacter* Enemy : FindActors<AACEnemyCharacter>(World))
		{
			if (Enemy && (Enemy->GetBossIdentityTag().IsValid() || Enemy->GetBossRewardData() || Enemy->FindComponentByClass<UACBossPhaseComponent>()))
			{
				Bosses.Add(Enemy);
			}
		}

		// TActorIterator 순서는 스폰 순서라 실행마다 달라질 수 있다. 이름으로 고정해 같은 대상을 계속 보게 한다
		Bosses.Sort([](const AACEnemyCharacter& Left, const AACEnemyCharacter& Right)
		{
			return Left.GetName() < Right.GetName();
		});
		return Bosses;
	}

	AACEnemyCharacter* FindBoss(UWorld* World)
	{
		const TArray<AACEnemyCharacter*> Bosses = FindAllBosses(World);
		return Bosses.IsEmpty() ? nullptr : Bosses[0];
	}

	FLinearColor SeverityColor(const EACAssetIssueSeverity Severity)
	{
		switch (Severity)
		{
			case EACAssetIssueSeverity::Error:
				return ErrorColor;

			case EACAssetIssueSeverity::Warning:
				return WarningColor;

			default:
				return MutedColor;
		}
	}

	const TCHAR* SeverityPrefix(const EACAssetIssueSeverity Severity)
	{
		switch (Severity)
		{
			case EACAssetIssueSeverity::Error:
				return TEXT("오류:");

			case EACAssetIssueSeverity::Warning:
				return TEXT("주의:");

			default:
				return TEXT("✓");
		}
	}

	/** 조작 결과를 에디터 알림으로 띄운다. 조건이 안 맞아 아무 일도 안 일어났을 때 침묵하지 않기 위한 것 */
	void Notify(const FString& Message, const bool bSuccess)
	{
		FNotificationInfo Info(FText::FromString(Message));
		Info.ExpireDuration = 3.f;

		const TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		}
	}
}

void SACBossCombatWorkbench::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(6.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				BuildNavigation()
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(PageSwitcher, SWidgetSwitcher)
				+ SWidgetSwitcher::Slot()[BuildTestAndLivePage()]
				+ SWidgetSwitcher::Slot()[BuildTuningPage()]
				+ SWidgetSwitcher::Slot()[BuildValidationPage()]
				+ SWidgetSwitcher::Slot()[BuildArenaPage()]
				+ SWidgetSwitcher::Slot()[BuildMontagePage()]
				+ SWidgetSwitcher::Slot()[BuildStateTreePage()]
				+ SWidgetSwitcher::Slot()[BuildStatsPage()]
			]
		]
	];

	RefreshTuningComparison();
	RefreshBossValidation();
	RefreshArenaValidation();
	RefreshMontageAudit();
	RefreshLiveMonitor();
}

void SACBossCombatWorkbench::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (InCurrentTime - LastLiveRefreshSeconds >= 0.25)
	{
		LastLiveRefreshSeconds = InCurrentTime;
		RefreshLiveMonitor();
	}

	// 매트릭스 재구성은 편집 콜백 안에서 위젯을 파괴하지 않도록 다음 틱으로 미룬다
	if (bAttributeMatrixPending && PageSwitcher.IsValid() && PageSwitcher->GetActiveWidgetIndex() == ACBossWorkbench::StatsPageIndex)
	{
		bAttributeMatrixPending = false;
		RefreshAttributeMatrix();
	}
}

TSharedRef<SWidget> SACBossCombatWorkbench::BuildNavigation()
{
	const TArray<FText> Labels = {
		LOCTEXT("TestTab", "테스트 / 라이브"),
		LOCTEXT("TuningTab", "튜닝 비교"),
		LOCTEXT("ValidationTab", "보스 / 페이즈 검증"),
		LOCTEXT("ArenaTab", "아레나"),
		LOCTEXT("MontageTab", "몽타주 감사"),
		LOCTEXT("StateTreeTab", "StateTree 검증"),
		LOCTEXT("StatsTab", "스탯")
	};

	TSharedRef<SHorizontalBox> Navigation = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < Labels.Num(); ++Index)
	{
		Navigation->AddSlot()
		.AutoWidth()
		.Padding(2.f)
		[
			SNew(SButton)
			.Text(Labels[Index])
			.OnClicked_Lambda([this, Index]
			{
				SetActivePage(Index);
				return FReply::Handled();
			})
		];
	}
	return Navigation;
}

TSharedRef<SWidget> SACBossCombatWorkbench::MakePageHeader(const FText& Title, const FText& Description) const
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 8.f)
		[
			SNew(STextBlock).Text(Description).ColorAndOpacity(ACBossWorkbench::MutedColor).AutoWrapText(true)
		];
}

TSharedRef<SWidget> SACBossCombatWorkbench::BuildTestAndLivePage()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[MakePageHeader(
				LOCTEXT("TestTitle", "Boss Test Launcher & GAS Live Monitor"),
				LOCTEXT("TestDesc", "맵을 열고 PIE를 시작한 뒤 Player/Boss GAS, StateTree, Phase를 실시간으로 확인합니다."))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UWorld::StaticClass())
					.ObjectPath_Lambda([this] { return SelectedMapPath.ToString(); })
					.OnObjectChanged(this, &SACBossCombatWorkbench::HandleMapAssetChanged)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
				[
					SNew(SButton).Text(LOCTEXT("LoadMap", "맵 열기")).OnClicked(this, &SACBossCombatWorkbench::HandleLoadSelectedMap)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("StartPIE", "PIE 시작")).OnClicked(this, &SACBossCombatWorkbench::HandleStartPIE)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("StopPIE", "PIE 종료")).OnClicked(this, &SACBossCombatWorkbench::HandleStopPIE)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("RestartStage", "스테이지 재시작")).OnClicked(this, &SACBossCombatWorkbench::HandleRestartStage)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("BossHP100", "Boss HP 100%")).OnClicked(this, &SACBossCombatWorkbench::HandleSetBossHealth, 1.f)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("BossHP50", "Boss HP 50%")).OnClicked(this, &SACBossCombatWorkbench::HandleSetBossHealth, 0.5f)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("BossHP1", "Boss HP 1")).OnClicked(this, &SACBossCombatWorkbench::HandleSetBossHealth, 0.0001f)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("ForcePhase", "다음 페이즈 강제")).OnClicked(this, &SACBossCombatWorkbench::HandleForceNextPhase)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 8.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("WebOn", "WebDebug 시작")).OnClicked(this, &SACBossCombatWorkbench::HandleWebDebug, true)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("WebOff", "WebDebug 종료")).OnClicked(this, &SACBossCombatWorkbench::HandleWebDebug, false)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("WebOpen", "대시보드 열기")).OnClicked(this, &SACBossCombatWorkbench::HandleOpenWebDashboard)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("LogsOpen", "RunLogs 열기")).OnClicked(this, &SACBossCombatWorkbench::HandleOpenRunLogs)]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SAssignNew(LiveMonitorBox, SVerticalBox)
			]
		];
}

TSharedRef<SWidget> SACBossCombatWorkbench::BuildTuningPage()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[MakePageHeader(
				LOCTEXT("TuningTitle", "전체 보스 거리 튜닝 비교"),
				LOCTEXT("TuningDesc", "모든 BossTuning DataAsset을 비교하고 직접 공격 거리 공백을 표시합니다."))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SButton).Text(LOCTEXT("RefreshTuning", "새로고침")).OnClicked_Lambda([this]
				{
					RefreshTuningComparison();
					return FReply::Handled();
				})
			]
			+ SVerticalBox::Slot().AutoHeight()[SAssignNew(TuningComparisonBox, SVerticalBox)]
		];
}

TSharedRef<SWidget> SACBossCombatWorkbench::BuildValidationPage()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[MakePageHeader(
				LOCTEXT("ValidationTitle", "Boss Asset & Phase/Cutscene Validator"),
				LOCTEXT("ValidationDesc", "Enemy Blueprint의 StartupData, Reward, AIController, Phase Sequence/Ability/Tag를 검사합니다. 아래에서 Posture/Guard 감소·유예 GE 짝도 함께 검사합니다(플레이어 포함)."))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SButton).Text(LOCTEXT("RefreshValidation", "전체 보스 검사")).OnClicked_Lambda([this]
				{
					RefreshBossValidation();
					return FReply::Handled();
				})
			]
			+ SVerticalBox::Slot().AutoHeight()[SAssignNew(BossValidationBox, SVerticalBox)]
		];
}

TSharedRef<SWidget> SACBossCombatWorkbench::BuildArenaPage()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[MakePageHeader(
				LOCTEXT("ArenaTitle", "Arena Setup & Validator"),
				LOCTEXT("ArenaDesc", "현재 레벨의 전투 기준 액터, GameMode, NavMesh, 시작 거리를 검사하고 누락된 기준 액터를 배치합니다."))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("RefreshArena", "현재 레벨 검사")).OnClicked_Lambda([this]
				{
					RefreshArenaValidation();
					return FReply::Handled();
				})]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(LOCTEXT("PlaceArena", "누락 기준 액터 배치")).OnClicked(this, &SACBossCombatWorkbench::HandlePlaceArenaTemplate)]
			]
			+ SVerticalBox::Slot().AutoHeight()[SAssignNew(ArenaValidationBox, SVerticalBox)]
		];
}

TSharedRef<SWidget> SACBossCombatWorkbench::BuildMontagePage()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[MakePageHeader(
				LOCTEXT("MontageTitle", "Attack Montage Auditor"),
				LOCTEXT("MontageDesc", "Enemy 공격 몽타주의 Root Motion, IncomingAttackWarning, MotionWarping, ComboWindow, GameplayTag Notify를 검사합니다."))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SButton).Text(LOCTEXT("RefreshMontage", "공격 몽타주 검사")).OnClicked_Lambda([this]
				{
					RefreshMontageAudit();
					return FReply::Handled();
				})
			]
			+ SVerticalBox::Slot().AutoHeight()[SAssignNew(MontageAuditBox, SVerticalBox)]
		];
}

TSharedRef<SWidget> SACBossCombatWorkbench::BuildStateTreePage()
{
	TSharedRef<SWidget> Page = SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[MakePageHeader(
				LOCTEXT("StateTreeTitle", "StateTree Wiring Validator"),
				LOCTEXT("StateTreeDesc", "자식 선택 규칙, 어빌리티 태그 배선, 쿨다운 태그 오타처럼 런타임에 조용히 실패하는 항목을 정적으로 검사합니다."))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SButton).Text(LOCTEXT("RefreshStateTree", "다시 검사")).OnClicked_Lambda([this]
				{
					RefreshStateTreeValidation();
					return FReply::Handled();
				})
			]
			+ SVerticalBox::Slot().AutoHeight()[SAssignNew(StateTreeValidationBox, SVerticalBox)]
		];

	ACBossWorkbench::AddLine(StateTreeValidationBox, TEXT("이 페이지에 처음 들어올 때 검사합니다."), ACBossWorkbench::MutedColor);

	return Page;
}

TSharedRef<SWidget> SACBossCombatWorkbench::BuildStatsPage()
{
	MetaInjectAmounts.Init(10.f, ACAttributeAudit::GetMetaAttributes().Num());

	TSharedRef<SWidget> Page = SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[MakePageHeader(
				LOCTEXT("StatsTitle", "Attribute Matrix & Live Editor"),
				LOCTEXT("StatsDesc", "캐릭터별 어트리뷰트 초기값과 그 출처를 비교하고, CurveTable 값을 여기서 바로 고칩니다. PIE 중에는 아래에서 실시간 값도 조정할 수 있습니다."))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SButton).Text(LOCTEXT("RefreshStats", "다시 수집")).OnClicked_Lambda([this]
				{
					RefreshAttributeMatrix();
					return FReply::Handled();
				})
			]
			+ SVerticalBox::Slot().AutoHeight()[SAssignNew(AttributeMatrixBox, SVerticalBox)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)[SNew(SSeparator)]
			+ SVerticalBox::Slot().AutoHeight()[BuildLiveAttributeEditor()]
		];

	ACBossWorkbench::AddLine(AttributeMatrixBox, TEXT("이 페이지에 처음 들어올 때 수집합니다."), ACBossWorkbench::MutedColor);

	return Page;
}

TSharedRef<SWidget> SACBossCombatWorkbench::BuildLiveAttributeEditor()
{
	using namespace ACBossWorkbench;

	TSharedRef<SVerticalBox> Section = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
		[
			SNew(STextBlock).Text(LOCTEXT("LiveEditTitle", "라이브 편집 (PIE 전용)")).Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(SCheckBox)
				.Style(FAppStyle::Get(), TEXT("RadioButton"))
				.IsChecked_Lambda([this] { return bLiveEditTargetIsBoss ? ECheckBoxState::Unchecked : ECheckBoxState::Checked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState) { bLiveEditTargetIsBoss = false; })
				[
					MakeText(TEXT("플레이어"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SCheckBox)
				.Style(FAppStyle::Get(), TEXT("RadioButton"))
				.IsChecked_Lambda([this] { return bLiveEditTargetIsBoss ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState) { bLiveEditTargetIsBoss = true; })
				[
					MakeText(TEXT("보스"))
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(STextBlock)
			.Text_Lambda([this]
			{
				return GetLiveEditTargetASC()
					? LOCTEXT("LiveEditReady", "대상 연결됨")
					: LOCTEXT("LiveEditNoTarget", "PIE가 실행 중이 아니거나 대상을 찾지 못했습니다.");
			})
			.ColorAndOpacity_Lambda([this]
			{
				return FSlateColor(GetLiveEditTargetASC() ? GoodColor : MutedColor);
			})
		];

	Section->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)[MakeText(TEXT("현재값 — BaseValue를 직접 덮어씁니다"), MutedColor)];
	for (const FGameplayAttribute& Attribute : ACAttributeAudit::GetRuntimeGaugeAttributes())
	{
		Section->AddSlot().AutoHeight()[MakeLiveAttributeRow(Attribute)];
	}

	Section->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 2.f)[MakeText(TEXT("스탯 — BaseValue를 직접 덮어씁니다"), MutedColor)];
	for (const FGameplayAttribute& Attribute : ACAttributeAudit::GetTrackedAttributes())
	{
		Section->AddSlot().AutoHeight()[MakeLiveAttributeRow(Attribute)];
	}

	Section->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 2.f)
	[
		MakeText(TEXT("피해 주입 — 즉시형 GE로 적용되어 체간 붕괴·가드 브레이크·사망 판정까지 정상적으로 돕니다"), MutedColor)
	];

	const TArray<FGameplayAttribute>& MetaAttributes = ACAttributeAudit::GetMetaAttributes();
	for (int32 Index = 0; Index < MetaAttributes.Num(); ++Index)
	{
		Section->AddSlot().AutoHeight()[MakeMetaInjectRow(MetaAttributes[Index], Index)];
	}

	return Section;
}

TSharedRef<SWidget> SACBossCombatWorkbench::MakeLiveAttributeRow(const FGameplayAttribute& Attribute)
{
	using namespace ACBossWorkbench;

	const FGameplayAttribute CapturedAttribute = Attribute;
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f)
		[
			SNew(SBox).WidthOverride(180.f)[MakeText(MakeShortAttributeName(CapturedAttribute), MutedColor)]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f)
		[
			SNew(SBox)
			.WidthOverride(120.f)
			[
				SNew(SNumericEntryBox<float>)
				.AllowSpin(false)
				.IsEnabled_Lambda([this] { return GetLiveEditTargetASC() != nullptr; })
				.Value_Lambda([this, CapturedAttribute]() -> TOptional<float>
				{
					if (const UAbilitySystemComponent* ASC = GetLiveEditTargetASC())
					{
						return ASC->GetNumericAttribute(CapturedAttribute);
					}
					return TOptional<float>();
				})
				.OnValueCommitted_Lambda([this, CapturedAttribute](float NewValue, ETextCommit::Type)
				{
					SetLiveAttributeValue(CapturedAttribute, NewValue);
				})
			]
		];
}

TSharedRef<SWidget> SACBossCombatWorkbench::MakeMetaInjectRow(const FGameplayAttribute& Attribute, const int32 MetaIndex)
{
	using namespace ACBossWorkbench;

	const FGameplayAttribute CapturedAttribute = Attribute;
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f)
		[
			SNew(SBox).WidthOverride(180.f)[MakeText(MakeShortAttributeName(CapturedAttribute), MutedColor)]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f)
		[
			SNew(SBox)
			.WidthOverride(120.f)
			[
				SNew(SNumericEntryBox<float>)
				.AllowSpin(false)
				.Value_Lambda([this, MetaIndex]() -> TOptional<float>
				{
					return MetaInjectAmounts.IsValidIndex(MetaIndex) ? MetaInjectAmounts[MetaIndex] : TOptional<float>();
				})
				.OnValueCommitted_Lambda([this, MetaIndex](float NewValue, ETextCommit::Type)
				{
					if (MetaInjectAmounts.IsValidIndex(MetaIndex))
					{
						MetaInjectAmounts[MetaIndex] = NewValue;
					}
				})
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 2.f)
		[
			SNew(SButton)
			.Text(LOCTEXT("InjectMeta", "주입"))
			.IsEnabled_Lambda([this] { return GetLiveEditTargetASC() != nullptr; })
			.OnClicked_Lambda([this, CapturedAttribute, MetaIndex]
			{
				if (MetaInjectAmounts.IsValidIndex(MetaIndex))
				{
					InjectMetaAttribute(CapturedAttribute, MetaInjectAmounts[MetaIndex]);
				}
				return FReply::Handled();
			})
		];
}

void SACBossCombatWorkbench::SetActivePage(int32 PageIndex)
{
	if (PageSwitcher.IsValid())
	{
		PageSwitcher->SetActiveWidgetIndex(PageIndex);
	}

	if (PageIndex == ACBossWorkbench::StateTreePageIndex && bStateTreeValidationPending)
	{
		bStateTreeValidationPending = false;
		RefreshStateTreeValidation();
	}
}

UWorld* SACBossCombatWorkbench::GetEditorWorld() const
{
	return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
}

UWorld* SACBossCombatWorkbench::GetPIEWorld() const
{
	return GEditor ? GEditor->PlayWorld : nullptr;
}

void SACBossCombatWorkbench::HandleMapAssetChanged(const FAssetData& AssetData)
{
	SelectedMapPath = AssetData.GetSoftObjectPath();
}

FReply SACBossCombatWorkbench::HandleLoadSelectedMap()
{
	if (!SelectedMapPath.IsValid())
	{
		ACBossWorkbench::Notify(TEXT("맵을 먼저 선택하세요."), false);
		return FReply::Handled();
	}

	if (ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>())
	{
		LevelEditorSubsystem->LoadLevel(SelectedMapPath.GetLongPackageName());
		RefreshArenaValidation();
	}
	return FReply::Handled();
}

FReply SACBossCombatWorkbench::HandleStartPIE()
{
	if (ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>())
	{
		if (LevelEditorSubsystem->IsInPlayInEditor())
		{
			ACBossWorkbench::Notify(TEXT("이미 PIE가 실행 중입니다."), false);
			return FReply::Handled();
		}

		LevelEditorSubsystem->EditorRequestBeginPlay();
	}
	return FReply::Handled();
}

FReply SACBossCombatWorkbench::HandleStopPIE()
{
	if (ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>())
	{
		if (!LevelEditorSubsystem->IsInPlayInEditor())
		{
			ACBossWorkbench::Notify(TEXT("PIE가 실행 중이 아닙니다."), false);
			return FReply::Handled();
		}

		LevelEditorSubsystem->EditorRequestEndPlay();
	}
	return FReply::Handled();
}

FReply SACBossCombatWorkbench::HandleRestartStage()
{
	UWorld* World = GetPIEWorld();
	if (!World)
	{
		ACBossWorkbench::Notify(TEXT("PIE가 실행 중이 아닙니다."), false);
		return FReply::Handled();
	}

	AACGameMode* GameMode = Cast<AACGameMode>(World->GetAuthGameMode());
	if (!GameMode)
	{
		ACBossWorkbench::Notify(TEXT("현재 맵의 GameMode가 ACGameMode가 아닙니다."), false);
		return FReply::Handled();
	}

	GameMode->DebugRestartCurrentStage();
	ACBossWorkbench::Notify(TEXT("스테이지를 재시작했습니다."), true);
	return FReply::Handled();
}

FReply SACBossCombatWorkbench::HandleSetBossHealth(float HealthRatio)
{
	AACEnemyCharacter* Boss = ACBossWorkbench::FindBoss(GetPIEWorld());
	if (!Boss)
	{
		ACBossWorkbench::Notify(GetPIEWorld() ? TEXT("보스를 찾지 못했습니다.") : TEXT("PIE가 실행 중이 아닙니다."), false);
		return FReply::Handled();
	}

	UACAbilitySystemComponent* ASC = Boss->GetACAbilitySystemComponent();
	if (!ASC)
	{
		ACBossWorkbench::Notify(TEXT("보스의 AbilitySystemComponent를 찾지 못했습니다."), false);
		return FReply::Handled();
	}

	const float MaxHealth = ASC->GetNumericAttribute(UACAttributeSet::GetMaxHealthAttribute());
	const float NewHealth = HealthRatio <= 0.001f ? 1.f : MaxHealth * FMath::Clamp(HealthRatio, 0.f, 1.f);
	ASC->SetNumericAttributeBase(UACAttributeSet::GetHealthAttribute(), NewHealth);
	return FReply::Handled();
}

FReply SACBossCombatWorkbench::HandleForceNextPhase()
{
#if !UE_BUILD_SHIPPING
	AACEnemyCharacter* Boss = ACBossWorkbench::FindBoss(GetPIEWorld());
	if (!Boss)
	{
		ACBossWorkbench::Notify(GetPIEWorld() ? TEXT("보스를 찾지 못했습니다.") : TEXT("PIE가 실행 중이 아닙니다."), false);
		return FReply::Handled();
	}

	UACBossPhaseComponent* PhaseComponent = Boss->FindComponentByClass<UACBossPhaseComponent>();
	if (!PhaseComponent)
	{
		ACBossWorkbench::Notify(TEXT("이 보스에는 BossPhaseComponent가 없습니다."), false);
		return FReply::Handled();
	}

	PhaseComponent->DebugForceNextPhase();
#endif
	return FReply::Handled();
}

FReply SACBossCombatWorkbench::HandleWebDebug(bool bEnable)
{
	UWorld* World = GetPIEWorld();
	if (!World)
	{
		ACBossWorkbench::Notify(TEXT("PIE가 실행 중이 아닙니다."), false);
		return FReply::Handled();
	}

	GEngine->Exec(World, bEnable ? TEXT("ac.WebDebug 1") : TEXT("ac.WebDebug 0"));
	ACBossWorkbench::Notify(bEnable ? TEXT("WebDebug를 시작했습니다.") : TEXT("WebDebug를 종료했습니다."), true);
	return FReply::Handled();
}

FReply SACBossCombatWorkbench::HandleOpenWebDashboard()
{
	FPlatformProcess::LaunchURL(TEXT("http://127.0.0.1:8091"), nullptr, nullptr);
	return FReply::Handled();
}

FReply SACBossCombatWorkbench::HandleOpenRunLogs()
{
	const FString RunLogsDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RunLogs"));
	FPlatformProcess::ExploreFolder(*RunLogsDirectory);
	return FReply::Handled();
}

void SACBossCombatWorkbench::RefreshLiveMonitor()
{
	using namespace ACBossWorkbench;

	if (!LiveMonitorBox.IsValid())
	{
		return;
	}

	LiveMonitorBox->ClearChildren();
	UWorld* World = GetPIEWorld();
	if (!World)
	{
		AddLine(LiveMonitorBox, TEXT("PIE가 실행 중이 아닙니다. 맵을 선택하고 PIE 시작을 누르세요."), InfoColor);
		return;
	}

	AACCharacterBase* Player = nullptr;
	if (APlayerController* PlayerController = World->GetFirstPlayerController())
	{
		Player = Cast<AACCharacterBase>(PlayerController->GetPawn());
	}
	const TArray<AACEnemyCharacter*> Bosses = FindAllBosses(World);
	AACEnemyCharacter* Boss = Bosses.IsEmpty() ? nullptr : Bosses[0];

	AddLine(LiveMonitorBox, FString::Printf(TEXT("PIE World: %s"), *World->GetMapName()), InfoColor);
	if (Bosses.Num() > 1)
	{
		AddLine(LiveMonitorBox, FString::Printf(TEXT("주의: 보스가 %d체입니다. 이름순 첫 번째(%s)만 표시·조작합니다."), Bosses.Num(), *Boss->GetName()), WarningColor);
	}
	if (Player)
	{
		AddSeparator(LiveMonitorBox);
		AddLine(LiveMonitorBox, FString::Printf(TEXT("PLAYER — %s"), *Player->GetName()), GoodColor);
		if (const UACAttributeSet* Attributes = Player->GetACAttributeSet())
		{
			AddAttributeRow(LiveMonitorBox, TEXT("Health"), Attributes->GetHealth(), Attributes->GetMaxHealth(), FLinearColor(0.90f, 0.30f, 0.30f));
			AddAttributeRow(LiveMonitorBox, TEXT("Stamina"), Attributes->GetStamina(), Attributes->GetMaxStamina(), FLinearColor(0.20f, 0.75f, 0.42f));
			AddAttributeRow(LiveMonitorBox, TEXT("Posture"), Attributes->GetPosture(), Attributes->GetMaxPosture(), FLinearColor(0.95f, 0.64f, 0.12f));
			AddAttributeRow(LiveMonitorBox, TEXT("Guard"), Attributes->GetGuardGauge(), Attributes->GetMaxGuardGauge(), FLinearColor(0.25f, 0.52f, 0.95f));
			AddAttributeRow(LiveMonitorBox, TEXT("Burn"), Attributes->GetBurnGauge(), Attributes->GetMaxBurnGauge(), FLinearColor(0.95f, 0.35f, 0.12f));
		}
	}
	else
	{
		AddLine(LiveMonitorBox, TEXT("플레이어를 찾지 못했습니다."), WarningColor);
	}

	if (Boss)
	{
		AddSeparator(LiveMonitorBox);
		AddLine(LiveMonitorBox, FString::Printf(TEXT("BOSS — %s  [%s]"), *Boss->GetName(), *Boss->GetBossIdentityTag().ToString()), GoodColor);
		if (const UACAttributeSet* Attributes = Boss->GetACAttributeSet())
		{
			AddAttributeRow(LiveMonitorBox, TEXT("Health"), Attributes->GetHealth(), Attributes->GetMaxHealth(), FLinearColor(0.90f, 0.30f, 0.30f));
			AddAttributeRow(LiveMonitorBox, TEXT("Posture"), Attributes->GetPosture(), Attributes->GetMaxPosture(), FLinearColor(0.95f, 0.64f, 0.12f));
			AddAttributeRow(LiveMonitorBox, TEXT("Guard"), Attributes->GetGuardGauge(), Attributes->GetMaxGuardGauge(), FLinearColor(0.25f, 0.52f, 0.95f));
			AddAttributeRow(LiveMonitorBox, TEXT("Burn"), Attributes->GetBurnGauge(), Attributes->GetMaxBurnGauge(), FLinearColor(0.95f, 0.35f, 0.12f));
		}

		if (Player)
		{
			AddLine(LiveMonitorBox, FString::Printf(TEXT("거리: %.1f cm"), FVector::Dist(Player->GetActorLocation(), Boss->GetActorLocation())), InfoColor);
		}

		if (const UACBossPhaseComponent* PhaseComponent = Boss->FindComponentByClass<UACBossPhaseComponent>())
		{
			AddLine(
				LiveMonitorBox,
				FString::Printf(
					TEXT("페이즈: %d / %d%s"),
					PhaseComponent->GetCurrentPhase(),
					PhaseComponent->GetMaxPhase(),
					PhaseComponent->IsPhaseTransitionInProgress() ? TEXT("  (전환 중)") : TEXT("")),
				PhaseComponent->IsPhaseTransitionInProgress() ? WarningColor : InfoColor);
		}

		if (AAIController* AIController = Cast<AAIController>(Boss->GetController()))
		{
			if (const UStateTreeAIComponent* StateTreeComponent = AIController->FindComponentByClass<UStateTreeAIComponent>())
			{
				FString StateText = TEXT("StateTree");
#if WITH_GAMEPLAY_DEBUGGER
				const TArray<FName> ActiveStates = StateTreeComponent->GetActiveStateNames();
				if (!ActiveStates.IsEmpty())
				{
					TArray<FString> StateNames;
					for (const FName StateName : ActiveStates)
					{
						StateNames.Add(StateName.ToString());
					}
					StateText = FString::Join(StateNames, TEXT(" > "));
				}
#endif
				AddLine(LiveMonitorBox, FString::Printf(TEXT("StateTree: %s"), *StateText), InfoColor);
			}
		}

		if (const UACAbilitySystemComponent* ASC = Boss->GetACAbilitySystemComponent())
		{
			FGameplayTagContainer OwnedTags;
			ASC->GetOwnedGameplayTags(OwnedTags);
			AddLine(LiveMonitorBox, FString::Printf(TEXT("활성 태그: %s"), *OwnedTags.ToStringSimple()), MutedColor);

			TArray<FString> ActiveAbilityNames;
			for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
			{
				if (Spec.IsActive() && Spec.Ability)
				{
					ActiveAbilityNames.Add(Spec.Ability->GetClass()->GetName());
				}
			}
			AddLine(
				LiveMonitorBox,
				FString::Printf(TEXT("활성 Ability: %s"), ActiveAbilityNames.IsEmpty() ? TEXT("없음") : *FString::Join(ActiveAbilityNames, TEXT(", "))),
				MutedColor);
		}
	}
	else
	{
		AddLine(LiveMonitorBox, TEXT("보스를 찾지 못했습니다. BossIdentityTag, BossRewardData 또는 BossPhaseComponent를 확인하세요."), WarningColor);
	}
}

void SACBossCombatWorkbench::RefreshTuningComparison()
{
	using namespace ACBossWorkbench;
	if (!TuningComparisonBox.IsValid())
	{
		return;
	}

	TuningComparisonBox->ClearChildren();
	const TArray<FAssetData> Assets = GetAssetsByClass(UACDataAsset_BossTuning::StaticClass()->GetClassPathName());
	AddLine(TuningComparisonBox, FString::Printf(TEXT("Boss Tuning DataAsset: %d개"), Assets.Num()), InfoColor);

	TuningComparisonBox->AddSlot().AutoHeight().Padding(0.f, 5.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(2.0f)[MakeText(TEXT("Asset"), MutedColor)]
		+ SHorizontalBox::Slot().FillWidth(1.0f)[MakeText(TEXT("Melee"), MutedColor)]
		+ SHorizontalBox::Slot().FillWidth(1.8f)[MakeText(TEXT("Run Attack"), MutedColor)]
		+ SHorizontalBox::Slot().FillWidth(1.0f)[MakeText(TEXT("Strafe"), MutedColor)]
		+ SHorizontalBox::Slot().FillWidth(1.0f)[MakeText(TEXT("Chase"), MutedColor)]
		+ SHorizontalBox::Slot().FillWidth(2.2f)[MakeText(TEXT("직접 공격 공백"), MutedColor)]
		+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(48.f)]
	];

	for (const FAssetData& AssetData : Assets)
	{
		const UACDataAsset_BossTuning* Data = Cast<UACDataAsset_BossTuning>(AssetData.GetAsset());
		if (!Data)
		{
			continue;
		}

		TArray<FString> Gaps;
		if (Data->MeleeDistance < Data->RunAttackMinDistance)
		{
			Gaps.Add(FString::Printf(TEXT("%.0f~%.0f"), Data->MeleeDistance, Data->RunAttackMinDistance));
		}
		if (Data->RunAttackMaxDistance < Data->ChaseDistance)
		{
			Gaps.Add(FString::Printf(TEXT("%.0f~%.0f"), Data->RunAttackMaxDistance, Data->ChaseDistance));
		}
		const bool bInvalid = Data->RunAttackMinDistance > Data->RunAttackMaxDistance || Data->BackDashAttackMinDistance > Data->BackDashAttackMaxDistance;
		const FLinearColor StatusColor = bInvalid ? ErrorColor : (Gaps.IsEmpty() ? GoodColor : WarningColor);
		const FSoftObjectPath AssetPath = AssetData.GetSoftObjectPath();

		TuningComparisonBox->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(2.0f)[MakeText(AssetData.AssetName.ToString(), StatusColor)]
			+ SHorizontalBox::Slot().FillWidth(1.0f)[MakeText(FString::Printf(TEXT("0~%.0f"), Data->MeleeDistance))]
			+ SHorizontalBox::Slot().FillWidth(1.8f)[MakeText(FString::Printf(TEXT("%.0f~%.0f"), Data->RunAttackMinDistance, Data->RunAttackMaxDistance))]
			+ SHorizontalBox::Slot().FillWidth(1.0f)[MakeText(FString::Printf(TEXT("%.0f"), Data->StrafeDistance))]
			+ SHorizontalBox::Slot().FillWidth(1.0f)[MakeText(FString::Printf(TEXT("%.0f"), Data->ChaseDistance))]
			+ SHorizontalBox::Slot().FillWidth(2.2f)[MakeText(bInvalid ? TEXT("Min/Max 오류") : (Gaps.IsEmpty() ? TEXT("없음") : FString::Join(Gaps, TEXT(", "))), StatusColor)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).Text(LOCTEXT("OpenAsset", "열기")).OnClicked_Lambda([AssetPath]
				{
					OpenAssetByPath(AssetPath);
					return FReply::Handled();
				})
			]
		];
	}
}

void SACBossCombatWorkbench::RefreshBossValidation()
{
	using namespace ACBossWorkbench;
	if (!BossValidationBox.IsValid())
	{
		return;
	}

	BossValidationBox->ClearChildren();

	const TArray<FACAssetReport> BossReports = ACBossAssetValidator::ValidateAll();
	int32 BossProblemCount = 0;

	for (const FACAssetReport& Report : BossReports)
	{
		BossProblemCount += Report.CountBySeverity(EACAssetIssueSeverity::Error) + Report.CountBySeverity(EACAssetIssueSeverity::Warning);

		const FSoftObjectPath AssetPath = Report.AssetPath;
		BossValidationBox->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				MakeText(FString::Printf(TEXT("%s  [%s]"), *Report.AssetName, *Report.Subtitle), InfoColor)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).Text(LOCTEXT("OpenBossBP", "BP 열기")).OnClicked_Lambda([AssetPath]
				{
					OpenAssetByPath(AssetPath);
					return FReply::Handled();
				})
			]
		];

		for (const FACAssetIssue& Issue : Report.Issues)
		{
			AddLine(
				BossValidationBox,
				FString::Printf(TEXT("  %s %s"), SeverityPrefix(Issue.Severity), *Issue.Message),
				SeverityColor(Issue.Severity));
		}
	}

	AddSeparator(BossValidationBox);
	AddLine(BossValidationBox, TEXT("Posture / Guard 자연 감소 · 유예 GE 짝"), InfoColor);

	const TArray<FACDecayIssue> DecayIssues = ACDecayPairValidator::ValidateAll();
	if (DecayIssues.IsEmpty())
	{
		AddLine(BossValidationBox, TEXT("  ✓ 문제 없음"), GoodColor);
	}
	else
	{
		for (const FACDecayIssue& Issue : DecayIssues)
		{
			const bool bIsError = Issue.Severity == EACDecayIssueSeverity::Error;
			const FSoftObjectPath AssetPath = Issue.AssetPath;

			BossValidationBox->AddSlot()
			.AutoHeight()
			.Padding(2.f, 1.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					MakeText(
						FString::Printf(TEXT("  [%s] %s · %s — %s"), bIsError ? TEXT("오류") : TEXT("주의"), *Issue.CharacterName, *Issue.GaugeName, *Issue.Message),
						bIsError ? ErrorColor : WarningColor)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(LOCTEXT("OpenDecayAsset", "열기")).OnClicked_Lambda([AssetPath]
					{
						OpenAssetByPath(AssetPath);
						return FReply::Handled();
					})
				]
			];
		}
	}

	BossValidationBox->InsertSlot(0)
	.AutoHeight()
	.Padding(0.f, 0.f, 0.f, 4.f)
	[
		MakeText(
			FString::Printf(TEXT("보스 %d개 검사 완료 — 확인 항목 %d개 · Decay 짝 문제 %d개"), BossReports.Num(), BossProblemCount, DecayIssues.Num()),
			(BossProblemCount > 0 || !DecayIssues.IsEmpty()) ? WarningColor : GoodColor)
	];
}

void SACBossCombatWorkbench::RefreshArenaValidation()
{
	using namespace ACBossWorkbench;
	if (!ArenaValidationBox.IsValid())
	{
		return;
	}

	ArenaValidationBox->ClearChildren();
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		AddLine(ArenaValidationBox, TEXT("현재 Editor World를 찾지 못했습니다."), ErrorColor);
		return;
	}

	AddLine(ArenaValidationBox, FString::Printf(TEXT("현재 레벨: %s"), *World->GetMapName()), InfoColor);

	const TArray<APlayerStart*> PlayerStarts = FindActors<APlayerStart>(World);

	// 보스는 스테이지의 아레나 레벨에 직접 배치한다 — 런타임 스폰 경로가 없으므로 배치 여부 자체를 검사한다
	TArray<AACEnemyCharacter*> PlacedBosses;
	for (AACEnemyCharacter* Enemy : FindActors<AACEnemyCharacter>(World))
	{
		if (Enemy && Enemy->GetBossIdentityTag().IsValid())
		{
			PlacedBosses.Add(Enemy);
		}
	}

	const TArray<AACBattleStartPoint*> BattleStarts = FindActors<AACBattleStartPoint>(World);
	const TArray<AACStageExitPoint*> StageExits = FindActors<AACStageExitPoint>(World);
	const TArray<ANavMeshBoundsVolume*> NavBounds = FindActors<ANavMeshBoundsVolume>(World);
	const TArray<ALevelSequenceActor*> Sequences = FindActors<ALevelSequenceActor>(World);

	auto AddCount = [&](const TCHAR* Label, int32 Count, bool bRequired)
	{
		const bool bValid = !bRequired || Count > 0;
		AddLine(
			ArenaValidationBox,
			FString::Printf(TEXT("%s %s: %d"), bValid ? TEXT("✓") : TEXT("✕"), Label, Count),
			bValid ? GoodColor : ErrorColor);
	};

	AddCount(TEXT("PlayerStart"), PlayerStarts.Num(), true);
	AddCount(TEXT("배치된 보스"), PlacedBosses.Num(), true);
	// 전투 시작 지점은 로비에 두는 액터이므로 아레나에는 없는 것이 정상이다
	AddCount(TEXT("ACBattleStartPoint"), BattleStarts.Num(), false);
	AddCount(TEXT("ACStageExitPoint"), StageExits.Num(), true);
	AddCount(TEXT("NavMeshBoundsVolume"), NavBounds.Num(), true);
	AddCount(TEXT("LevelSequenceActor"), Sequences.Num(), false);

	const TSubclassOf<AGameModeBase> DefaultGameMode = World->GetWorldSettings() ? World->GetWorldSettings()->DefaultGameMode : nullptr;
	const bool bCorrectGameMode = DefaultGameMode && DefaultGameMode->IsChildOf(AACGameMode::StaticClass());
	AddLine(
		ArenaValidationBox,
		FString::Printf(TEXT("%s GameMode Override: %s"), bCorrectGameMode ? TEXT("✓") : TEXT("✕"), DefaultGameMode ? *DefaultGameMode->GetName() : TEXT("None")),
		bCorrectGameMode ? GoodColor : ErrorColor);

	if (!PlayerStarts.IsEmpty() && !PlacedBosses.IsEmpty())
	{
		const float StartDistance = FVector::Dist(PlayerStarts[0]->GetActorLocation(), PlacedBosses[0]->GetActorLocation());
		const bool bDistanceReasonable = StartDistance >= 500.f && StartDistance <= 2500.f;
		AddLine(
			ArenaValidationBox,
			FString::Printf(TEXT("%s 시작 거리: %.0fcm (권장 500~2500cm)"), bDistanceReasonable ? TEXT("✓") : TEXT("주의:"), StartDistance),
			bDistanceReasonable ? GoodColor : WarningColor);

		if (!NavBounds.IsEmpty())
		{
			bool bPlayerCovered = false;
			bool bBossCovered = false;
			for (const ANavMeshBoundsVolume* NavVolume : NavBounds)
			{
				const FBox Bounds = NavVolume->GetComponentsBoundingBox(true);
				bPlayerCovered |= Bounds.IsInsideOrOn(PlayerStarts[0]->GetActorLocation());
				bBossCovered |= Bounds.IsInsideOrOn(PlacedBosses[0]->GetActorLocation());
			}
			AddLine(ArenaValidationBox, bPlayerCovered ? TEXT("✓ PlayerStart가 NavMeshBounds 내부에 있습니다.") : TEXT("주의: PlayerStart가 NavMeshBounds 밖에 있습니다."), bPlayerCovered ? GoodColor : WarningColor);
			AddLine(ArenaValidationBox, bBossCovered ? TEXT("✓ 배치된 보스가 NavMeshBounds 내부에 있습니다.") : TEXT("주의: 배치된 보스가 NavMeshBounds 밖에 있습니다."), bBossCovered ? GoodColor : WarningColor);
		}
	}
}

FReply SACBossCombatWorkbench::HandlePlaceArenaTemplate()
{
	using namespace ACBossWorkbench;
	UWorld* World = GetEditorWorld();
	if (!World || !World->GetCurrentLevel())
	{
		Notify(TEXT("편집 중인 레벨을 찾지 못했습니다."), false);
		return FReply::Handled();
	}

	AActor* SelectedActor = GEditor->GetSelectedActors() ? GEditor->GetSelectedActors()->GetTop<AActor>() : nullptr;
	const FVector Anchor = SelectedActor ? SelectedActor->GetActorLocation() : FVector::ZeroVector;
	const FScopedTransaction Transaction(LOCTEXT("PlaceArenaTemplateTransaction", "Place Boss Arena Template Actors"));

	auto AddIfMissing = [&](UClass* ActorClass, const FVector& Offset, const TCHAR* Label) -> AActor*
	{
		for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
		{
			return *It;
		}

		AActor* NewActor = GEditor->AddActor(
			World->GetCurrentLevel(),
			ActorClass,
			FTransform(FRotator::ZeroRotator, Anchor + Offset),
			true,
			RF_Transactional,
			false);
		if (NewActor)
		{
			NewActor->SetActorLabel(Label);
			NewActor->Modify();
		}
		return NewActor;
	};

	AddIfMissing(APlayerStart::StaticClass(), FVector(-800.f, 0.f, 0.f), TEXT("Arena_PlayerStart"));
	AddIfMissing(AACStageExitPoint::StaticClass(), FVector(-1100.f, 0.f, 0.f), TEXT("Arena_StageExit"));
	if (AActor* NavActor = AddIfMissing(ANavMeshBoundsVolume::StaticClass(), FVector::ZeroVector, TEXT("Arena_NavMeshBounds")))
	{
		if (NavActor->GetActorScale3D().Equals(FVector::OneVector))
		{
			NavActor->SetActorScale3D(FVector(12.f, 12.f, 3.f));
		}
	}

	World->MarkPackageDirty();
	GEditor->RedrawLevelEditingViewports(true);
	RefreshArenaValidation();
	return FReply::Handled();
}

void SACBossCombatWorkbench::RefreshMontageAudit()
{
	using namespace ACBossWorkbench;
	if (!MontageAuditBox.IsValid())
	{
		return;
	}

	MontageAuditBox->ClearChildren();

	const TArray<FACAssetReport> Reports = ACMontageAudit::AuditAll();
	int32 ProblemMontageCount = 0;
	int32 NoteOnlyMontageCount = 0;

	for (const FACAssetReport& Report : Reports)
	{
		const int32 ProblemCount = Report.CountBySeverity(EACAssetIssueSeverity::Error) + Report.CountBySeverity(EACAssetIssueSeverity::Warning);
		const int32 NoteCount = Report.CountBySeverity(EACAssetIssueSeverity::Info);

		ProblemMontageCount += ProblemCount > 0 ? 1 : 0;
		NoteOnlyMontageCount += (ProblemCount == 0 && NoteCount > 0) ? 1 : 0;

		TArray<FString> ProblemTexts;
		TArray<FString> NoteTexts;
		for (const FACAssetIssue& Issue : Report.Issues)
		{
			if (Issue.Severity == EACAssetIssueSeverity::Info)
			{
				NoteTexts.Add(Issue.Message);
			}
			else
			{
				ProblemTexts.Add(Issue.Message);
			}
		}

		FString AuditText = ProblemTexts.IsEmpty() ? TEXT("필수 항목 정상") : FString::Join(ProblemTexts, TEXT(" · "));
		if (!NoteTexts.IsEmpty())
		{
			AuditText += FString::Printf(TEXT("  ·  참고: %s"), *FString::Join(NoteTexts, TEXT(", ")));
		}

		const FLinearColor RowColor = ProblemCount > 0 ? WarningColor : (NoteCount > 0 ? InfoColor : GoodColor);
		const FSoftObjectPath AssetPath = Report.AssetPath;

		MontageAuditBox->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.35f)[MakeText(Report.AssetName, RowColor)]
			+ SHorizontalBox::Slot().FillWidth(0.65f)[MakeText(AuditText, RowColor)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).Text(LOCTEXT("OpenMontage", "열기")).OnClicked_Lambda([AssetPath]
				{
					OpenAssetByPath(AssetPath);
					return FReply::Handled();
				})
			]
		];
	}

	MontageAuditBox->InsertSlot(0)
	.AutoHeight()
	.Padding(0.f, 0.f, 0.f, 5.f)
	[
		MakeText(
			FString::Printf(TEXT("공격 몽타주 %d개 검사 — 필수 확인 %d개 · 참고만 %d개"), Reports.Num(), ProblemMontageCount, NoteOnlyMontageCount),
			ProblemMontageCount > 0 ? WarningColor : GoodColor)
	];
}

void SACBossCombatWorkbench::RefreshStateTreeValidation()
{
	using namespace ACBossWorkbench;
	if (!StateTreeValidationBox.IsValid())
	{
		return;
	}

	StateTreeValidationBox->ClearChildren();

	const TArray<FACStateTreeReport> Reports = ACStateTreeValidator::ValidateAllStateTrees();
	if (Reports.IsEmpty())
	{
		AddLine(StateTreeValidationBox, TEXT("/Game 아래에서 StateTree 에셋을 찾지 못했습니다."), InfoColor);
		return;
	}

	int32 TotalErrors = 0;
	int32 TotalWarnings = 0;
	int32 TotalInfos = 0;

	for (const FACStateTreeReport& Report : Reports)
	{
		const int32 ErrorCount = Report.CountBySeverity(EACStateTreeIssueSeverity::Error);
		const int32 WarningCount = Report.CountBySeverity(EACStateTreeIssueSeverity::Warning);
		const int32 InfoCount = Report.CountBySeverity(EACStateTreeIssueSeverity::Info);
		TotalErrors += ErrorCount;
		TotalWarnings += WarningCount;
		TotalInfos += InfoCount;

		const FLinearColor HeaderColor = ErrorCount > 0 ? ErrorColor : (WarningCount > 0 ? WarningColor : GoodColor);
		const FSoftObjectPath AssetPath = Report.AssetPath;

		StateTreeValidationBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 8.f, 0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				MakeText(
					FString::Printf(
						TEXT("%s — 상태 %d개 / 오류 %d · 주의 %d · 참고 %d"),
						*Report.AssetName,
						Report.StateCount,
						ErrorCount,
						WarningCount,
						InfoCount),
					HeaderColor)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).Text(LOCTEXT("OpenStateTree", "열기")).OnClicked_Lambda([AssetPath]
				{
					OpenAssetByPath(AssetPath);
					return FReply::Handled();
				})
			]
		];

		if (Report.Issues.IsEmpty())
		{
			AddLine(StateTreeValidationBox, TEXT("  ✓ 문제 없음"), GoodColor);
			continue;
		}

		for (const FACStateTreeIssue& Issue : Report.Issues)
		{
			const TCHAR* Prefix = TEXT("참고");
			FLinearColor IssueColor = MutedColor;
			if (Issue.Severity == EACStateTreeIssueSeverity::Error)
			{
				Prefix = TEXT("오류");
				IssueColor = ErrorColor;
			}
			else if (Issue.Severity == EACStateTreeIssueSeverity::Warning)
			{
				Prefix = TEXT("주의");
				IssueColor = WarningColor;
			}

			AddLine(
				StateTreeValidationBox,
				FString::Printf(TEXT("  [%s] %s — %s"), Prefix, *Issue.StatePath, *Issue.Message),
				IssueColor);
		}
	}

	StateTreeValidationBox->InsertSlot(0)
	.AutoHeight()
	.Padding(0.f, 0.f, 0.f, 5.f)
	[
		MakeText(
			FString::Printf(
				TEXT("StateTree %d개 검사 — 오류 %d · 주의 %d · 참고 %d"),
				Reports.Num(),
				TotalErrors,
				TotalWarnings,
				TotalInfos),
			TotalErrors > 0 ? ErrorColor : (TotalWarnings > 0 ? WarningColor : GoodColor))
	];
}

void SACBossCombatWorkbench::RefreshAttributeMatrix()
{
	using namespace ACBossWorkbench;

	if (!AttributeMatrixBox.IsValid())
	{
		return;
	}

	AttributeMatrixBox->ClearChildren();

	const TArray<FACAttributeProfile> Profiles = ACAttributeAudit::CollectAllProfiles();
	if (Profiles.IsEmpty())
	{
		AddLine(AttributeMatrixBox, TEXT("/Game 아래에서 Startup DataAsset을 찾지 못했습니다."), InfoColor);
		return;
	}

	AddLine(AttributeMatrixBox, FString::Printf(TEXT("캐릭터 %d개 · 어트리뷰트 %d개 (레벨 1 기준)"), Profiles.Num(), ACAttributeAudit::GetTrackedAttributes().Num()), InfoColor);

	// 복제 잔재 경고 — 한 캐릭터의 초기화 GE가 서로 다른 CurveTable을 참조하면 대개 실수다
	for (const FACAttributeProfile& Profile : Profiles)
	{
		if (Profile.ReferencedCurveTables.Num() <= 1)
		{
			continue;
		}

		TArray<FString> TableNames;
		for (const TWeakObjectPtr<UCurveTable>& Table : Profile.ReferencedCurveTables)
		{
			TableNames.Add(Table.IsValid() ? Table->GetName() : TEXT("(없음)"));
		}

		AddLine(
			AttributeMatrixBox,
			FString::Printf(TEXT("주의: %s의 초기화 GE가 CurveTable을 %d개 참조합니다 — %s"), *Profile.CharacterName, Profile.ReferencedCurveTables.Num(), *FString::Join(TableNames, TEXT(", "))),
			WarningColor);
	}

	AddSeparator(AttributeMatrixBox);

	TSharedRef<SHorizontalBox> HeaderRow = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(180.f)[MakeText(TEXT("어트리뷰트"), MutedColor)]];
	for (const FACAttributeProfile& Profile : Profiles)
	{
		HeaderRow->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(150.f)[MakeText(Profile.CharacterName, MutedColor)]];
	}
	AttributeMatrixBox->AddSlot().AutoHeight().Padding(2.f)[HeaderRow];

	const TArray<FGameplayAttribute>& TrackedAttributes = ACAttributeAudit::GetTrackedAttributes();
	for (int32 AttributeIndex = 0; AttributeIndex < TrackedAttributes.Num(); ++AttributeIndex)
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(180.f)[MakeText(MakeShortAttributeName(TrackedAttributes[AttributeIndex]))]
			];

		for (const FACAttributeProfile& Profile : Profiles)
		{
			if (!Profile.Entries.IsValidIndex(AttributeIndex))
			{
				Row->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(150.f)];
				continue;
			}

			const FACAttributeEntry& Entry = Profile.Entries[AttributeIndex];

			FLinearColor CellColor = FLinearColor::White;
			FString SourceText = TEXT("GE 상수값");
			switch (Entry.Source)
			{
				case EACAttributeSource::NotSet:
					CellColor = MutedColor;
					SourceText = TEXT("설정 안 됨 — AttributeSet 생성자 기본값");
					break;

				case EACAttributeSource::CurveTable:
					CellColor = InfoColor;
					SourceText = FString::Printf(TEXT("%s · %s[%s]"), *Entry.SourceEffectName, Entry.CurveTable.IsValid() ? *Entry.CurveTable->GetName() : TEXT("(없음)"), *Entry.CurveRowName.ToString());
					break;

				case EACAttributeSource::Dynamic:
					CellColor = WarningColor;
					SourceText = FString::Printf(TEXT("%s · 적용 시점에 계산됨"), *Entry.SourceEffectName);
					break;

				default:
					SourceText = FString::Printf(TEXT("%s · GE 상수값"), *Entry.SourceEffectName);
					break;
			}

			const FText TooltipText = FText::FromString(SourceText);

			// CurveTable에서 온 값만 인라인 편집을 허용한다. 나머지는 원본 에셋을 열어 고쳐야 한다
			if (Entry.Source == EACAttributeSource::CurveTable && Entry.CurveTable.IsValid())
			{
				const TWeakObjectPtr<UCurveTable> CapturedTable = Entry.CurveTable;
				const FName CapturedRowName = Entry.CurveRowName;
				const float CapturedValue = Entry.Value;

				Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f)
				[
					SNew(SBox)
					.WidthOverride(146.f)
					.ToolTipText(TooltipText)
					[
						SNew(SNumericEntryBox<float>)
						.AllowSpin(false)
						.Value_Lambda([CapturedValue] { return TOptional<float>(CapturedValue); })
						.OnValueCommitted_Lambda([this, CapturedTable, CapturedRowName](float NewValue, ETextCommit::Type)
						{
							if (ACAttributeAudit::SetCurveValue(CapturedTable.Get(), CapturedRowName, 1.f, NewValue))
							{
								bAttributeMatrixPending = true;
							}
						})
					]
				];
			}
			else
			{
				Row->AddSlot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(150.f)
					.ToolTipText(TooltipText)
					[
						MakeText(FString::Printf(TEXT("%.2f"), Entry.Value), CellColor)
					]
				];
			}
		}

		AttributeMatrixBox->AddSlot().AutoHeight().Padding(2.f, 1.f)[Row];
	}

	AddSeparator(AttributeMatrixBox);
	AddLine(AttributeMatrixBox, TEXT("파랑 = CurveTable(편집 가능) · 흰색 = GE 상수값 · 주황 = 적용 시점 계산 · 회색 = 설정 안 됨"), MutedColor);
	AddLine(AttributeMatrixBox, TEXT("CurveTable을 CSV에서 임포트했다면 여기서 고친 값은 재임포트 시 덮어써집니다."), MutedColor);
}

UAbilitySystemComponent* SACBossCombatWorkbench::GetLiveEditTargetASC() const
{
	UWorld* World = GetPIEWorld();
	if (!World)
	{
		return nullptr;
	}

	if (bLiveEditTargetIsBoss)
	{
		AACEnemyCharacter* Boss = ACBossWorkbench::FindBoss(World);
		return Boss ? Boss->GetACAbilitySystemComponent() : nullptr;
	}

	if (const APlayerController* PlayerController = World->GetFirstPlayerController())
	{
		if (AACCharacterBase* Player = Cast<AACCharacterBase>(PlayerController->GetPawn()))
		{
			return Player->GetACAbilitySystemComponent();
		}
	}
	return nullptr;
}

void SACBossCombatWorkbench::SetLiveAttributeValue(const FGameplayAttribute& Attribute, const float NewValue)
{
	if (UAbilitySystemComponent* ASC = GetLiveEditTargetASC())
	{
		ASC->SetNumericAttributeBase(Attribute, NewValue);
	}
}

void SACBossCombatWorkbench::InjectMetaAttribute(const FGameplayAttribute& Attribute, const float Amount)
{
	UAbilitySystemComponent* ASC = GetLiveEditTargetASC();
	if (!ASC || FMath::IsNearlyZero(Amount))
	{
		return;
	}

	UGameplayEffect* DebugEffect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("ACWorkbenchMetaInject")));
	DebugEffect->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& Modifier = DebugEffect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Amount));

	ASC->ApplyGameplayEffectToSelf(DebugEffect, 1.f, ASC->MakeEffectContext());
}

#undef LOCTEXT_NAMESPACE
