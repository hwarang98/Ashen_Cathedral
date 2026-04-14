// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerGameplayAbility.h"
#include "ACPlayerAbility_TargetLock.generated.h"

class UACWidgetBase;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACPlayerAbility_TargetLock : public UACPlayerGameplayAbility
{
	GENERATED_BODY()

public:
	UACPlayerAbility_TargetLock();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable)
	void OnTargetLockTick(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void SwitchTarget(const FGameplayTag& InSwitchDirectionTag);
	//
private:
	void TryLockOnTarget();
	void GetAvailableActorsToLock();
	AActor* GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors);
	void GetAvailableActorsAroundTarget(TArray<AActor*>& OutActorsOnLeft, TArray<AActor*>& OutActorsOnRight);
	void DrawTargetLockWidget();
	void SetTargetLockWidgetPosition();
	void InitTargetLockMovement();
	void InitTargetLockMappingContext();

	void CancelTargetLockAbility();
	void CleanUp();
	void ResetTargetLockMovement();
	void ResetTargetLockMappingContext();

	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float BoxTraceDistance = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	FVector TraceBoxSize = FVector(5000.f, 5000.f, 300.f);

	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TArray<TEnumAsByte<EObjectTypeQuery>> BoxTraceChannel;

	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	bool bShowPersistentDebugShape = false;

	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TSubclassOf<UACWidgetBase> TargetLockWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float TargetLockRotationInterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float TargetLockMaxWalkSpeed = 250.f;

	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TObjectPtr<UInputMappingContext> TargetLockMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float TargetLockCameraOffsetDistance = 20.f;

	// 위젯 수직 위치 오프셋 (양수: 위, 음수: 아래)
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float TargetLockWidgetVerticalOffset = 0.f;

	UPROPERTY()
	TArray<AActor*> AvailableActorsToLock;

	UPROPERTY()
	TObjectPtr<AActor> CurrentLockedActor;

	UPROPERTY()
	TObjectPtr<UACWidgetBase> DrawnTargetLockWidget;

	UPROPERTY()
	FVector2D TargetLockWidgetSize = FVector2D::ZeroVector;

	UPROPERTY()
	float CachedDefaultMaxWalkSpeed = 0.f;

	FActiveGameplayEffectHandle TargetLockSpeedEffectHandle;
};