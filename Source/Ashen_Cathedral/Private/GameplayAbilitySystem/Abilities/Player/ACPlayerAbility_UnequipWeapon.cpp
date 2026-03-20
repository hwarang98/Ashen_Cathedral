// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_UnequipWeapon.h"
#include "ACGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Items/Weapon/ACWeapon.h"

UACPlayerAbility_UnequipWeapon::UACPlayerAbility_UnequipWeapon()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Player_Ability_UnEquipWeapon);
	SetAssetTags(TagsToAdd);

	// 장착 중에는 장착/해제 어빌리티 동시 실행 방지
	ActivationBlockedTags.AddTag(ACGameplayTags::Player_Status_Equipping);
	ActivationOwnedTags.AddTag(ACGameplayTags::Player_Status_Equipping);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	UnequipEventTag = ACGameplayTags::Player_Event_UnequipWeapon;
}

bool UACPlayerAbility_UnequipWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AACPlayerCharacter* OwnerCharacter = GetPlayerCharacterFromActorInfo();
	if (!OwnerCharacter)
	{
		return false;
	}

	const UPawnCombatComponent* CombatComponent = GetPlayerCombatComponentFromActorInfo();
	if (!CombatComponent)
	{
		return false;
	}

	// 무기를 들고 있어야 활성화 가능
	return CombatComponent->GetCharacterCurrentEquippedWeapon() != nullptr;
}

void UACPlayerAbility_UnequipWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 몽타주가 없으면, 로직만 즉시 실행하고 어빌리티를 종료
	if (!UnequipMontage)
	{
		HandleUnequipLogic(FGameplayEventData{});
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 몽타주 재생 태스크 생성
	UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, UnequipMontage);
	PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
	PlayMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);
	PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);

	// 게임플레이 이벤트 수신 대기 태스크 생성
	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, UnequipEventTag);
	WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::HandleUnequipLogic);

	PlayMontageTask->ReadyForActivation();
	WaitEventTask->ReadyForActivation();
}

void UACPlayerAbility_UnequipWeapon::OnMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UACPlayerAbility_UnequipWeapon::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UACPlayerAbility_UnequipWeapon::HandleUnequipLogic(FGameplayEventData Payload)
{
	const AACPlayerCharacter* OwnerCharacter = GetPlayerCharacterFromActorInfo();
	if (!OwnerCharacter)
	{
		return;
	}

	UPawnCombatComponent* PawnCombatComponent = GetPlayerCombatComponentFromActorInfo();
	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();

	if (!PawnCombatComponent || !ASC)
	{
		return;
	}

	if (AACWeapon* PlayerWeapon = Cast<AACWeapon>(PawnCombatComponent->GetCharacterCurrentEquippedWeapon()))
	{
		// 데이터 에셋 포인터를 가져오고 nullptr 체크
		if (const UACDataAsset_WeaponData* WeaponData = PlayerWeapon->WeaponData)
		{
			// 1. 서버: 능력 제거
			const TArray<FGameplayAbilitySpecHandle> GrantedAbilitySpecHandles = PlayerWeapon->GetGrantedAbilitySpecHandles();

			for (const FGameplayAbilitySpecHandle& SpecHandle : GrantedAbilitySpecHandles)
			{
				ASC->ClearAbility(SpecHandle);
			}

			PlayerWeapon->AssignGrantedAbilitySpecHandles(TArray<FGameplayAbilitySpecHandle>());

			// 2. 서버: 상태 변경 (OnRep 호출)
			PawnCombatComponent->SetCurrentEquippedWeaponTag(FGameplayTag()); // [수정]

			// 3. 서버: 무기 장착 태그 제거
			ASC->RemoveLooseGameplayTag(ACGameplayTags::Player_Ability_EquipWeapon);
		}
	}
}