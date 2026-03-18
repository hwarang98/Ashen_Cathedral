// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimInstance/ACAnimInstanceBase.h"
#include "ACPlayerLinkedAnimLayer.generated.h"


class UACPlayerAnimInstance;

/**
 * @class UACPlayerLinkedAnimLayer
 * @brief 플레이어의 무기별 애니메이션 레이어를 관리하는 링크드 애님 인스턴스
 *
 * 이 클래스는 주 플레이어 애님 인스턴스('UCMPlayerAnimInstance') 위에 동적으로 연결(링크)되어 무기 장착 상태(예: 칼 장착)에 따른 고유한 상체 애니메이션 (Idle, 걷기, 공격 등)을 덮어쓰는 데 사용
 * 'UCMAnimInstanceBase'를 상속받으며, 'UCMPlayerAnimInstance'의 변수(예: 속도, 방향)에 접근하여 기본 Locomotion 상태와 동기화된 애니메이션을 재생합니다.
 * @see UCMPlayerAnimInstance
 * @see UCMAnimInstanceBase
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerLinkedAnimLayer : public UACAnimInstanceBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta = (blueprintThreadSafe))
	UACPlayerAnimInstance* GetPlayerAnimInstance() const;
};