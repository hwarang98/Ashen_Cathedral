// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Player/UACPlayerAbility_EquipWeapon.h"
#include "ACGameplayTags.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/UI/PlayerUIComponent.h"
#include "DataAssets/Items/Weapon/ACDataAsset_WeaponData.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Items/Weapon/ACWeapon.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

UUACPlayerAbility_EquipWeapon::UUACPlayerAbility_EquipWeapon()
{
	// --- 어빌리티 태그 설정 ---
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Player_Ability_EquipWeapon); // 예시 태그. 필요시 수정
	SetAssetTags(TagsToAdd);

	ActivationOwnedTags.AddTag(ACGameplayTags::Player_Status_Equipping); // 이 어빌리티가 활성화되어 있는 동안 소유자(캐릭터)에게 이 태그를 부여

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	EquipEventTag = ACGameplayTags::Player_Event_EquipWeapon;
}

bool UUACPlayerAbility_EquipWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
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

	const UPawnCombatComponent* CombatComponent = OwnerCharacter->GetPawnCombatComponent();
	if (!CombatComponent)
	{
		return false;
	}

	// 무기를 들고 있지 않아야 활성화 가능
	if (CombatComponent->GetCharacterCurrentEquippedWeapon() != nullptr)
	{
		return false;
	}

	return true;
}

void UUACPlayerAbility_EquipWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 몽타주가 없으면, 로직만 즉시 실행하고 어빌리티를 종료
	if (!EquipMontage)
	{
		HandleEquipLogic(FGameplayEventData{});
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 몽타주 재생 태스크 생성
	UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, EquipMontage);
	PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnded);
	PlayMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnded);

	PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);

	//게임플레이 이벤트 수신 대기 태스크 생성
	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, EquipEventTag);
	WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::HandleEquipLogic);

	// --- 3. 두 태스크 모두 활성화 ---
	PlayMontageTask->ReadyForActivation();
	WaitEventTask->ReadyForActivation();
}

void UUACPlayerAbility_EquipWeapon::OnMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UUACPlayerAbility_EquipWeapon::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UUACPlayerAbility_EquipWeapon::HandleEquipLogic(FGameplayEventData Payload)
{
	// 실제 장착 처리는 UPlayerCombatComponent가 단일 원본으로 보유한다.
	// 로비 선택대의 즉시 교체도 같은 함수를 호출하므로 두 경로의 결과가 항상 같다.
	// WaitGameplayEvent는 OnlyTriggerOnce=false이므로 이벤트가 두 번 와도 되도록
	// 중복 지급 방어는 ApplyEquipEffects 안에 있다.
	if (UPlayerCombatComponent* PlayerCombatComponent = GetPlayerCombatComponentFromActorInfo())
	{
		PlayerCombatComponent->ApplyEquipEffects(Cast<AACWeapon>(PlayerCombatComponent->GetCharacterCarriedWeaponByTag(WeaponToEquipTag)));
	}
}