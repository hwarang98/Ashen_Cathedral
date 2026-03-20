// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimInstance/ACAnimInstanceBase.h"
#include "ACPlayerAnimInstance.generated.h"

class AACPlayerCharacter;
class AACCharacterBase;
class UCharacterMovementComponent;

/**
 * @class UACPlayerAnimInstance
 * @brief 플레이어 캐릭터의 애니메이션 로직을 처리하는 애니메이션 인스턴스 클래스.
 *
 * UACAnimInstanceBase를 상속받아 구현되며, Ashen Cathedral 프로젝트의 플레이어 캐릭터와 관련된 애니메이션 로직을 정의하는 데 사용
 * 각종 상태 및 동작과 관련된 애니메이션 데이터를 관리하거나, 이를 기반으로 애니메이션 블루프린트 내의 다양한 동작을 처리할 수 있음
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerAnimInstance : public UACAnimInstanceBase
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	// ACPlayerCharacter::OnJumped_Implementation()에서 호출
	void OnOwnerJumped();

	#pragma region Blueprint API
	UFUNCTION(BlueprintCallable, meta=(BlueprintThreadSafe))
	FORCEINLINE bool GetIsJumping() const { return IsFalling; }
	#pragma endregion

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Character")
	TObjectPtr<AACPlayerCharacter> OwningPlayerCharacter;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	bool IsCrouching = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	bool IsSprinting = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	float MoveDirection = 0.f;

	// 점프 입력 직후 true, 착지 시 false — Jump Start 상태 진입용
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	bool bJumpTriggered = false;

	// 점프 순간 스냅샷 — 공중에서 방향이 바뀌어도 Jump 애니메이션은 이 값을 사용
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	float JumpDirection = 0.f;
};