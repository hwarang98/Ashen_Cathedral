// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace ACGameplayTags
{
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Event_Combat_IncomingAttack) // 공격 몽타주의 예고 Notify가 발송하는 이벤트. 패링/블록 성공을 의미하지 않으며, Boss AI에게 반응 판단 기회만 제공한다.

	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Attack_Blockable)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Attack_Parryable)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Attack_Unblockable)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Attack_Unparryable)
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Attack_Weight_Heavy) // 피격자가 대형 히트리액트를 재생하게 하는 강타 표식. Shared.Attack 하위라 예고 Notify의 태그 필터를 그대로 통과한다.

	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_Status_Stagger) // 패링 성공으로 부여되는 짧은 경직 상태. Posture 붕괴(PostureBroken)와는 별개의 즉발성 락아웃이다.
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Shared_SetByCaller_StaggerDuration)
}
