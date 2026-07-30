// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace ACGameplayTags
{
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_FX_Block);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_FX_Parry);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_FX_SuccessfulBlock);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Sounds_Melee_Nodachi);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Sounds_Melee_Sword);

	// 혈흔 VFX 큐 — 공격 어빌리티의 BloodHitGameplayCueTag에 지정해서 사용한다
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_FX_Blood_Melee_Light);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_FX_Blood_Melee_Heavy);
	// 플레이어/적의 모든 카운터 공격이 공통으로 사용하는 혈흔 큐
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_FX_Blood_Melee_Counter);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_FX_Blood_Stab);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_FX_Blood_Death);
	ASHEN_CATHEDRAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_FX_Blood_Dripping);

}