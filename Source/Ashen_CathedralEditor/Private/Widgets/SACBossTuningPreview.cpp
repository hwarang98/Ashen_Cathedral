#include "Widgets/SACBossTuningPreview.h"

#include "DataAssets/AI/ACDataAsset_BossTuning.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"

namespace ACBossTuningPreview
{
	constexpr float LabelWidth = 116.f;
	constexpr float OuterPadding = 8.f;
	constexpr float TitleHeight = 24.f;
	constexpr float RulerHeight = 30.f;
	constexpr float RowHeight = 28.f;
	constexpr float BarHeight = 16.f;
	constexpr int32 RowCount = 8;

	const FLinearColor BackgroundColor(0.025f, 0.03f, 0.04f, 1.f);
	const FLinearColor TrackColor(0.085f, 0.095f, 0.11f, 1.f);
	const FLinearColor GridColor(0.22f, 0.24f, 0.28f, 0.55f);
	const FLinearColor TextColor(0.82f, 0.84f, 0.88f, 1.f);
	const FLinearColor MeleeColor(0.22f, 0.70f, 0.35f, 1.f);
	const FLinearColor RunAttackColor(0.95f, 0.52f, 0.12f, 1.f);
	const FLinearColor StrafeColor(0.18f, 0.55f, 0.92f, 0.9f);
	const FLinearColor ChaseColor(0.83f, 0.24f, 0.20f, 1.f);
	const FLinearColor DefenseColor(0.14f, 0.72f, 0.78f, 1.f);
	const FLinearColor BackDodgeColor(0.62f, 0.34f, 0.86f, 1.f);
	const FLinearColor GapColor(0.95f, 0.18f, 0.12f, 0.9f);
	const FLinearColor WarningColor(1.f, 0.63f, 0.12f, 1.f);
	const FLinearColor ErrorColor(1.f, 0.22f, 0.16f, 1.f);
	const FLinearColor SuccessColor(0.34f, 0.82f, 0.42f, 1.f);
}

void SACBossTuningPreview::Construct(const FArguments& InArgs)
{
	TuningData = InArgs._TuningData;
}

FVector2D SACBossTuningPreview::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(620.f, 375.f);
}

int32 SACBossTuningPreview::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	using namespace ACBossTuningPreview;

	const UACDataAsset_BossTuning* Data = TuningData.Get();
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FSlateBrush* WhiteBrush = FAppStyle::GetBrush(TEXT("WhiteBrush"));

	auto DrawBox = [&](const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color, int32 DrawLayer)
	{
		if (Size.X <= 0.f || Size.Y <= 0.f)
		{
			return;
		}

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			DrawLayer,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
			WhiteBrush,
			ESlateDrawEffect::None,
			Color);
	};

	auto DrawText = [&](const FVector2D& Position, const FString& Text, const FSlateFontInfo& Font, const FLinearColor& Color, int32 DrawLayer)
	{
		FSlateDrawElement::MakeText(
			OutDrawElements,
			DrawLayer,
			AllottedGeometry.ToOffsetPaintGeometry(Position),
			Text,
			Font,
			ESlateDrawEffect::None,
			Color);
	};

	DrawBox(FVector2D::ZeroVector, LocalSize, BackgroundColor, LayerId);

	FSlateFontInfo NormalFont = FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"));
	FSlateFontInfo SmallFont = NormalFont;
	SmallFont.Size = 8;
	FSlateFontInfo TitleFont = FAppStyle::GetFontStyle(TEXT("BoldFont"));
	TitleFont.Size = 11;

	if (!Data)
	{
		DrawText(FVector2D(OuterPadding, OuterPadding), TEXT("보스 튜닝 데이터가 선택되지 않았습니다."), NormalFont, WarningColor, LayerId + 1);
		return LayerId + 1;
	}

	float LargestDistance = 0.f;
	LargestDistance = FMath::Max(LargestDistance, Data->ChaseDistance);
	LargestDistance = FMath::Max(LargestDistance, Data->StrafeDistance);
	LargestDistance = FMath::Max(LargestDistance, Data->MeleeDistance);
	LargestDistance = FMath::Max(LargestDistance, Data->RunAttackMaxDistance);
	LargestDistance = FMath::Max(LargestDistance, Data->DefenseDistance);
	LargestDistance = FMath::Max(LargestDistance, Data->BackDodgeDistance);
	LargestDistance = FMath::Max(LargestDistance, Data->BackDashAttackMaxDistance);

	const float PreviewMaxDistance = FMath::Max(1000.f, FMath::CeilToFloat(LargestDistance / 100.f) * 100.f + 100.f);
	const float TrackLeft = LabelWidth + OuterPadding;
	const float TrackRight = FMath::Max(TrackLeft + 1.f, LocalSize.X - OuterPadding);
	const float TrackWidth = TrackRight - TrackLeft;

	auto DistanceToX = [&](float Distance)
	{
		return TrackLeft + FMath::Clamp(Distance / PreviewMaxDistance, 0.f, 1.f) * TrackWidth;
	};

	DrawText(
		FVector2D(OuterPadding, 4.f),
		FString::Printf(TEXT("Boss Distance Bands - %s"), *Data->GetName()),
		TitleFont,
		TextColor,
		LayerId + 1);

	const float RulerY = TitleHeight;
	TArray<FVector2D> AxisPoints;
	AxisPoints.Add(FVector2D(TrackLeft, RulerY + 6.f));
	AxisPoints.Add(FVector2D(TrackRight, RulerY + 6.f));
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(),
		AxisPoints,
		ESlateDrawEffect::None,
		GridColor,
		true,
		1.f);

	for (float TickDistance = 0.f; TickDistance <= PreviewMaxDistance; TickDistance += 100.f)
	{
		const float TickX = DistanceToX(TickDistance);
		TArray<FVector2D> TickPoints;
		TickPoints.Add(FVector2D(TickX, RulerY + 3.f));
		TickPoints.Add(FVector2D(TickX, RulerY + 10.f));
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			TickPoints,
			ESlateDrawEffect::None,
			GridColor,
			true,
			1.f);

		const float TextOffset = TickDistance <= 0.f ? 0.f : 11.f;
		DrawText(
			FVector2D(TickX - TextOffset, RulerY + 11.f),
			FString::Printf(TEXT("%.0f"), TickDistance),
			SmallFont,
			TextColor,
			LayerId + 2);
	}

	auto DrawRange = [&](const TCHAR* Label, float Minimum, float Maximum, const FLinearColor& Color, int32 RowIndex)
	{
		const float RowY = TitleHeight + RulerHeight + RowIndex * RowHeight;
		const float BarY = RowY + (RowHeight - BarHeight) * 0.5f;
		DrawText(FVector2D(OuterPadding, RowY + 6.f), Label, NormalFont, TextColor, LayerId + 2);
		DrawBox(FVector2D(TrackLeft, BarY), FVector2D(TrackWidth, BarHeight), TrackColor, LayerId + 1);

		const float RangeStart = DistanceToX(FMath::Min(Minimum, Maximum));
		const float RangeEnd = DistanceToX(FMath::Max(Minimum, Maximum));
		const FLinearColor DrawColor = Minimum <= Maximum ? Color : ErrorColor;
		DrawBox(FVector2D(RangeStart, BarY), FVector2D(FMath::Max(2.f, RangeEnd - RangeStart), BarHeight), DrawColor, LayerId + 2);

		const FString RangeText = FString::Printf(TEXT("%.0f - %.0f cm"), Minimum, Maximum);
		DrawText(FVector2D(RangeStart + 4.f, BarY + 2.f), RangeText, SmallFont, FLinearColor::White, LayerId + 3);
	};

	DrawRange(TEXT("Melee Attack"), 0.f, Data->MeleeDistance, MeleeColor, 0);
	DrawRange(TEXT("Run Attack"), Data->RunAttackMinDistance, Data->RunAttackMaxDistance, RunAttackColor, 1);
	DrawRange(TEXT("Strafe Zone"), 0.f, Data->StrafeDistance, StrafeColor, 2);
	DrawRange(TEXT("Chase"), Data->ChaseDistance, PreviewMaxDistance, ChaseColor, 3);
	DrawRange(TEXT("Defense"), 0.f, Data->DefenseDistance, DefenseColor, 4);
	DrawRange(TEXT("Back Dodge"), 0.f, Data->BackDodgeDistance, BackDodgeColor, 5);
	DrawRange(TEXT("Back Dash Attack"), Data->BackDashAttackMinDistance, Data->BackDashAttackMaxDistance, BackDodgeColor, 6);

	const float GapRowY = TitleHeight + RulerHeight + 7 * RowHeight;
	const float GapBarY = GapRowY + (RowHeight - BarHeight) * 0.5f;
	DrawText(FVector2D(OuterPadding, GapRowY + 6.f), TEXT("공격 공백"), NormalFont, TextColor, LayerId + 2);
	DrawBox(FVector2D(TrackLeft, GapBarY), FVector2D(TrackWidth, BarHeight), TrackColor, LayerId + 1);

	const bool bHasMeleeToRunGap = Data->MeleeDistance < Data->RunAttackMinDistance;
	const bool bHasRunToChaseGap = Data->RunAttackMaxDistance < Data->ChaseDistance;

	if (bHasMeleeToRunGap)
	{
		const float GapStart = DistanceToX(Data->MeleeDistance);
		const float GapEnd = DistanceToX(Data->RunAttackMinDistance);
		DrawBox(FVector2D(GapStart, GapBarY), FVector2D(GapEnd - GapStart, BarHeight), GapColor, LayerId + 2);
	}

	if (bHasRunToChaseGap)
	{
		const float GapStart = DistanceToX(Data->RunAttackMaxDistance);
		const float GapEnd = DistanceToX(Data->ChaseDistance);
		DrawBox(FVector2D(GapStart, GapBarY), FVector2D(GapEnd - GapStart, BarHeight), GapColor, LayerId + 2);
	}

	TArray<TPair<FString, FLinearColor>> Messages;
	if (Data->RunAttackMinDistance > Data->RunAttackMaxDistance)
	{
		Messages.Emplace(TEXT("오류: 돌진 공격 최소 거리가 최대 거리보다 큽니다."), ErrorColor);
	}
	else
	{
		if (bHasMeleeToRunGap)
		{
			Messages.Emplace(
				FString::Printf(
					TEXT("주의: %.0f~%.0fcm 구간에는 직접 공격 후보가 없습니다. (공백 %.0fcm)"),
					Data->MeleeDistance,
					Data->RunAttackMinDistance,
					Data->RunAttackMinDistance - Data->MeleeDistance),
				WarningColor);
		}

		if (bHasRunToChaseGap)
		{
			Messages.Emplace(
				FString::Printf(
					TEXT("주의: %.0f~%.0fcm 구간에는 직접 공격 후보가 없습니다. (공백 %.0fcm)"),
					Data->RunAttackMaxDistance,
					Data->ChaseDistance,
					Data->ChaseDistance - Data->RunAttackMaxDistance),
				WarningColor);
		}
	}

	if (Data->BackDashAttackMinDistance > Data->BackDashAttackMaxDistance)
	{
		Messages.Emplace(TEXT("오류: 백대시 공격 최소 거리가 최대 거리보다 큽니다."), ErrorColor);
	}

	if (Messages.IsEmpty())
	{
		Messages.Emplace(TEXT("거리 공백 또는 잘못된 최소/최대 설정이 없습니다."), SuccessColor);
	}

	const float MessageStartY = TitleHeight + RulerHeight + RowCount * RowHeight + 8.f;
	for (int32 MessageIndex = 0; MessageIndex < Messages.Num() && MessageIndex < 3; ++MessageIndex)
	{
		DrawText(
			FVector2D(OuterPadding, MessageStartY + MessageIndex * 17.f),
			Messages[MessageIndex].Key,
			SmallFont,
			Messages[MessageIndex].Value,
			LayerId + 3);
	}

	return LayerId + 3;
}
