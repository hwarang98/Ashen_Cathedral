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

// 로그라이크 보상 카드 계열
UENUM(BlueprintType)
enum class EACCardCategory : uint8
{
	Attack   UMETA(DisplayName = "공격"),
	Defense  UMETA(DisplayName = "방어"),
	Mobility UMETA(DisplayName = "기동"),
	Parry    UMETA(DisplayName = "패링"),
	Resource UMETA(DisplayName = "자원"),
};

// 로그라이크 보상 카드 희귀도
UENUM(BlueprintType)
enum class EACCardRarity : uint8
{
	Common    UMETA(DisplayName = "일반"),
	Uncommon  UMETA(DisplayName = "고급"),
	Rare      UMETA(DisplayName = "희귀"),
	Legendary UMETA(DisplayName = "전설"),
};