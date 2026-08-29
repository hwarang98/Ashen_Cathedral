// FPostProcessSettings에 JSON 값을 리플렉션으로 매핑한다.

#pragma once

#include "CoreMinimal.h"
#include "PPPresetTypes.h"

class APostProcessVolume;
class UPostProcessComponent;
struct FPostProcessSettings;

/**
 * @brief 프리셋 → FPostProcessSettings 매핑기.
 *
 * 하드코딩된 프로퍼티 목록을 두지 않는다. FPostProcessSettings::StaticStruct()를 이름으로 조회해
 * 현재 엔진에 실제로 존재하는 프로퍼티만 처리하므로, 엔진 버전이 올라가도 코드 수정이 필요 없다.
 */
class POSTPROCESSPRESETIMPORTEREDITOR_API FPPPresetApplier
{
public:
	/**
	 * @brief 적용하지 않고 무엇이 바뀔지만 계산한다.
	 * @param InPreset  적용할 프리셋
	 * @param InCurrent 현재 볼륨의 설정 (비교 기준)
	 * @param OutPlan   변경 목록과 경고가 채워진다
	 */
	static void BuildPlan(const FPPPresetEntry& InPreset, const FPostProcessSettings& InCurrent, FPPApplyPlan& OutPlan);

	/**
	 * @brief 프리셋을 볼륨에 적용한다. Undo/Redo가 가능하도록 트랜잭션과 Modify를 처리한다.
	 * @return 적용 성공 여부. 볼륨이 null이면 false.
	 */
	static bool ApplyToVolume(const FPPPresetEntry& InPreset, APostProcessVolume* InVolume, FPPApplyPlan& OutPlan);

	/**
	 * @brief 블루프린트 안의 PostProcessComponent에 적용한다. 볼륨과 동일하게 트랜잭션 처리된다.
	 */
	static bool ApplyToComponent(const FPPPresetEntry& InPreset, class UPostProcessComponent* InComponent, FPPApplyPlan& OutPlan);

	/**
	 * @brief 설정 구조체에 직접 적용한다. DataAsset 생성 경로가 이걸 쓴다.
	 */
	static void ApplyToSettings(const FPPPresetEntry& InPreset, FPostProcessSettings& InOutSettings, FPPApplyPlan& OutPlan);
};
