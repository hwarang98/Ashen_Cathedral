#include "SPPPresetImporterWindow.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "DesktopPlatformModule.h"
#include "Editor.h"
#include "Components/PostProcessComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "IDesktopPlatform.h"
#include "ObjectTools.h"
#include "PPPresetApplier.h"
#include "PPPresetDataAsset.h"
#include "PPPresetJsonReader.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "PPPresetImporter"

namespace
{
	const FName ColumnProperty("Property");
	const FName ColumnOld("Old");
	const FName ColumnNew("New");
}

void SPPPresetImporterWindow::Construct(const FArguments& InArgs)
{
	StatusMessage = TEXT("JSON 파일을 선택하세요.");
	RefreshVolumeList();

	ChildSlot
	[
		SNew(SVerticalBox)

		// --- JSON 파일 선택 ---
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(STextBlock).Text(LOCTEXT("JsonFile", "JSON 파일"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SAssignNew(FilePathBox, SEditableTextBox)
				.HintText(LOCTEXT("JsonHint", "프리셋 JSON 경로"))
				.IsReadOnly(true)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Browse", "찾아보기"))
				.OnClicked_Lambda([this]() { OnBrowseClicked(); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Reload", "다시 불러오기"))
				.OnClicked(this, &SPPPresetImporterWindow::OnReloadClicked)
			]
		]

		// --- 프리셋 선택 ---
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 0.f, 6.f, 6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(STextBlock).Text(LOCTEXT("Preset", "프리셋"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SAssignNew(PresetCombo, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&PresetNames)
				.OnGenerateWidget(this, &SPPPresetImporterWindow::MakePresetComboItem)
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> InItem, ESelectInfo::Type)
				{
					SelectedPresetName = InItem;
					RefreshPreview();
				})
				[
					SNew(STextBlock).Text(this, &SPPPresetImporterWindow::GetPresetComboText)
				]
			]
		]

		// --- 대상 볼륨 ---
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 0.f, 6.f, 6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(STextBlock).Text(LOCTEXT("Target", "대상 볼륨"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SAssignNew(VolumeCombo, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&VolumeNames)
				.OnGenerateWidget(this, &SPPPresetImporterWindow::MakeVolumeComboItem)
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> InItem, ESelectInfo::Type)
				{
					SelectedVolumeName = InItem;
					RefreshPreview();
				})
				[
					SNew(STextBlock).Text(this, &SPPPresetImporterWindow::GetVolumeComboText)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("UseSelected", "선택된 볼륨 사용"))
				.OnClicked(this, &SPPPresetImporterWindow::OnUseSelectedVolumeClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("RefreshVolumes", "목록 새로고침"))
				.OnClicked_Lambda([this]() { RefreshVolumeList(); RefreshPreview(); return FReply::Handled(); })
			]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 0.f)[ SNew(SSeparator) ]

		// --- 적용 미리보기 ---
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f)
		[
			SNew(STextBlock).Text(LOCTEXT("Preview", "적용될 속성 (기존 값 → 새 값)"))
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(6.f, 0.f)
		[
			SNew(SBorder)
			[
				SAssignNew(PreviewList, SListView<TSharedPtr<FPPPreviewRow>>)
				.ListItemsSource(&PreviewRows)
				.OnGenerateRow(this, &SPPPresetImporterWindow::MakePreviewRow)
				.SelectionMode(ESelectionMode::None)
				.HeaderRow(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(ColumnProperty).DefaultLabel(LOCTEXT("ColProperty", "프로퍼티")).FillWidth(0.4f)
					+ SHeaderRow::Column(ColumnOld).DefaultLabel(LOCTEXT("ColOld", "기존")).FillWidth(0.3f)
					+ SHeaderRow::Column(ColumnNew).DefaultLabel(LOCTEXT("ColNew", "새 값")).FillWidth(0.3f)
				)
			]
		]

		// --- 경고 ---
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 6.f, 6.f, 0.f)
		[
			SNew(STextBlock).Text(LOCTEXT("Warnings", "경고 (누락 에셋 / 알 수 없는 속성)"))
		]
		+ SVerticalBox::Slot().MaxHeight(140.f).Padding(6.f, 0.f)
		[
			SNew(SBorder)
			[
				SAssignNew(WarningList, SListView<TSharedPtr<FString>>)
				.ListItemsSource(&WarningRows)
				.OnGenerateRow(this, &SPPPresetImporterWindow::MakeWarningRow)
				.SelectionMode(ESelectionMode::None)
			]
		]

		// --- 상태 + 실행 버튼 ---
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f)
		[
			SNew(STextBlock).Text(this, &SPPPresetImporterWindow::GetStatusText).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("Apply", "Apply to Volume"))
				.IsEnabled(this, &SPPPresetImporterWindow::IsApplyEnabled)
				.OnClicked(this, &SPPPresetImporterWindow::OnApplyClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("CreateAsset", "Create Preset DataAsset"))
				.IsEnabled(this, &SPPPresetImporterWindow::IsPresetSelected)
				.OnClicked(this, &SPPPresetImporterWindow::OnCreateDataAssetClicked)
			]
		]
	];
}

void SPPPresetImporterWindow::OnBrowseClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	TArray<FString> OutFiles;
	const bool bPicked = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(AsShared()),
		TEXT("프리셋 JSON 선택"),
		FPaths::ProjectDir(),
		TEXT(""),
		TEXT("JSON files (*.json)|*.json"),
		EFileDialogFlags::None,
		OutFiles);

	if (bPicked && OutFiles.Num() > 0)
	{
		LoadJsonFile(OutFiles[0]);
	}
}

FReply SPPPresetImporterWindow::OnReloadClicked()
{
	if (!JsonFilePath.IsEmpty())
	{
		LoadJsonFile(JsonFilePath);
	}
	return FReply::Handled();
}

void SPPPresetImporterWindow::LoadJsonFile(const FString& InPath)
{
	JsonFilePath = InPath;
	if (FilePathBox.IsValid())
	{
		FilePathBox->SetText(FText::FromString(JsonFilePath));
	}

	FString Error;
	if (!FPPPresetJsonReader::LoadFromFile(JsonFilePath, Presets, Error))
	{
		StatusMessage = FString::Printf(TEXT("불러오기 실패: %s"), *Error);
		PresetNames.Reset();
		SelectedPresetName.Reset();
		if (PresetCombo.IsValid()) { PresetCombo->RefreshOptions(); }
		RefreshPreview();
		return;
	}

	PresetNames.Reset();
	for (const FPPPresetEntry& Entry : Presets)
	{
		PresetNames.Add(MakeShared<FString>(Entry.PresetName));
	}

	SelectedPresetName = PresetNames.Num() > 0 ? PresetNames[0] : nullptr;
	if (PresetCombo.IsValid())
	{
		PresetCombo->RefreshOptions();
		PresetCombo->SetSelectedItem(SelectedPresetName);
	}

	StatusMessage = FString::Printf(TEXT("프리셋 %d개를 불러왔습니다."), Presets.Num());
	RefreshPreview();
}

namespace
{
	/** 오버라이드가 켜진 개수를 센다. 볼륨의 성격을 한눈에 알려주는 값이다. */
	int32 CountOverrides(const FPostProcessSettings& InSettings)
	{
		int32 Count = 0;
		for (TFieldIterator<FBoolProperty> It(FPostProcessSettings::StaticStruct()); It; ++It)
		{
			if (It->GetName().StartsWith(TEXT("bOverride_")) && It->GetPropertyValue_InContainer(&InSettings))
			{
				++Count;
			}
		}
		return Count;
	}

	FString MakeTargetLabel(const FString& InName, const FString& InKind, float InPriority, bool bUnbound, const FPostProcessSettings& InSettings)
	{
		return FString::Printf(TEXT("%s  [%s | Priority %.0f | %s | 오버라이드 %d]"),
			*InName, *InKind, InPriority,
			bUnbound ? TEXT("Unbound") : TEXT("경계 있음"),
			CountOverrides(InSettings));
	}
}

void SPPPresetImporterWindow::RefreshVolumeList()
{
	Targets.Reset();
	VolumeNames.Reset();

	if (GEditor)
	{
		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			// 1) PostProcessVolume 액터
			for (TActorIterator<APostProcessVolume> It(World); It; ++It)
			{
				FPPTargetEntry Entry;
				Entry.Volume = *It;
				Entry.Label = MakeTargetLabel(It->GetActorNameOrLabel(), TEXT("Volume"),
					It->Priority, It->bUnbound != 0, It->Settings);
				Targets.Add(Entry);
			}

			// 2) 블루프린트 등에 붙어 있는 PostProcessComponent
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				TArray<UPostProcessComponent*> Components;
				It->GetComponents(Components);
				for (UPostProcessComponent* Component : Components)
				{
					if (!IsValid(Component))
					{
						continue;
					}
					FPPTargetEntry Entry;
					Entry.Component = Component;
					Entry.Label = MakeTargetLabel(
						FString::Printf(TEXT("%s > %s"), *It->GetActorNameOrLabel(), *Component->GetName()),
						TEXT("Component"), Component->Priority, Component->bUnbound != 0, Component->Settings);
					Targets.Add(Entry);
				}
			}
		}
	}

	for (const FPPTargetEntry& Entry : Targets)
	{
		VolumeNames.Add(MakeShared<FString>(Entry.Label));
	}

	const int32 Previous = SelectedVolumeName.IsValid()
		? VolumeNames.IndexOfByPredicate([this](const TSharedPtr<FString>& In) { return In.IsValid() && *In == *SelectedVolumeName; })
		: INDEX_NONE;

	SelectedVolumeName = VolumeNames.IsValidIndex(Previous)
		? VolumeNames[Previous]
		: (VolumeNames.Num() > 0 ? VolumeNames[0] : nullptr);

	if (VolumeCombo.IsValid())
	{
		VolumeCombo->RefreshOptions();
		VolumeCombo->SetSelectedItem(SelectedVolumeName);
	}
}

FReply SPPPresetImporterWindow::OnUseSelectedVolumeClicked()
{
	if (!GEditor)
	{
		return FReply::Handled();
	}

	RefreshVolumeList();

	USelection* Selection = GEditor->GetSelectedActors();
	for (int32 Index = 0; Index < Selection->Num(); ++Index)
	{
		AActor* Actor = Cast<AActor>(Selection->GetSelectedObject(Index));
		if (!Actor)
		{
			continue;
		}

		const int32 Found = Targets.IndexOfByPredicate([Actor](const FPPTargetEntry& In)
		{
			if (In.Volume.IsValid() && In.Volume.Get() == Actor) { return true; }
			return In.Component.IsValid() && In.Component->GetOwner() == Actor;
		});

		if (VolumeNames.IsValidIndex(Found))
		{
			SelectedVolumeName = VolumeNames[Found];
			if (VolumeCombo.IsValid()) { VolumeCombo->SetSelectedItem(SelectedVolumeName); }
			StatusMessage = FString::Printf(TEXT("대상: %s"), **SelectedVolumeName);
			RefreshPreview();
			return FReply::Handled();
		}
	}

	StatusMessage = TEXT("선택된 액터에 PostProcessVolume이나 PostProcessComponent가 없습니다.");
	return FReply::Handled();
}

const FPPTargetEntry* SPPPresetImporterWindow::GetSelectedTarget() const
{
	const int32 Index = VolumeNames.IndexOfByKey(SelectedVolumeName);
	return Targets.IsValidIndex(Index) && Targets[Index].IsValid() ? &Targets[Index] : nullptr;
}

const FPostProcessSettings* SPPPresetImporterWindow::GetTargetSettings() const
{
	const FPPTargetEntry* Target = GetSelectedTarget();
	if (!Target)
	{
		return nullptr;
	}
	if (Target->Volume.IsValid()) { return &Target->Volume->Settings; }
	if (Target->Component.IsValid()) { return &Target->Component->Settings; }
	return nullptr;
}

void SPPPresetImporterWindow::RefreshPreview()
{
	PreviewRows.Reset();
	WarningRows.Reset();

	const int32 PresetIndex = PresetNames.IndexOfByKey(SelectedPresetName);
	const FPostProcessSettings* Current = GetTargetSettings();

	if (Presets.IsValidIndex(PresetIndex) && Current)
	{
		FPPApplyPlan Plan;
		FPPPresetApplier::BuildPlan(Presets[PresetIndex], *Current, Plan);

		for (const FPPPropertyChange& Change : Plan.Changes)
		{
			PreviewRows.Add(MakeShared<FPPPreviewRow>(FPPPreviewRow{ Change.PropertyName, Change.OldValue, Change.NewValue }));
		}
		for (const FString& Path : Plan.MissingAssetPaths)
		{
			WarningRows.Add(MakeShared<FString>(FString::Printf(TEXT("[누락 에셋] %s"), *Path)));
		}
		for (const FString& Name : Plan.UnknownProperties)
		{
			WarningRows.Add(MakeShared<FString>(FString::Printf(TEXT("[알 수 없는 속성] %s"), *Name)));
		}
		for (const FString& Name : Plan.SkippedProperties)
		{
			WarningRows.Add(MakeShared<FString>(FString::Printf(TEXT("[건너뜀] %s"), *Name)));
		}

		// 프리셋이 특정 레벨용으로 기록돼 있으면, 현재 열린 레벨과 다를 때 경고한다.
		// Package 자체는 적용 대상을 결정하지 않는다. 사람이 실수로 다른 레벨에 붓는 것을 막기 위한 표시일 뿐이다.
		const FString& PresetPackage = Presets[PresetIndex].PackagePath;
		if (!PresetPackage.IsEmpty() && GEditor)
		{
			if (const UWorld* World = GEditor->GetEditorWorldContext().World())
			{
				FString Expected = PresetPackage;
				const int32 DotIndex = Expected.Find(TEXT("."));
				if (DotIndex != INDEX_NONE)
				{
					Expected.LeftInline(DotIndex);
				}

				const FString CurrentPackage = World->GetPackage() ? World->GetPackage()->GetName() : FString();
				if (!CurrentPackage.IsEmpty() && !Expected.Equals(CurrentPackage, ESearchCase::IgnoreCase))
				{
					WarningRows.Add(MakeShared<FString>(FString::Printf(
						TEXT("[주의] 이 프리셋은 %s 용인데 현재 레벨은 %s 입니다"), *Expected, *CurrentPackage)));
				}
			}
		}
	}

	if (PreviewList.IsValid()) { PreviewList->RequestListRefresh(); }
	if (WarningList.IsValid()) { WarningList->RequestListRefresh(); }
}

FReply SPPPresetImporterWindow::OnApplyClicked()
{
	const int32 PresetIndex = PresetNames.IndexOfByKey(SelectedPresetName);
	const FPPTargetEntry* Target = GetSelectedTarget();

	if (!Presets.IsValidIndex(PresetIndex) || !Target)
	{
		StatusMessage = TEXT("프리셋과 대상을 모두 선택하세요.");
		return FReply::Handled();
	}

	FPPApplyPlan Plan;
	const bool bApplied = Target->Volume.IsValid()
		? FPPPresetApplier::ApplyToVolume(Presets[PresetIndex], Target->Volume.Get(), Plan)
		: FPPPresetApplier::ApplyToComponent(Presets[PresetIndex], Target->Component.Get(), Plan);

	if (!bApplied)
	{
		StatusMessage = TEXT("적용에 실패했습니다.");
		return FReply::Handled();
	}

	StatusMessage = FString::Printf(
		TEXT("%s 적용 완료 — 변경 %d개, 누락 에셋 %d개, 알 수 없는 속성 %d개. Ctrl+Z로 되돌릴 수 있습니다."),
		**SelectedPresetName, Plan.Changes.Num(), Plan.MissingAssetPaths.Num(), Plan.UnknownProperties.Num());

	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports(true);
	}

	// 오버라이드 개수가 바뀌었으니 목록 라벨도 갱신한다.
	RefreshVolumeList();
	RefreshPreview();
	return FReply::Handled();
}

FReply SPPPresetImporterWindow::OnCreateDataAssetClicked()
{
	const int32 PresetIndex = PresetNames.IndexOfByKey(SelectedPresetName);
	if (!Presets.IsValidIndex(PresetIndex))
	{
		StatusMessage = TEXT("프리셋을 먼저 선택하세요.");
		return FReply::Handled();
	}

	const FPPPresetEntry& Preset = Presets[PresetIndex];

	// 파일명으로 쓸 수 없는 문자를 정리한다.
	FString AssetName = FString::Printf(TEXT("PPP_%s"), *Preset.PresetName);
	AssetName = ObjectTools::SanitizeObjectName(AssetName);

	const FString PackagePath = TEXT("/Game/PostProcessPresets/Imported");
	const FString FullPackageName = FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName);

	UPackage* Package = CreatePackage(*FullPackageName);
	if (!Package)
	{
		StatusMessage = TEXT("패키지를 만들지 못했습니다.");
		return FReply::Handled();
	}
	Package->FullyLoad();

	UPPPresetDataAsset* Asset = NewObject<UPPPresetDataAsset>(Package, UPPPresetDataAsset::StaticClass(), *AssetName, RF_Public | RF_Standalone);
	if (!Asset)
	{
		StatusMessage = TEXT("DataAsset을 만들지 못했습니다.");
		return FReply::Handled();
	}

	FPPApplyPlan Plan;
	FPostProcessSettings Settings;
	FPPPresetApplier::ApplyToSettings(Preset, Settings, Plan);

	Asset->SourcePresetName = Preset.PresetName;
	Asset->SourcePackagePath = Preset.PackagePath;
	Asset->Settings = Settings;
	Asset->AppliedOverrides = Plan.AppliedOverrides;
	Asset->MissingAssetPaths = Plan.MissingAssetPaths;
	Asset->UnknownProperties = Plan.UnknownProperties;

	Asset->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Asset);

	// 저장하지 않으면 콘텐츠 브라우저에 폴더조차 나타나지 않는다. 바로 디스크에 쓴다.
	const FString FileName = FPackageName::LongPackageNameToFilename(FullPackageName, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	const bool bSaved = UPackage::SavePackage(Package, Asset, *FileName, SaveArgs);

	if (bSaved)
	{
		// 만들어진 에셋으로 콘텐츠 브라우저를 이동시켜 바로 보이게 한다.
		TArray<UObject*> Created;
		Created.Add(Asset);
		GEditor->SyncBrowserToObjects(Created);
	}

	StatusMessage = FString::Printf(
		TEXT("%s %s. 누락 에셋 %d개, 알 수 없는 속성 %d개."),
		*FullPackageName,
		bSaved ? TEXT("생성 및 저장 완료") : TEXT("생성됨 (저장 실패 - 직접 저장하세요)"),
		Plan.MissingAssetPaths.Num(), Plan.UnknownProperties.Num());

	return FReply::Handled();
}

bool SPPPresetImporterWindow::IsApplyEnabled() const
{
	return IsPresetSelected() && GetSelectedTarget() != nullptr;
}

bool SPPPresetImporterWindow::IsPresetSelected() const
{
	return Presets.IsValidIndex(PresetNames.IndexOfByKey(SelectedPresetName));
}

TSharedRef<SWidget> SPPPresetImporterWindow::MakePresetComboItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : FString()));
}

TSharedRef<SWidget> SPPPresetImporterWindow::MakeVolumeComboItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : FString()));
}

TSharedRef<ITableRow> SPPPresetImporterWindow::MakePreviewRow(TSharedPtr<FPPPreviewRow> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	class SPreviewRowWidget : public SMultiColumnTableRow<TSharedPtr<FPPPreviewRow>>
	{
	public:
		SLATE_BEGIN_ARGS(SPreviewRowWidget) {}
			SLATE_ARGUMENT(TSharedPtr<FPPPreviewRow>, Item)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
		{
			Item = InArgs._Item;
			SMultiColumnTableRow<TSharedPtr<FPPPreviewRow>>::Construct(FSuperRowType::FArguments(), InOwnerTable);
		}

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
		{
			FString Value;
			if (Item.IsValid())
			{
				if (InColumnName == ColumnProperty) { Value = Item->PropertyName; }
				else if (InColumnName == ColumnOld) { Value = Item->OldValue; }
				else { Value = Item->NewValue; }
			}
			return SNew(STextBlock).Text(FText::FromString(Value)).Margin(FMargin(4.f, 2.f));
		}

	private:
		TSharedPtr<FPPPreviewRow> Item;
	};

	return SNew(SPreviewRowWidget, OwnerTable).Item(InItem);
}

TSharedRef<ITableRow> SPPPresetImporterWindow::MakeWarningRow(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
	[
		SNew(STextBlock)
		.Text(FText::FromString(InItem.IsValid() ? *InItem : FString()))
		.Margin(FMargin(4.f, 2.f))
	];
}

FText SPPPresetImporterWindow::GetPresetComboText() const
{
	return FText::FromString(SelectedPresetName.IsValid() ? *SelectedPresetName : TEXT("(프리셋 없음)"));
}

FText SPPPresetImporterWindow::GetVolumeComboText() const
{
	return FText::FromString(SelectedVolumeName.IsValid() ? *SelectedVolumeName : TEXT("(볼륨 없음)"));
}

FText SPPPresetImporterWindow::GetStatusText() const
{
	return FText::FromString(StatusMessage);
}

#undef LOCTEXT_NAMESPACE
