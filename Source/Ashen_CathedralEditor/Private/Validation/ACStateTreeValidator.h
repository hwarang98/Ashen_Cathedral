// StateTree 에셋을 정적 분석해 런타임에 조용히 실패하는 배선 오류를 찾아내는 에디터 전용 검사기

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

/** 검사 결과의 심각도 */
enum class EACStateTreeIssueSeverity : uint8
{
	/** 확실히 동작하지 않는다 */
	Error,

	/** 의도한 동작이 아닐 가능성이 높다 */
	Warning,

	/** 다른 경로로 해결됐을 수 있어 눈으로 확인만 하면 되는 항목 */
	Info
};

/** StateTree 검사에서 발견된 문제 하나 */
struct FACStateTreeIssue
{
	EACStateTreeIssueSeverity Severity = EACStateTreeIssueSeverity::Warning;

	/** "Root > Combat > Defense > ReactDodge" 형태의 상태 경로 */
	FString StatePath;

	FString Message;
};

/** StateTree 에셋 하나에 대한 검사 결과 */
struct FACStateTreeReport
{
	FSoftObjectPath AssetPath;
	FString AssetName;
	int32 StateCount = 0;
	TArray<FACStateTreeIssue> Issues;

	/** 지정한 심각도의 문제 개수를 센다 */
	int32 CountBySeverity(EACStateTreeIssueSeverity Severity) const;
};

namespace ACStateTreeValidator
{
	/**
	 * @brief /Game 아래 모든 StateTree 에셋을 검사한다.
	 *
	 * @return 에셋 이름 순으로 정렬된 검사 결과. 문제가 없는 에셋도 포함된다.
	 * @note 태그 대조에 필요한 GameplayAbility CDO를 함께 스캔하므로 호출 시 에셋 로드가 발생한다.
	 *       비활성화(bEnabled=false)된 상태와 노드는 검사 대상에서 제외한다.
	 */
	TArray<FACStateTreeReport> ValidateAllStateTrees();
}
