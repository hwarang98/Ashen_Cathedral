// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Combat/PawnCombatComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AnimInstance/Player/ACPlayerLinkedAnimLayer.h"
#include "Character/ACCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "DataAssets/Items/Weapon/ACDataAsset_WeaponData.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameplayTags/ACGameplayTags_Shared.h"
#include "Items/Weapon/ACWeapon.h"
#include "Structs/ACStructTypes.h"
#include "Components/BoxComponent.h"

void UPawnCombatComponent::OnHitTargetActor(AActor* HitActor)
{
	// 공격때마다 1회만 공격처리 (중복 방지)
	if (OverlappedActors.Contains(HitActor))
	{
		return;
	}

	OverlappedActors.AddUnique(HitActor);

	// GAS 이벤트 전송 (공통 로직)
	FGameplayEventData EventData;
	EventData.Instigator = GetOwningPawn();
	EventData.Target = HitActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		GetOwningPawn(),
		ACGameplayTags::Shared_Event_MeleeHit,
		EventData
		);

	// 자식 클래스의 추가 로직 실행
	OnHitTargetActorImpl(HitActor);
}

void UPawnCombatComponent::OnWeaponPulledFromTargetActor(AActor* InteractingActor) {}

void UPawnCombatComponent::OnHitTargetActorImpl(AActor* HitActor)
{
	// 기본 구현은 비어있음 - 자식 클래스에서 필요시 override
}

AACWeaponBase* UPawnCombatComponent::GetCharacterCarriedWeaponByTag(FGameplayTag InWeaponTagToGet) const
{
	for (const FWeaponEntry& Entry : CharacterCarriedWeaponList)
	{
		if (Entry.WeaponTag == InWeaponTagToGet)
		{
			return Entry.WeaponActor;
		}
	}

	return nullptr;
}

void UPawnCombatComponent::RegisterSpawnedWeapon(FGameplayTag InWeaponTagToResister, AACWeaponBase* InWeaponToResister, bool bResisterAsEquippedWeapon)
{
	checkf(GetCharacterCarriedWeaponByTag(InWeaponTagToResister) == nullptr, TEXT("%s는(은) 이미 장착된 무기 목록에 포함되어 있습니다"), *InWeaponTagToResister.ToString());

	check(InWeaponToResister);

	FWeaponEntry ReplicatedWeaponEntry;
	ReplicatedWeaponEntry.WeaponTag = InWeaponTagToResister;
	ReplicatedWeaponEntry.WeaponActor = InWeaponToResister;
	CharacterCarriedWeaponList.Add(ReplicatedWeaponEntry);

	InWeaponToResister->OnWeaponHitTarget.BindUObject(this, &ThisClass::OnHitTargetActor);
	InWeaponToResister->OnWeaponPulledFromTarget.BindUObject(this, &ThisClass::OnWeaponPulledFromTargetActor);

	if (bResisterAsEquippedWeapon)
	{
		CurrentEquippedWeaponTag = InWeaponTagToResister;
	}
}

void UPawnCombatComponent::SetCurrentEquippedWeaponTag(const FGameplayTag& NewWeaponTag)
{

	const FGameplayTag OldWeaponTag = CurrentEquippedWeaponTag;
	if (OldWeaponTag == NewWeaponTag)
	{
		return;
	}

	// 1. 서버에서 태그 값을 변경 (이 값은 원격 클라이언트로 복제됨)
	CurrentEquippedWeaponTag = NewWeaponTag;

	//2. 서버(Listen)에서 즉시 시각적/물리적 상태를 적용
	HandleEquipEffects(CurrentEquippedWeaponTag, OldWeaponTag);

	// 3. 원격 클라이언트들은 OnRep_CurrentEquippedWeaponTag를 통해 HandleClientSideEquipEffects가 정상적으로 호출
}

AACWeaponBase* UPawnCombatComponent::GetCharacterCurrentEquippedWeapon() const
{
	if (!CurrentEquippedWeaponTag.IsValid())
	{
		return nullptr;
	}

	return GetCharacterCarriedWeaponByTag(CurrentEquippedWeaponTag);
}

AACCharacterBase* UPawnCombatComponent::GetOwnerCharacter() const
{
	if (AACCharacterBase* OwningCharacter = Cast<AACCharacterBase>(GetOwner()))
	{
		return OwningCharacter;
	}

	return nullptr;
}

void UPawnCombatComponent::ToggleWeaponCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType)
{
	if (ToggleDamageType == EToggleDamageType::CurrentEquippedWeapon)
	{
		const AACWeaponBase* WeaponToToggle = GetCharacterCurrentEquippedWeapon();

		if (!WeaponToToggle)
		{
			return;
		}

		WeaponToToggle->GetWeaponCollisionBox()->SetCollisionEnabled(bShouldEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

		if (!bShouldEnable)
		{
			OverlappedActors.Empty();
		}
	}
}

void UPawnCombatComponent::HandleToggleCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType)
{
	if (ToggleDamageType == EToggleDamageType::CurrentEquippedWeapon)
	{
		const AACWeaponBase* WeaponToToggle = GetCharacterCurrentEquippedWeapon();

		if (!WeaponToToggle)
		{
			return;
		}

		WeaponToToggle->GetWeaponCollisionBox()->SetCollisionEnabled(bShouldEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

		if (!bShouldEnable)
		{
			OverlappedActors.Empty();
		}
	}
}

void UPawnCombatComponent::HandleEquipEffects(const FGameplayTag& NewWeaponTag, const FGameplayTag& OldWeaponTag)
{
	const AACCharacterBase* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->GetMesh())
	{
		return;
	}

	if (OldWeaponTag.IsValid())
	{
		if (const AACWeapon* OldWeapon = Cast<AACWeapon>(GetCharacterCarriedWeaponByTag(OldWeaponTag)))
		{
			if (const UACDataAsset_WeaponData* WeaponData = OldWeapon->WeaponData)
			{
				// if (UPawnUIComponent* PawnUIComp = OwnerCharacter->GetPawnUIComponent())
				// {
				// 	for (const FCMPlayerAbilitySet& AbilitySet : WeaponData->DefaultWeaponAbilities)
				// 	{
				// 		PawnUIComp->RemoveAbilityIcon(AbilitySet.SlotTag);
				// 	}
				// }
			}
		}
	}

	// 1. 이전 무기가 있었다면, 클라이언트 효과(입력, 애님)를 제거
	if (OldWeaponTag.IsValid())
	{
		if (AACWeapon* OldWeapon = Cast<AACWeapon>(GetCharacterCarriedWeaponByTag(OldWeaponTag)))
		{
			const UACDataAsset_WeaponData* WeaponData = OldWeapon->WeaponData;

			// 애님 레이어 해제
			if (WeaponData->WeaponAnimLayerToLink)
			{
				OwnerCharacter->GetMesh()->UnlinkAnimClassLayers(WeaponData->WeaponAnimLayerToLink.Get());
			}

			// [로컬 플레이어 전용] 입력 컨텍스트 제거
			if (OwnerCharacter->IsLocallyControlled())
			{
				if (const APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController()))
				{
					if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
					{
						if (WeaponData->WeaponInputMappingContext)
						{
							Subsystem->RemoveMappingContext(WeaponData->WeaponInputMappingContext);
						}
					}
				}
			}

			// 무기 장착 제거
			if (WeaponData->UnequippedSocketName != NAME_None)
			{
				OldWeapon->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponData->UnequippedSocketName);
			}

			// 무기 숨기기
			OldWeapon->HideWeapon();
		}
	}

	// 2. 새 무기가 있다면, 클라이언트 효과(입력, 애님)를 적용
	if (NewWeaponTag.IsValid())
	{
		if (AACWeapon* NewWeapon = Cast<AACWeapon>(GetCharacterCarriedWeaponByTag(NewWeaponTag)))
		{
			const UACDataAsset_WeaponData* WeaponData = NewWeapon->WeaponData;

			// 애님 레이어 연결
			if (WeaponData->WeaponAnimLayerToLink)
			{
				OwnerCharacter->GetMesh()->LinkAnimClassLayers(WeaponData->WeaponAnimLayerToLink);
			}

			// [로컬 플레이어 전용] 입력 컨텍스트 추가
			if (OwnerCharacter->IsLocallyControlled())
			{
				if (const APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController()))
				{
					if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
					{
						if (WeaponData->WeaponInputMappingContext)
						{
							Subsystem->AddMappingContext(WeaponData->WeaponInputMappingContext, 1);
						}
					}
				}
			}

			// 무기를 Equipped 소켓(손)에 부착
			if (WeaponData->EquippedSocketName != NAME_None)
			{
				NewWeapon->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponData->EquippedSocketName);
			}

			// 무기 보이기
			NewWeapon->ShowWeapon();

			// 스킬 파티클 시스템 프라이밍 (첫 사용 시 프리징 방지)
			PreloadSkillParticles(WeaponData);
		}
	}
}

float UPawnCombatComponent::GetCurrentWeaponBaseDamage() const { return 0.f; }
float UPawnCombatComponent::GetCurrentWeaponHeavyAttackGroggyDamage() const { return 0.f; }
float UPawnCombatComponent::GetCurrentWeaponCounterAttackGroggyDamage() const { return 0.f; }
float UPawnCombatComponent::GetCurrentWeaponAttackSpeed() const { return 1.f; }

void UPawnCombatComponent::PreloadSkillParticles(const UACDataAsset_WeaponData* WeaponData)
{
	if (!WeaponData || WeaponData->SkillParticleSystems.IsEmpty())
	{
		return;
	}

	// 각 파티클을 화면 밖에서 매우 작은 스케일로 스폰하여 메모리에 로드
	for (const TSoftObjectPtr<UNiagaraSystem>& ParticlePtr : WeaponData->SkillParticleSystems)
	{
		if (UNiagaraSystem* ParticleSystem = ParticlePtr.LoadSynchronous())
		{
			// 화면 밖 위치에 거의 보이지 않는 크기로 스폰
			UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				ParticleSystem,
				FVector(0, 0, -10000), // 화면 밖
				FRotator::ZeroRotator,
				FVector(0.001f), // 거의 보이지 않는 크기
				true,            // Auto Destroy
				true,            // Auto Activate
				ENCPoolMethod::None,
				false // Preculling
				);

			// 즉시 비활성화하여 메모리에만 로드된 상태 유지
			if (NiagaraComponent)
			{
				NiagaraComponent->DeactivateImmediate();
			}
		}
	}
}