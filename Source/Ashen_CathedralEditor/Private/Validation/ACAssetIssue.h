// 에셋 검사기들이 공통으로 쓰는 결과 타입 — 워크벤치 UI와 자동화 테스트가 같은 데이터를 소비한다

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

/** 검사 결과의 심각도 */
enum class EACAssetIssueSeverity : uint8
{
	/** 확실히 잘못됐다. 자동화 테스트를 실패시킨다 */
	Error,

	/** 의도한 동작이 아닐 가능성이 높다 */
	Warning,

	/** 정상 확인 항목이거나 참고용 정보. 테스트 결과에 반영하지 않는다 */
	Info
};

/** 검사에서 발견된 항목 하나 */
struct FACAssetIssue
{
	EACAssetIssueSeverity Severity = EACAssetIssueSeverity::Warning;
	FString Message;
};

/** 에셋 하나에 대한 검사 결과 */
struct FACAssetReport
{
	FString AssetName;
	FSoftObjectPath AssetPath;

	/** 이름 옆에 함께 보여줄 부가 정보. 비어 있을 수 있다 */
	FString Subtitle;

	TArray<FACAssetIssue> Issues;

	void Add(const EACAssetIssueSeverity Severity, FString&& Message)
	{
		Issues.Add(FACAssetIssue{ Severity, MoveTemp(Message) });
	}

	int32 CountBySeverity(const EACAssetIssueSeverity Severity) const
	{
		int32 Count = 0;
		for (const FACAssetIssue& Issue : Issues)
		{
			Count += Issue.Severity == Severity ? 1 : 0;
		}
		return Count;
	}

	bool HasErrors() const
	{
		return CountBySeverity(EACAssetIssueSeverity::Error) > 0;
	}
};
