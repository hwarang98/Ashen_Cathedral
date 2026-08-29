// JSON 파일에서 프리셋 목록을 읽어들인다.

#pragma once

#include "CoreMinimal.h"
#include "PPPresetTypes.h"

/**
 * @brief 프리셋 JSON 파서.
 *
 * 아래 세 가지 최상위 형태를 모두 받아들인다.
 *   1) 배열      : [ { "Preset": "...", "Package": "...", "Settings": { ... } }, ... ]
 *   2) 래핑 객체 : { "Presets": [ 위와 동일 ] }
 *   3) 맵        : { "프리셋이름": { ... 설정 ... }, ... }
 */
class POSTPROCESSPRESETIMPORTEREDITOR_API FPPPresetJsonReader
{
public:
	/**
	 * @brief 파일을 읽어 프리셋 목록을 만든다.
	 * @param InFilePath 절대 경로
	 * @param OutPresets 성공 시 채워진다. 실패 시 비워진다.
	 * @param OutError   실패 사유
	 * @return 파싱 성공 여부
	 */
	static bool LoadFromFile(const FString& InFilePath, TArray<FPPPresetEntry>& OutPresets, FString& OutError);

	/** 문자열에서 직접 파싱한다. 자동화 테스트가 이 경로를 쓴다. */
	static bool LoadFromString(const FString& InJsonText, TArray<FPPPresetEntry>& OutPresets, FString& OutError);
};
