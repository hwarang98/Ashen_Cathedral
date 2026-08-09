// Enemy Blueprint의 보스 필수 에셋(StartupData / Reward / AIController / Phase 전환) 배선을 검사하는 에디터 전용 검사기

#pragma once

#include "CoreMinimal.h"
#include "Validation/ACAssetIssue.h"

namespace ACBossAssetValidator
{
	/**
	 * @brief /Game/Enemy 아래 보스로 판정되는 Enemy Blueprint를 찾아 필수 배선을 검사한다.
	 *
	 * @return 보스 이름 순으로 정렬된 검사 결과. 정상 항목도 Info로 함께 담긴다.
	 * @note 보스 판정은 BossIdentityTag / BossRewardData / BossPhaseComponent 중 하나라도 있는 경우다.
	 *       에셋 레지스트리 태그로 네이티브 부모를 먼저 걸러 필요한 Blueprint만 로드한다.
	 */
	TArray<FACAssetReport> ValidateAll();
}
