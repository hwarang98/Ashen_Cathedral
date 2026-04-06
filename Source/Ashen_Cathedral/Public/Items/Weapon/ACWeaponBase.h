// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameFramework/Actor.h"
#include "ACWeaponBase.generated.h"

class UBoxComponent;
class UMeshComponent;

DECLARE_DELEGATE_OneParam(FonTargetInteractedDelegate, AActor*)

UCLASS()
class ASHEN_CATHEDRAL_API AACWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AACWeaponBase();
	virtual void BeginPlay() override;

	FonTargetInteractedDelegate OnWeaponHitTarget;
	FonTargetInteractedDelegate OnWeaponPulledFromTarget;

	virtual void AddGrantedGameplayEffect(FActiveGameplayEffectHandle Handle);

	/**
	 * 무기가 장착되면서 적용된 Gameplay Effect들의 핸들을 제거하고 반환합니다.
	 *
	 * 이 함수는 내부적으로 저장된 GrantedEffectHandles 배열의 모든 요소를 제거하고,
	 * 제거된 핸들들의 복사본을 호출자에게 반환합니다.
	 *
	 * @return 제거된 Gameplay Effect 핸들의 배열.
	 */
	virtual TArray<FActiveGameplayEffectHandle> RemoveGrantedGameplayEffects();

	// 스폰 직후 호출해서 무기 숨김
	void HideWeapon() const;

	// 장착 이벤트 시 호출해서 무기 보임
	void ShowWeapon() const;

	bool GetHideUntilEquipped() const { return bHideUntilEquipped; }

	FORCEINLINE UBoxComponent* GetWeaponCollisionBox() const { return WeaponCollisionBox; }
	FORCEINLINE UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	/**
	 * @brief Static 또는 Skeletal 메시 컴포넌트를 UMeshComponent*로 반환한다.
	 * Skeletal Mesh가 있으면 우선 반환하고, 없으면 Static Mesh를 반환한다.
	 * BP에서 USkeletalMeshComponent를 추가한 무기에서도 정상 동작한다.
	 */
	UMeshComponent* GetWeaponMeshComponent() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	TObjectPtr<UBoxComponent> WeaponCollisionBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons|Combat", meta = (AllowPrivateAccess = "true"))
	bool bIgnoreFriendly = true;

	// true인 경우, 무기가 숨겨지고 장착 이벤트 때 보여짐
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons|Visibility")
	bool bHideUntilEquipped = false;

	UFUNCTION()
	virtual void OnCollisionBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);

	UFUNCTION()
	virtual void OnCollisionBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 이 무기가 장착되면서 적용한 이펙트들의 핸들 목록
	TArray<FActiveGameplayEffectHandle> GrantedEffectHandles;
};