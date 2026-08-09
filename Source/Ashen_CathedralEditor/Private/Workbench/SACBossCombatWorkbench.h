#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;
class SWidgetSwitcher;
class UAbilitySystemComponent;
struct FAssetData;
struct FGameplayAttribute;

class SACBossCombatWorkbench : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SACBossCombatWorkbench)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedRef<SWidget> BuildNavigation();
	TSharedRef<SWidget> BuildTestAndLivePage();
	TSharedRef<SWidget> BuildTuningPage();
	TSharedRef<SWidget> BuildValidationPage();
	TSharedRef<SWidget> BuildArenaPage();
	TSharedRef<SWidget> BuildMontagePage();
	TSharedRef<SWidget> BuildStateTreePage();
	TSharedRef<SWidget> BuildStatsPage();

	/** PIE 중에만 동작하는 어트리뷰트 편집 섹션. 값은 람다 바인딩이라 한 번만 만들면 된다 */
	TSharedRef<SWidget> BuildLiveAttributeEditor();
	TSharedRef<SWidget> MakeLiveAttributeRow(const FGameplayAttribute& Attribute);
	TSharedRef<SWidget> MakeMetaInjectRow(const FGameplayAttribute& Attribute, int32 MetaIndex);
	TSharedRef<SWidget> MakePageHeader(const FText& Title, const FText& Description) const;

	void SetActivePage(int32 PageIndex);
	void RefreshLiveMonitor();
	void RefreshTuningComparison();
	void RefreshBossValidation();
	void RefreshArenaValidation();
	void RefreshMontageAudit();
	void RefreshStateTreeValidation();
	void RefreshAttributeMatrix();

	/** 라이브 편집 대상(플레이어/보스)의 ASC. PIE가 아니면 nullptr */
	UAbilitySystemComponent* GetLiveEditTargetASC() const;

	/** BaseValue를 직접 덮어쓴다. GE를 거치지 않으므로 붕괴/사망 판정은 돌지 않는다 */
	void SetLiveAttributeValue(const FGameplayAttribute& Attribute, float NewValue);

	/** 메타 어트리뷰트에 즉시형 GE를 적용해 PostGameplayEffectExecute까지 정상적으로 태운다 */
	void InjectMetaAttribute(const FGameplayAttribute& Attribute, float Amount);

	void HandleMapAssetChanged(const FAssetData& AssetData);
	FReply HandleLoadSelectedMap();
	FReply HandleStartPIE();
	FReply HandleStopPIE();
	FReply HandleRestartStage();
	FReply HandleSetBossHealth(float HealthRatio);
	FReply HandleForceNextPhase();
	FReply HandleWebDebug(bool bEnable);
	FReply HandleOpenWebDashboard();
	FReply HandleOpenRunLogs();
	FReply HandlePlaceArenaTemplate();

	UWorld* GetEditorWorld() const;
	UWorld* GetPIEWorld() const;

	TSharedPtr<SWidgetSwitcher> PageSwitcher;
	TSharedPtr<SVerticalBox> LiveMonitorBox;
	TSharedPtr<SVerticalBox> TuningComparisonBox;
	TSharedPtr<SVerticalBox> BossValidationBox;
	TSharedPtr<SVerticalBox> ArenaValidationBox;
	TSharedPtr<SVerticalBox> MontageAuditBox;
	TSharedPtr<SVerticalBox> StateTreeValidationBox;
	TSharedPtr<SVerticalBox> AttributeMatrixBox;

	FSoftObjectPath SelectedMapPath;
	double LastLiveRefreshSeconds = 0.0;

	/** StateTree 검사는 어빌리티 에셋을 로드하므로 탭을 열 때가 아니라 페이지에 처음 들어올 때 한 번만 돌린다 */
	bool bStateTreeValidationPending = true;

	/** 스탯 매트릭스도 초기화 GE를 로드하므로 같은 규칙을 따른다 */
	bool bAttributeMatrixPending = true;

	/** 라이브 편집 대상. false면 플레이어 */
	bool bLiveEditTargetIsBoss = true;

	/** 피해 주입 입력값. ACAttributeAudit::GetMetaAttributes()와 같은 순서 */
	TArray<float> MetaInjectAmounts;
};
