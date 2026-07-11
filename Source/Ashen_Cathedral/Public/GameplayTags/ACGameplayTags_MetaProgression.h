// 메타 성장(성흔 조각) 시스템의 보스 식별용 GameplayTag 선언 — 전투 상태/이벤트 태그와 목적이 달라 별도 파일로 분리

#pragma once

#include "NativeGameplayTags.h"

namespace ACGameplayTags
{
	#pragma region MetaProgression BossID Tags
	// 보스 처치 시 첫 클리어 여부를 판정하는 세이브 키. UACDataAsset_BossReward::BossID에 할당해 사용한다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(MetaProgression_BossID_AshenKnight);
	#pragma endregion
}
