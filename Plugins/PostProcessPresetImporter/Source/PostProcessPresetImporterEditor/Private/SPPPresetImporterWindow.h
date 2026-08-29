// 임포터 창 위젯.

#pragma once

#include "CoreMinimal.h"
#include "PPPresetTypes.h"
#include "Widgets/SCompoundWidget.h"

class APostProcessVolume;
class UPostProcessComponent;
struct FPostProcessSettings;
class SEditableTextBox;
template <typename ItemType> class SComboBox;
template <typename ItemType> class SListView;

/**
 * @brief 프리셋을 적용할 수 있는 대상 하나.
 * @note PostProcessVolume 액터와 블루프린트 안의 PostProcessComponent를 모두 담는다.
 */
struct FPPTargetEntry
{
	TWeakObjectPtr<APostProcessVolume> Volume;
	TWeakObjectPtr<UPostProcessComponent> Component;

	/** 드롭다운에 보일 설명. 우선순위/경계/오버라이드 개수를 포함한다. */
	FString Label;

	bool IsValid() const { return Volume.IsValid() || Component.IsValid(); }
};

/** 미리보기 리스트 한 줄. */
struct FPPPreviewRow
{
	FString PropertyName;
	FString OldValue;
	FString NewValue;
};

class SPPPresetImporterWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPPPresetImporterWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// --- JSON ---
	void OnBrowseClicked();
	FReply OnReloadClicked();
	void LoadJsonFile(const FString& InPath);

	// --- 대상 볼륨 ---
	void RefreshVolumeList();
	FReply OnUseSelectedVolumeClicked();

	/** 선택된 대상. 없으면 nullptr. */
	const FPPTargetEntry* GetSelectedTarget() const;

	/** 선택된 대상의 현재 설정. 미리보기 비교 기준. */
	const FPostProcessSettings* GetTargetSettings() const;

	// --- 미리보기 / 적용 ---
	void RefreshPreview();
	FReply OnApplyClicked();
	FReply OnCreateDataAssetClicked();

	bool IsApplyEnabled() const;
	bool IsPresetSelected() const;

	// --- 위젯 콜백 ---
	TSharedRef<SWidget> MakePresetComboItem(TSharedPtr<FString> InItem);
	TSharedRef<SWidget> MakeVolumeComboItem(TSharedPtr<FString> InItem);
	TSharedRef<class ITableRow> MakePreviewRow(TSharedPtr<FPPPreviewRow> InItem, const TSharedRef<class STableViewBase>& OwnerTable);
	TSharedRef<class ITableRow> MakeWarningRow(TSharedPtr<FString> InItem, const TSharedRef<class STableViewBase>& OwnerTable);

	FText GetPresetComboText() const;
	FText GetVolumeComboText() const;
	FText GetStatusText() const;

private:
	FString JsonFilePath;
	FString StatusMessage;

	TArray<FPPPresetEntry> Presets;

	TArray<TSharedPtr<FString>> PresetNames;
	TSharedPtr<FString> SelectedPresetName;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> PresetCombo;

	TArray<FPPTargetEntry> Targets;
	TArray<TSharedPtr<FString>> VolumeNames;
	TSharedPtr<FString> SelectedVolumeName;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> VolumeCombo;

	TArray<TSharedPtr<FPPPreviewRow>> PreviewRows;
	TSharedPtr<SListView<TSharedPtr<FPPPreviewRow>>> PreviewList;

	TArray<TSharedPtr<FString>> WarningRows;
	TSharedPtr<SListView<TSharedPtr<FString>>> WarningList;

	TSharedPtr<SEditableTextBox> FilePathBox;
};
