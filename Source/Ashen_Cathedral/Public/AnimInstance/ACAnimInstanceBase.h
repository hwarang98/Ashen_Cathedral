// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ACAnimInstanceBase.generated.h"

struct FGameplayTag;
class UCharacterMovementComponent;
class AACCharacterBase;

/**
 * @class UACAnimInstanceBase
 * @brief 애니메이션 인스턴스의 기본 클래스.
 *
 * UAnimInstance를 상속받은 Ashen Cathedral 프로젝트의 기본 애니메이션 인스턴스 클래스
 * 이 클래스는 캐릭터의 상태 기반 애니메이션 기능을 확장하거나 커스터마이즈하는 데 사용
 * UACPlayerAnimInstance 및 UACPlayerLinkedAnimLayer와 같은 플레이어 관련 애니메이션 클래스의 기본 클래스 역할
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UFUNCTION(BlueprintPure, Category = "AnimData|Character", meta = (BlueprintThreadSafe))
	bool DoesOwnerHaveTag(const FGameplayTag TagToCheck) const;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Character")
	TObjectPtr<AACCharacterBase> OwningCharacter;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Character")
	TObjectPtr<UCharacterMovementComponent> OwningMovementComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	float GroundSpeed = 0.f;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	bool IsFalling = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	bool bHasAcceleration = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Locomotion")
	float LocomotionDirection;
};