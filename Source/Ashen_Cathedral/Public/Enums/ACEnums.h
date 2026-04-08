// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EACAbilityActivationPolicy : uint8
{
	/* 기본값: 입력 또는 게임플레이 이벤트를 통해 트리거될 때 활성화 */
	OnTriggered,

	/* ASC에 부여(Grant)되는 즉시 자동으로 활성화 (예: 패시브, 1회성 스폰) */
	OnGiven
};

UENUM(BlueprintType)
enum class EToggleDamageType : uint8
{
	CurrentEquippedWeapon,
	LeftHand,
	RightHand
};

UENUM()
enum class EACValidType : uint8
{
	Valid,
	Invalid,
};

UENUM()
enum class EACConfirmType : uint8
{
	Yes,
	No
};


UENUM(BlueprintType)
enum class ERollDirection : uint8
{
	Forward UMETA(DisplayName = "전방 (Forward)"),
	ForwardRight UMETA(DisplayName = "전방 우측 (Forward Right)"),
	Right UMETA(DisplayName = "우측 (Right)"),
	BackwardRight UMETA(DisplayName = "후방 우측 (Backward Right)"),
	Backward UMETA(DisplayName = "후방 (Backward)"),
	BackwardLeft UMETA(DisplayName = "후방 좌측 (Backward Left)"),
	Left UMETA(DisplayName = "좌측 (Left)"),
	ForwardLeft UMETA(DisplayName = "전방 좌측 (Forward Left)")
};