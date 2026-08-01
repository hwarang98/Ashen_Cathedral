// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_Attack.h"
#include "ACGameplayDebugHelper.h"
#include "ACGameplayTags.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"

UACPlayerAbility_Attack::UACPlayerAbility_Attack()
{
	// 이 어빌리티의 식별 태그 (에셋 태그)
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Player_Ability_Attack_Light);
	SetAssetTags(TagsToAdd);

	// 이 어빌리티가 활성화된 동안 아래 태그를 가진 어빌리티는 활성화 불가
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Attack_Light);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_EquipWeapon);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_UnEquipWeapon);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Sprint);

	// 공격 시작 시 진행 중인 스프린트를 즉시 취소
	CancelAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Sprint);
}

void UACPlayerAbility_Attack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bComboChaining = false;

	// 카운터 어택은 SelectAttackMontage를 거치지 않으므로, 여기서 초기화해 두면
	// 베이스의 CurrentComboCount = 0 동작과 동일하게 콤보 데미지 보너스가 빠진다.
	SelectedComboStage = 0;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Commit에 성공해 실제로 공격이 시작된 경우에만, 직전 공격이 걸어둔 리셋 타이머를 취소한다.
	// 그대로 두면 이번 몽타주 재생 도중 타이머가 만료되어 콤보가 중간에 끊긴다.
	if (IsActive())
	{
		if (AACPlayerCharacter* PlayerCharacter = GetTypedOuter<AACPlayerCharacter>())
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(PlayerCharacter->SharedComboResetTimerHandle);
			}
		}
	}
}

bool UACPlayerAbility_Attack::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return HasAnyStamina(ActorInfo);
}

bool UACPlayerAbility_Attack::HasAnyStamina(const FGameplayAbilityActorInfo* ActorInfo)
{
	const AACPlayerCharacter* PlayerCharacter = ActorInfo ? Cast<AACPlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UACAttributeSet* AttributeSet = PlayerCharacter ? PlayerCharacter->GetACAttributeSet() : nullptr;

	return !AttributeSet || AttributeSet->GetStamina() > 0.f;
}

bool UACPlayerAbility_Attack::IsSpecialAttackInputTag(const FGameplayTag& InputTag)
{
	return InputTag.MatchesTag(ACGameplayTags::InputTag_SpecialWeaponAbility);
}

UAnimMontage* UACPlayerAbility_Attack::SelectAttackMontage()
{
	if (AttackMontages.IsEmpty())
	{
		return nullptr;
	}

	// 단발성 스페셜 공격은 공유 콤보에 참여하지 않는다. 여러 몽타주가 등록되어 있으면 콤보 단계가 아니라
	// 연출 변형으로 보고 매번 랜덤으로 하나를 고른다 (CounterAttackMontages와 동일한 규칙).
	// SelectedComboStage는 0으로 남아 콤보 단계 데미지 보너스가 적용되지 않는다.
	if (!bParticipatesInSharedCombo)
	{
		const int32 RandomIndex = FMath::RandRange(0, AttackMontages.Num() - 1);
		return AttackMontages[RandomIndex];
	}

	AACPlayerCharacter* PlayerCharacter = GetTypedOuter<AACPlayerCharacter>();
	if (!PlayerCharacter)
	{
		return Super::SelectAttackMontage();
	}

	const int32 LastIndex = AttackMontages.Num() - 1;

	// 직전 공격이 피니셔 직전 단계였다면 배열 길이와 무관하게 이번 공격을 피니셔로 연결하고,
	// 그렇지 않으면 공유 단계를 이 배열의 유효 범위로 Clamp한다.
	const int32 SelectedIndex = PlayerCharacter->bSharedComboFinisherReady
		? LastIndex
		: FMath::Clamp(PlayerCharacter->SharedComboCount, 0, LastIndex);

	PlayerCharacter->bSharedComboFinisherPlaying = (SelectedIndex == LastIndex);
	// 길이가 1이면 LastIndex - 1이 -1이라 예약이 서지 않고, 항상 피니셔로만 처리된다.
	PlayerCharacter->bSharedComboFinisherReady = (SelectedIndex == LastIndex - 1);
	PlayerCharacter->SharedComboCount = SelectedIndex + 1;

	SelectedComboStage = SelectedIndex + 1;

	return AttackMontages[SelectedIndex];
}

int32 UACPlayerAbility_Attack::GetComboDamageCount() const
{
	return SelectedComboStage;
}

void UACPlayerAbility_Attack::HandleComboComplete()
{
	AACPlayerCharacter* PlayerCharacter = GetTypedOuter<AACPlayerCharacter>();
	if (!PlayerCharacter)
		return;

	// 피니셔와 단발성 스페셜은 리셋 지연 없이 즉시 초기화해, 다음 공격이 반드시 1타부터 시작하게 한다.
	if (!bParticipatesInSharedCombo || PlayerCharacter->bSharedComboFinisherPlaying)
	{
		PlayerCharacter->ResetSharedComboState();
		ResetComboCount();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		// 공유 핸들을 사용하므로 다른 어빌리티가 건 타이머를 자동으로 교체한다.
		World->GetTimerManager().SetTimer(
			PlayerCharacter->SharedComboResetTimerHandle,
			this,
			&ThisClass::OnComboResetTimerExpired,
			PlayerCharacter->ComboResetDelay,
			false);
	}
}

void UACPlayerAbility_Attack::HandleComboCancelled()
{
	AACPlayerCharacter* PlayerCharacter = GetTypedOuter<AACPlayerCharacter>();

	if (bComboChaining)
	{
		bComboChaining = false;

		// 체인 중에는 콤보 단계를 유지해야 하므로, 직전 공격이 걸어둔 리셋 타이머만 취소한다.
		if (PlayerCharacter)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(PlayerCharacter->SharedComboResetTimerHandle);
			}
		}
		return;
	}

	if (PlayerCharacter)
	{
		PlayerCharacter->ResetSharedComboState();
	}
	Super::HandleComboCancelled();
}

void UACPlayerAbility_Attack::OnComboResetTimerExpired()
{
	if (AACPlayerCharacter* PlayerChar = GetTypedOuter<AACPlayerCharacter>())
	{
		PlayerChar->ResetSharedComboState();
	}
	ResetComboCount();
}

void UACPlayerAbility_Attack::TriggerComboChain(const FGameplayTag& InputTag)
{
	// 단발성 스페셜 공격에서는 어떤 후속 콤보로도 이어지지 않는다.
	if (!IsActive() || !bParticipatesInSharedCombo)
	{
		return;
	}

	AACPlayerCharacter* PlayerCharacter = GetTypedOuter<AACPlayerCharacter>();

	// 피니셔 재생 중에는 콤보 윈도우가 열려 있어도 추가 체이닝을 허용하지 않는다.
	if (PlayerCharacter && PlayerCharacter->bSharedComboFinisherPlaying)
	{
		return;
	}

	// EndAbility 이후 CurrentActorInfo가 무효화될 수 있으므로 미리 ASC를 캐시한다.
	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();

	// 콤보 윈도우 구간(ANS_ComboWindow)이 아니면 체인을 허용하지 않는다.
	if (!ASC || !ASC->HasMatchingGameplayTag(ACGameplayTags::Player_Status_ComboWindow))
	{
		return;
	}

	bComboChaining = true;
	// EndAbility(true) → HandleComboCancelled(bComboChaining=true → ResetComboCount 건너뜀)
	//                   → Super::EndAbility() → bIsActive=false, 블록 해제
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);

	// EndAbility 완료 직후 동기 방식으로 다음 어빌리티를 활성화한다.
	// PlayMontageAndWait가 새 몽타주를 재생하면 기존 몽타주는 자동으로 블렌드아웃된다.
	const FGameplayAbilitySpec* NextSpec = IsValid(ASC) ? ASC->FindInactiveAbilitySpecByInputTag(InputTag) : nullptr;
	const bool bNextAbilityActivated = NextSpec && ASC->TryActivateAbility(NextSpec->Handle);

	// 대상 어빌리티를 찾지 못했거나 활성화에 실패하면(스태미나 부족 등) 다음 공격으로 이어지지 않으므로
	// 예약된 피니셔와 콤보 단계가 남지 않게 즉시 정리한다.
	if (!bNextAbilityActivated && PlayerCharacter)
	{
		PlayerCharacter->ResetSharedComboState();
		ResetComboCount();
	}
}

bool UACPlayerAbility_Attack::TryTriggerSpecialAttack(const FGameplayTag& InputTag)
{
	// 스페셜 실행 중(공유 콤보 미참여)에는 추가 전환을 허용하지 않는다.
	if (!IsActive() || !bParticipatesInSharedCombo || !IsSpecialAttackInputTag(InputTag))
	{
		return false;
	}

	// EndAbility 이후 CurrentActorInfo가 무효화될 수 있으므로 미리 ASC를 캐시한다.
	UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo();
	if (!IsValid(ASC))
	{
		return false;
	}

	const AACPlayerCharacter* PlayerCharacter = GetTypedOuter<AACPlayerCharacter>();

	// 피니셔 재생 중에는 전용 SpecialLinkWindow에서만, 일반 공격 중에는 기존 ComboWindow에서만 전환을 허용한다.
	const FGameplayTag& RequiredWindowTag = (PlayerCharacter && PlayerCharacter->bSharedComboFinisherPlaying)
		? ACGameplayTags::Player_Status_SpecialLinkWindow
		: ACGameplayTags::Player_Status_ComboWindow;

	if (!ASC->HasMatchingGameplayTag(RequiredWindowTag))
	{
		return false;
	}

	const FGameplayAbilitySpec* SpecialSpec = ASC->FindInactiveAbilitySpecByInputTag(InputTag);
	if (!SpecialSpec || !SpecialSpec->Ability)
	{
		return false;
	}

	// 현재 공격이 걸어둔 BlockAbilitiesWithTag 때문에 CanActivateAbility는 이 시점에 반드시 실패한다.
	// 그래서 블록 태그와 무관하게 판정할 수 있는 조건(쿨다운/비용/스태미나)만 어빌리티 종료 '전'에 검사해,
	// 쓸 수 없는 스페셜 때문에 진행 중인 피니셔가 끊기는 것을 막는다.
	const FGameplayAbilitySpecHandle SpecialHandle = SpecialSpec->Handle;
	if (!SpecialSpec->Ability->CheckCooldown(SpecialHandle, CurrentActorInfo)
		|| !SpecialSpec->Ability->CheckCost(SpecialHandle, CurrentActorInfo)
		|| !HasAnyStamina(CurrentActorInfo))
	{
		return false;
	}

	// 여기서부터 전환을 확정한다. bComboChaining을 설정하지 않으므로
	// EndAbility → HandleComboCancelled 경로가 ResetSharedComboState()로 일반 콤보 상태를 초기화한다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);

	// 활성화에 실패하더라도 위에서 콤보 상태가 이미 정리되었으므로 남는 상태가 없다.
	ASC->TryActivateAbility(SpecialHandle);
	return true;
}
