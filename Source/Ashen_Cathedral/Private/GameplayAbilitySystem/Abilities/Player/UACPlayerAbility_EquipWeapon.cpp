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
	const AACPlayerCharacter* OwnerCharacter = GetPlayerCharacterFromActorInfo();

	if (!OwnerCharacter)
	{
		return;
	}

	UPawnCombatComponent* PawnCombatComponent = OwnerCharacter->GetPawnCombatComponent();
	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();

	if (!PawnCombatComponent || !ASC)
	{
		return;
	}

	if (AACWeapon* PlayerWeapon = Cast<AACWeapon>(PawnCombatComponent->GetCharacterCarriedWeaponByTag(WeaponToEquipTag)))
	{
		// 데이터 에셋 포인터를 가져오고 nullptr 체크
		if (const UACDataAsset_WeaponData* WeaponData = PlayerWeapon->WeaponData)
		{
			// 1. 서버: 능력 부여
			TArray<FGameplayAbilitySpecHandle> GrantedAbilitySpecHandles;

			// Reserve를 사용해서 메모리 최적화
			GrantedAbilitySpecHandles.Reserve(WeaponData->DefaultWeaponAbilities.Num());

			// UPawnUIComponent 가져오기
			// UPawnUIComponent* UIComponent = OwnerCharacter->FindComponentByClass<UPawnUIComponent>();

			for (const FACPlayerAbilitySet& AbilitySet : WeaponData->DefaultWeaponAbilities)
			{
				// 스펙 생성
				FGameplayAbilitySpec AbilitySpec(AbilitySet.AbilityToGrant);
				AbilitySpec.SourceObject = PlayerWeapon;
				AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilitySet.InputTag);

				const FGameplayAbilitySpecHandle SpecHandle = ASC->GiveAbility(AbilitySpec);

				// 생성된 무기 스펙 부여
				GrantedAbilitySpecHandles.Add(SpecHandle);

				// UI 컴포넌트에 아이콘 추가 (리플리케이트됨)
				// if (UIComponent && !AbilitySet.SoftAbilityIconMaterial.IsNull())
				// {
				// 	UIComponent->AddAbilityIcon(AbilitySet);
				// }
			}
			PlayerWeapon->AssignGrantedAbilitySpecHandles(GrantedAbilitySpecHandles);

			// 2. 서버: 상태 변경 (OnRep 호출)
			PawnCombatComponent->SetCurrentEquippedWeaponTag(WeaponToEquipTag); // [수정]

			// 3. UI에 장착 무기 아이콘 브로드캐스트
			if (const UPlayerUIComponent* PlayerUIComponent = OwnerCharacter->GetPlayerUIComponent())
			{
				PlayerUIComponent->OnEquippedWeaponChangedDelegate.Broadcast(WeaponData->SoftWeaponIconTexture);
			}

			// 4. 서버: 무기 장착 태그 추가
			ASC->AddLooseGameplayTag(ACGameplayTags::Player_Ability_EquipWeapon);
		}
	}
}