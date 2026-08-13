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

// 플레이어 무기 교체 파이프라인의 진행 단계
UENUM(BlueprintType)
enum class EACWeaponSwapPhase : uint8
{
	Idle           UMETA(DisplayName = "대기"),
	WaitingUnequip UMETA(DisplayName = "해제 완료 대기"),
	Swapping       UMETA(DisplayName = "파괴/스폰 동기 구간"),
	WaitingEquip   UMETA(DisplayName = "장착 완료 대기"),
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

// Enemy 4방향 닷지 방향. GameplayEvent의 EventMagnitude로 전달되므로 값 순서를 바꾸지 말 것
UENUM(BlueprintType)
enum class EACDodgeDirection : uint8
{
	Forward  UMETA(DisplayName = "전방 (Forward)"),
	Backward UMETA(DisplayName = "후방 (Backward)"),
	Left     UMETA(DisplayName = "좌측 (Left)"),
	Right    UMETA(DisplayName = "우측 (Right)"),
};

// Enemy 공격 어빌리티가 AttackMontages 배열에서 몽타주를 고르는 방식
UENUM(BlueprintType)
enum class EACAttackMontageSelectionMode : uint8
{
	Random        UMETA(DisplayName = "무작위"),
	Sequential    UMETA(DisplayName = "순차 (배열 순서대로)"),
	ComboSequence UMETA(DisplayName = "콤보 (ComboSequences에서 한 벌 뽑아 순서대로)"),
};

// 로그라이크 보상 카드 계열
UENUM(BlueprintType)
enum class EACCardCategory : uint8
{
	Attack   UMETA(DisplayName = "공격"),
	Defense  UMETA(DisplayName = "방어"),
	Mobility UMETA(DisplayName = "기동"),
	Parry    UMETA(DisplayName = "패링/스킬"),
	Resource UMETA(DisplayName = "자원/회복"),
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

// 보스 클리어 후 스테이지 출구가 플레이어를 보낼 목적지
UENUM(BlueprintType)
enum class EACStageExitDestination : uint8
{
	NextStage      UMETA(DisplayName = "다음 스테이지 (카드·적립분 유지)"),
	ReturnToLobby  UMETA(DisplayName = "로비 복귀 (적립분 정산, 카드 소실)"),
};

// 스테이지 클리어 후 플레이어에게 허용할 진행 선택지의 범위. StageDefinition이 소유하며 출구 액터의 활성화를 제한한다
UENUM(BlueprintType)
enum class EACStageExitPolicy : uint8
{
	NormalChoice       UMETA(DisplayName = "선택 가능 (다음 스테이지 / 로비 복귀)"),
	ForceReturnToLobby UMETA(DisplayName = "로비 복귀만 (튜토리얼 등)"),
	AutoNextStage      UMETA(DisplayName = "선택 없이 다음 스테이지로"),
};

// GameState가 추적하는 보스 전투 상태. Phase 판정 등 세부 전투 로직은 포함하지 않음
UENUM(BlueprintType)
enum class EACBattleState : uint8
{
	Idle                 UMETA(DisplayName = "대기"),
	BossBattleInProgress UMETA(DisplayName = "보스전 진행 중"),
	BossDefeated         UMETA(DisplayName = "보스 격파"),
	Completed            UMETA(DisplayName = "전투 완료"),
};