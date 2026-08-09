#include "Details/ACBossTuningDetailsCustomization.h"

#include "DataAssets/AI/ACDataAsset_BossTuning.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/SACBossTuningPreview.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "ACBossTuningDetailsCustomization"

TSharedRef<IDetailCustomization> FACBossTuningDetailsCustomization::MakeInstance()
{
	return MakeShared<FACBossTuningDetailsCustomization>();
}

void FACBossTuningDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

	if (CustomizedObjects.Num() != 1)
	{
		return;
	}

	UACDataAsset_BossTuning* TuningData = Cast<UACDataAsset_BossTuning>(CustomizedObjects[0].Get());
	if (!TuningData)
	{
		return;
	}

	IDetailCategoryBuilder& PreviewCategory = DetailBuilder.EditCategory(
		TEXT("Boss Tuning Preview"),
		LOCTEXT("PreviewCategory", "Boss Tuning Preview"),
		ECategoryPriority::Important);

	PreviewCategory.InitiallyCollapsed(false);
	PreviewCategory.AddCustomRow(LOCTEXT("PreviewSearchText", "Distance Band Range Gap Preview"))
		.WholeRowContent()
		.MinDesiredWidth(620.f)
		[
			SNew(SBox)
			.HeightOverride(375.f)
			[
				SNew(SACBossTuningPreview)
				.TuningData(TuningData)
			]
		];
}

#undef LOCTEXT_NAMESPACE
