// Posture/Guard 게이지의 자연 감소 GE와 유예 GE가 올바른 짝을 이루는지 검사하는 에디터 전용 검사기

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

/** Decay 짝 검사 결과의 심각도 */
enum class EACDecayIssueSeverity : uint8
{
	/** 감소나 유예 중 하나가 확실히 동작하지 않는다 */
	Error,

	/** 동작은 하지만 의도한 대로는 아닐 가능성이 높다 */
	Warning
};

/** Decay 짝 검사에서 발견된 문제 하나 */
struct FACDecayIssue
{
	EACDecayIssueSeverity Severity = EACDecayIssueSeverity::Warning;

	/** 문제가 발견된 Startup DataAsset 이름 */
	FString CharacterName;

	/** "Posture" 또는 "Guard" */
	FString GaugeName;

	FString Message;

	/** 문제의 원인이 된 에셋. 없으면 Startup DataAsset을 가리킨다 */
	FSoftObjectPath AssetPath;
};

namespace ACDecayPairValidator
{
	/**
	 * @brief /Game 아래 모든 Startup DataAsset의 Posture/Guard 감소·유예 GE 짝을 검사한다.
	 *
	 * @return 발견된 문제 목록. 해당 게이지를 아예 쓰지 않는 캐릭터는 검사 대상에서 제외된다.
	 * @note 초기화 GE의 CDO를 읽어야 하므로 호출 시 에셋 로드가 발생한다.
	 */
	TArray<FACDecayIssue> ValidateAll();
}
