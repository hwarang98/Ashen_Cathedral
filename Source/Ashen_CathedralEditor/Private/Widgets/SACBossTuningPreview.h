#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class UACDataAsset_BossTuning;

class SACBossTuningPreview : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SACBossTuningPreview)
	{
	}
		SLATE_ARGUMENT(TWeakObjectPtr<UACDataAsset_BossTuning>, TuningData)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	TWeakObjectPtr<UACDataAsset_BossTuning> TuningData;
};
