// JSON 프리셋 임포터가 공유하는 순수 데이터 타입.

#pragma once

#include "CoreMinimal.h"

class FJsonObject;

/**
 * @brief JSON에서 읽어들인 프리셋 한 건.
 * @note Settings는 파싱된 JSON 오브젝트를 그대로 들고 있다가 적용 시점에 리플렉션으로 매핑한다.
 */
struct FPPPresetEntry
{
	/** 프리셋 표시 이름 (JSON의 Preset 필드). */
	FString PresetName;

	/** 원본 패키지 경로 (JSON의 Package 필드). 없으면 비어 있다. */
	FString PackagePath;

	/** FPostProcessSettings로 매핑할 키/값 묶음. */
	TSharedPtr<FJsonObject> Settings;
};

/** 적용 계획의 개별 변경 항목. 미리보기와 실제 적용이 같은 경로를 공유한다. */
struct FPPPropertyChange
{
	FString PropertyName;
	FString OldValue;
	FString NewValue;
};

/**
 * @brief 하나의 프리셋을 대상 볼륨에 적용했을 때 벌어질 일의 전체 요약.
 * @note bApply=false로 만들면 미리보기, true면 실제 적용 결과가 된다. 내용은 동일하다.
 */
struct FPPApplyPlan
{
	/** 값이 실제로 달라지는 프로퍼티들. */
	TArray<FPPPropertyChange> Changes;

	/** JSON에는 있지만 현재 엔진의 FPostProcessSettings에 없는 프로퍼티 이름. */
	TArray<FString> UnknownProperties;

	/** JSON이 참조하지만 프로젝트에 존재하지 않는 에셋 경로. */
	TArray<FString> MissingAssetPaths;

	/** 타입이 맞지 않아 건너뛴 프로퍼티. */
	TArray<FString> SkippedProperties;

	/** 이 프리셋이 켜는 bOverride_* 플래그 이름. */
	TArray<FString> AppliedOverrides;

	void Reset()
	{
		Changes.Reset();
		UnknownProperties.Reset();
		MissingAssetPaths.Reset();
		SkippedProperties.Reset();
		AppliedOverrides.Reset();
	}

	bool HasWarnings() const
	{
		return UnknownProperties.Num() > 0 || MissingAssetPaths.Num() > 0 || SkippedProperties.Num() > 0;
	}
};
