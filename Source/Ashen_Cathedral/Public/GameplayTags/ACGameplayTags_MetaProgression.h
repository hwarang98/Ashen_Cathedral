// 메타 성장 시스템의 보스 식별 / 재화 GameplayTag 선언 — 전투 상태/이벤트 태그와 목적이 달라 별도 파일로 분리

#pragma once

#include "NativeGameplayTags.h"

namespace ACGameplayTags
{
	#pragma region MetaProgression BossID Tags
	// 보스 처치 시 첫 클리어 여부를 판정하는 세이브 키. UACDataAsset_BossReward::BossID에 할당해 사용한다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(MetaProgression_BossID_AshenKnight);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(MetaProgression_BossID_Ordan);
	#pragma endregion

	#pragma region MetaProgression Currency Tags
	// 메타 성장 재화 식별 태그. UACMetaProgressionSubsystem의 재화 API와 UACDataAsset_BossReward::RewardCurrency에 사용한다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(MetaProgression_Currency_AshSoul);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(MetaProgression_Currency_RelicFragment);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(MetaProgression_Currency_CathedralSigil);
	#pragma endregion
}