// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/HitResult.h"
#include "GameFramework/Actor.h"
#include "Structs/ACStructTypes.h"
#include "ACWeaponBase.generated.h"

class UBoxComponent;
class UMeshComponent;
class UNiagaraSystem;

// 적중 시 실제 충돌 지점(ImpactPoint/Normal/피격 컴포넌트/BoneName)까지 함께 전달한다
DECLARE_DELEGATE_TwoParams(FOnWeaponHitTargetDelegate, AActor*, const FHitResult&)
DECLARE_DELEGATE_OneParam(FOnWeaponPulledFromTargetDelegate, AActor*)

UCLASS()
class ASHEN_CATHEDRAL_API AACWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AACWeaponBase();
	virtual void BeginPlay() override;

	FOnWeaponHitTargetDelegate OnWeaponHitTarget;
	FOnWeaponPulledFromTargetDelegate OnWeaponPulledFromTarget;

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
	UFUNCTION(BlueprintCallable, Category = "Weapons|Visibility")
	void HideWeapon() const;

	// 장착 이벤트 시 호출해서 무기 보임
	UFUNCTION(BlueprintCallable, Category = "Weapons|Visibility")
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

	/** SocketName에 해당하는 Trail 오버라이드 이펙트를 반환한다. 등록된 게 없으면 nullptr. */
	UNiagaraSystem* GetTrailEffectOverride(FName SocketName) const;

	// 공격 애니메이션의 무기 Trail 노티파이가 소켓별로 재생할 이펙트를 런타임에 덮어씀 (예: 보스 페이즈 전환)
	void SetTrailEffectOverrides(const TArray<FACPhase2NiagaraAttachment>& NewOverrides) { TrailEffectOverrides = NewOverrides; }

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

	/**
	 * @brief 적중 델리게이트로 넘길 FHitResult를 확정한다.
	 *
	 * SweepResult가 실제 충돌 정보를 담고 있으면 그대로 사용하고, 그렇지 않으면 피격 컴포넌트 표면에서
	 * 접촉점/법선/BoneName을 추정해 채운다.
	 *
	 * @param OtherActor  적중된 대상 액터
	 * @param OtherComp   적중된 대상 컴포넌트. nullptr이면 표면 계산을 건너뛴다.
	 * @param bFromSweep  오버랩 이벤트가 스윕에서 발생했는지 여부
	 * @param SweepResult 오버랩 이벤트가 전달한 원본 히트 결과
	 * @return 항상 유효한(NaN 없는) ImpactPoint/ImpactNormal을 가진 HitResult
	 * @note 표면 접촉점을 구하지 못하면 최종적으로 대상 액터 위치를 사용한다.
	 */
	FHitResult ResolveWeaponHitResult(AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult) const;

	// 이 무기가 장착되면서 적용한 이펙트들의 핸들 목록
	TArray<FActiveGameplayEffectHandle> GrantedEffectHandles;

	// 무기 Trail 노티파이가 기본값 대신 소켓별로 재생할 이펙트 목록. 일치하는 소켓이 없으면 노티파이의 기본 이펙트를 사용
	UPROPERTY()
	TArray<FACPhase2NiagaraAttachment> TrailEffectOverrides;
};