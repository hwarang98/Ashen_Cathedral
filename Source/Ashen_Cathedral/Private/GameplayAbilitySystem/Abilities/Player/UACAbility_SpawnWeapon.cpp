// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Player/UACAbility_SpawnWeapon.h"

#include "Character/ACCharacterBase.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "Items/Weapon/ACWeaponBase.h"

UUACAbility_SpawnWeapon::UUACAbility_SpawnWeapon()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

void UUACAbility_SpawnWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 유효성 검사
	AACCharacterBase* OwnerCharacter = GetCMCharacterFromActorInfo();
	if (!OwnerCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UPawnCombatComponent* CombatComponent = OwnerCharacter->GetPawnCombatComponent();
	if (!CombatComponent || !WeaponClassToSpawn || !WeaponTagToRegister.IsValid() || InitialSocketName == NAME_None)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1.5. 이미 등록된 무기인지 확인
	if (CombatComponent->GetCharacterCarriedWeaponByTag(WeaponTagToRegister))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false); // 이미 있으므로 취소
		return;
	}

	// 2. 스폰 파라미터 설정
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 3. 무기 스폰 (공용 로직)
	if (AACWeaponBase* SpawnedWeapon = GetWorld()->SpawnActor<AACWeaponBase>(WeaponClassToSpawn, SpawnParams))
	{
		// 4. [중요] 스폰된 무기를 PawnCombatComponent에 등록
		// bRegisterAsEquippedWeapon 값에 따라 즉시 장착 또는 등록만 처리
		CombatComponent->RegisterSpawnedWeapon(WeaponTagToRegister, SpawnedWeapon, bRegisterAsEquippedWeapon);

		// 5. 초기 소켓(등)에 부착 (공용 로직)
		SpawnedWeapon->AttachToComponent(
			OwnerCharacter->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			InitialSocketName
			);

		// 6. 무기 숨기기 (장착 전까지 보이지 않도록)
		if (!bRegisterAsEquippedWeapon)
		{
			SpawnedWeapon->HideWeapon();
		}
	}

	// 7. 어빌리티 즉시 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}