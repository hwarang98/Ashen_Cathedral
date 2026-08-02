// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/Combat/PlayerCombatComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ACGameplayTags.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/UI/PlayerUIComponent.h"
#include "DataAssets/Items/Weapon/ACDataAsset_WeaponData.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_UnequipWeapon.h"
#include "Items/Weapon/ACWeapon.h"
#include "Subsystems/ACWeaponSelectionSubsystem.h"
#include "TimerManager.h"

UPlayerCombatComponent::UPlayerCombatComponent() {}

AACWeapon* UPlayerCombatComponent::GetPlayerCarriedWeaponByTag(FGameplayTag InWeaponTag) const
{
	return Cast<AACWeapon>(GetCharacterCarriedWeaponByTag(InWeaponTag));
}

AACWeapon* UPlayerCombatComponent::GetPlayerCurrentEquippedWeapon() const
{
	AACWeapon* PlayerWeapon = Cast<AACWeapon>(GetCharacterCurrentEquippedWeapon());

	return PlayerWeapon ? PlayerWeapon : nullptr;
}

const UACDataAsset_WeaponData* UPlayerCombatComponent::GetPlayerCurrentWeaponData() const
{
	if (AACWeapon* PlayerWeapon = GetPlayerCurrentEquippedWeapon())
	{
		return PlayerWeapon->WeaponData;
	}
	return nullptr;
}

const FACWeaponStatRow* UPlayerCombatComponent::GetCurrentWeaponStatRow() const
{
	if (const AACWeapon* PlayerWeapon = GetPlayerCurrentEquippedWeapon())
	{
		if (const UACDataAsset_WeaponData* WeaponData = PlayerWeapon->WeaponData)
		{
			return WeaponData->WeaponStatRow.GetRow<FACWeaponStatRow>(TEXT("GetCurrentWeaponStatRow"));
		}
	}
	return nullptr;
}

float UPlayerCombatComponent::GetPlayerCurrentEquippedWeaponDamageAtLevel() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->Damage;
	}
	return 0.f;
}

float UPlayerCombatComponent::GetPlayerCurrentWeaponLightPostureDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->LightAttackPostureDamage;
	}
	return 0.f;
}

float UPlayerCombatComponent::GetPlayerCurrentWeaponHeavyPostureDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->HeavyAttackPostureDamage;
	}
	return 0.f;
}

float UPlayerCombatComponent::GetPlayerCurrentWeaponCounterPostureDamage() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->CounterAttackPostureDamage;
	}
	return 0.f;
}

EACWeaponType UPlayerCombatComponent::GetPlayerCurrentWeaponType() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->WeaponType;
	}
	return EACWeaponType::None;
}

float UPlayerCombatComponent::GetCurrentWeaponBaseDamage() const
{
	return GetPlayerCurrentEquippedWeaponDamageAtLevel();
}

float UPlayerCombatComponent::GetCurrentWeaponLightAttackPostureDamage() const
{
	return GetPlayerCurrentWeaponLightPostureDamage();
}

float UPlayerCombatComponent::GetCurrentWeaponHeavyAttackPostureDamage() const
{
	return GetPlayerCurrentWeaponHeavyPostureDamage();
}

float UPlayerCombatComponent::GetCurrentWeaponCounterAttackPostureDamage() const
{
	return GetPlayerCurrentWeaponCounterPostureDamage();
}

float UPlayerCombatComponent::GetCurrentWeaponAttackSpeed() const
{
	if (const FACWeaponStatRow* Row = GetCurrentWeaponStatRow())
	{
		return Row->AttackSpeed;
	}
	return 1.f;
}

void UPlayerCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 레벨 전환 중 교체가 진행 중이었다면 여기서 정리해야 GameInstance의 진행 플래그가 다음 레벨로 넘어가지 않는다
	if (SwapPhase != EACWeaponSwapPhase::Idle)
	{
		AbortSwap(TEXT("컴포넌트 종료"));
	}

	if (AbilityEndedDelegateHandle.IsValid())
	{
		if (UACAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent())
		{
			ASC->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
		}
		AbilityEndedDelegateHandle.Reset();
	}

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UPlayerCombatComponent::SetCurrentEquippedWeaponTag(const FGameplayTag& NewWeaponTag)
{
	const FGameplayTag OldWeaponTag = CurrentEquippedWeaponTag;

	Super::SetCurrentEquippedWeaponTag(NewWeaponTag);

	if (OldWeaponTag == CurrentEquippedWeaponTag)
	{
		return;
	}

	// 해제 완료 신호. 지금은 GAS 어빌리티 리스트 락 안이므로 무기 파괴/어빌리티 부여는 반드시 다음 틱으로 미룬다
	if (SwapPhase == EACWeaponSwapPhase::WaitingUnequip && !CurrentEquippedWeaponTag.IsValid())
	{
		ScheduleSwapContinuation();
	}
}

bool UPlayerCombatComponent::RequestWeaponSwap(UACDataAsset_WeaponData* InNewWeaponData, bool bPlayUnequipMontage, bool bPlayEquipMontage)
{
	if (SwapPhase != EACWeaponSwapPhase::Idle)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerCombatComponent] 이미 무기 교체가 진행 중이라 요청을 거부했습니다."));
		return false;
	}

	if (!InNewWeaponData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerCombatComponent] WeaponData가 비어 있어 무기 교체를 시작할 수 없습니다."));
		return false;
	}

	if (!InNewWeaponData->WeaponClassToSpawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerCombatComponent] %s에 WeaponClassToSpawn이 설정되지 않았습니다."), *InNewWeaponData->GetName());
		return false;
	}

	if (!InNewWeaponData->WeaponTypeTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerCombatComponent] %s에 WeaponTypeTag가 설정되지 않았습니다."), *InNewWeaponData->GetName());
		return false;
	}

	if (!InNewWeaponData->EquipAbility.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerCombatComponent] %s에 EquipAbility가 설정되지 않았습니다."), *InNewWeaponData->GetName());
		return false;
	}

	const AACCharacterBase* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->GetMesh() || !GetOwnerAbilitySystemComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerCombatComponent] 소유 캐릭터 또는 ASC가 유효하지 않아 무기 교체를 시작할 수 없습니다."));
		return false;
	}

	if (const AACWeapon* EquippedWeapon = GetPlayerCurrentEquippedWeapon())
	{
		if (EquippedWeapon->WeaponData == InNewWeaponData)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PlayerCombatComponent] 이미 %s를 장착 중이라 교체하지 않습니다."), *InNewWeaponData->GetName());
			return false;
		}
	}

	EnsureAbilityEndedBinding();

	PendingWeaponData = InNewWeaponData;
	bPlayUnequipMontageRequested = bPlayUnequipMontage;
	bPlayEquipMontageRequested = bPlayEquipMontage;

	if (UACWeaponSelectionSubsystem* Subsystem = GetWeaponSelectionSubsystem())
	{
		Subsystem->SetWeaponChangeInProgress(true);
	}

	SwapPhase = EACWeaponSwapPhase::Swapping;

	if (GetCharacterCurrentEquippedWeapon() == nullptr)
	{
		// 맨손 상태에서의 최초 선택 — 해제 단계를 건너뛴다
		BeginSpawnAndEquipPhase();
	}
	else if (bPlayUnequipMontage)
	{
		BeginUnequipPhase();
	}
	else
	{
		// 해제 연출 없이 같은 프레임에 정리하고 곧바로 새 무기로 넘어간다
		ApplyUnequipEffects();
		BeginSpawnAndEquipPhase();
	}

	return true;
}

bool UPlayerCombatComponent::RequestSpawnSelectedWeapon()
{
	const UACWeaponSelectionSubsystem* Subsystem = GetWeaponSelectionSubsystem();
	if (!Subsystem || !Subsystem->HasSelectedWeaponData())
	{
		// 로비에서 맨손으로 시작하는 정상 경로이므로 경고가 아니다
		UE_LOG(LogTemp, Log, TEXT("[PlayerCombatComponent] 선택된 무기가 없어 맨손으로 시작합니다."));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// PossessedBy 안에서 호출되므로 입력 컨텍스트/애님 인스턴스가 준비된 다음 틱으로 미룬다
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::HandleDeferredSpawnSelected));

	return true;
}

void UPlayerCombatComponent::HandleDeferredSpawnSelected()
{
	if (UACWeaponSelectionSubsystem* Subsystem = GetWeaponSelectionSubsystem())
	{
		RequestWeaponSwap(Subsystem->GetSelectedWeaponData());
	}
}

void UPlayerCombatComponent::BeginUnequipPhase()
{
	SwapPhase = EACWeaponSwapPhase::WaitingUnequip;
	StartPhaseWatchdog();

	UACAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
	AACWeapon* CurrentWeapon = GetPlayerCurrentEquippedWeapon();

	if (!ASC || !CurrentWeapon)
	{
		ScheduleSwapContinuation();
		return;
	}

	// 해제 어빌리티는 무기의 DefaultWeaponAbilities 안에 들어 있다. 태그를 하드코딩하지 않고 클래스로 찾는다
	FGameplayAbilitySpecHandle UnequipHandle;

	for (const FGameplayAbilitySpecHandle& SpecHandle : CurrentWeapon->GetGrantedAbilitySpecHandles())
	{
		const FGameplayAbilitySpec* AbilitySpec = ASC->FindAbilitySpecFromHandle(SpecHandle);
		if (AbilitySpec && AbilitySpec->Ability && AbilitySpec->Ability->IsA(UACPlayerAbility_UnequipWeapon::StaticClass()))
		{
			UnequipHandle = SpecHandle;
			break;
		}
	}

	if (!UnequipHandle.IsValid() || !ASC->TryActivateAbility(UnequipHandle))
	{
		// 해제 어빌리티가 없거나 활성화가 거부됐다 — 몽타주 없이 강제 해제로 진행한다
		ScheduleSwapContinuation();
	}
}

void UPlayerCombatComponent::ScheduleSwapContinuation()
{
	if (SwapContinuationTimerHandle.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		SwapContinuationTimerHandle = World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::HandleSwapContinuation));
	}
}

void UPlayerCombatComponent::HandleSwapContinuation()
{
	SwapContinuationTimerHandle.Invalidate();
	ClearPhaseWatchdog();

	if (SwapPhase != EACWeaponSwapPhase::WaitingUnequip)
	{
		return;
	}

	SwapPhase = EACWeaponSwapPhase::Swapping;

	// 몽타주가 중단되어 해제 로직이 아예 실행되지 않았을 수 있으므로 취소 여부가 아니라 상태로 판단한다
	if (GetCharacterCurrentEquippedWeapon() != nullptr)
	{
		ApplyUnequipEffects();
	}

	BeginSpawnAndEquipPhase();
}

void UPlayerCombatComponent::ApplyUnequipEffects()
{
	UACAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
	AACWeapon* PlayerWeapon = GetPlayerCurrentEquippedWeapon();

	if (!ASC || !PlayerWeapon)
	{
		return;
	}

	const UACDataAsset_WeaponData* WeaponData = PlayerWeapon->WeaponData;

	for (const FGameplayAbilitySpecHandle& SpecHandle : PlayerWeapon->GetGrantedAbilitySpecHandles())
	{
		ASC->ClearAbility(SpecHandle);
	}

	PlayerWeapon->AssignGrantedAbilitySpecHandles(TArray<FGameplayAbilitySpecHandle>());

	SetCurrentEquippedWeaponTag(FGameplayTag());

	if (const AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(GetOwnerCharacter()))
	{
		if (const UPlayerUIComponent* PlayerUIComponent = PlayerCharacter->GetPlayerUIComponent())
		{
			PlayerUIComponent->OnEquippedWeaponChangedDelegate.Broadcast(nullptr);
		}
	}

	ASC->RemoveLooseGameplayTag(ACGameplayTags::Player_Ability_EquipWeapon);
	ASC->AddLooseGameplayTag(ACGameplayTags::Player_Weapon_Unarmed);

	if (WeaponData && WeaponData->WeaponTypeTag.IsValid())
	{
		ASC->RemoveLooseGameplayTag(WeaponData->WeaponTypeTag);
	}
}

void UPlayerCombatComponent::ClearWeaponAbilitySpecs(AACWeapon* InWeapon)
{
	UACAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();

	if (!ASC || !InWeapon)
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : InWeapon->GetGrantedAbilitySpecHandles())
	{
		ASC->ClearAbility(SpecHandle);
	}

	InWeapon->AssignGrantedAbilitySpecHandles(TArray<FGameplayAbilitySpecHandle>());

	if (InWeapon->HasValidEquipAbilitySpecHandle())
	{
		ASC->ClearAbility(InWeapon->GetEquipAbilitySpecHandle());
		InWeapon->ClearEquipAbilitySpecHandle();
	}
}

void UPlayerCombatComponent::DestroyAllRegisteredWeapons()
{
	for (const FGameplayTag& WeaponTag : GetCarriedWeaponTags())
	{
		AACWeaponBase* WeaponBase = GetCharacterCarriedWeaponByTag(WeaponTag);

		// 파괴 전에 이 무기에 매달린 모든 스펙을 회수해야 SourceObject가 죽은 스펙이 ASC에 남지 않는다
		ClearWeaponAbilitySpecs(Cast<AACWeapon>(WeaponBase));

		UnregisterWeapon(WeaponTag);

		if (WeaponBase)
		{
			WeaponBase->Destroy();
		}
	}
}

void UPlayerCombatComponent::BeginSpawnAndEquipPhase()
{
	DestroyAllRegisteredWeapons();

	AACCharacterBase* OwnerCharacter = GetOwnerCharacter();
	UACAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
	UWorld* World = GetWorld();

	if (!PendingWeaponData || !OwnerCharacter || !OwnerCharacter->GetMesh() || !ASC || !World)
	{
		AbortSwap(TEXT("소유 캐릭터 또는 WeaponData가 유효하지 않음"));
		return;
	}

	const FGameplayTag NewWeaponTag = PendingWeaponData->WeaponTypeTag;

	// RegisterSpawnedWeapon의 checkf로 죽는 대신 여기서 안전하게 중단한다
	if (GetCharacterCarriedWeaponByTag(NewWeaponTag))
	{
		AbortSwap(FString::Printf(TEXT("%s 태그의 무기가 이미 등록되어 있음"), *NewWeaponTag.ToString()));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AACWeapon* SpawnedWeapon = World->SpawnActor<AACWeapon>(PendingWeaponData->WeaponClassToSpawn, SpawnParams);
	if (!SpawnedWeapon)
	{
		AbortSwap(TEXT("무기 액터 스폰 실패"));
		return;
	}

	SpawnedWeapon->SetWeaponData(PendingWeaponData);
	PendingWeapon = SpawnedWeapon;

	RegisterSpawnedWeapon(NewWeaponTag, SpawnedWeapon, false);

	const FName AttachSocketName = (PendingWeaponData->InitialSocketName != NAME_None) ? PendingWeaponData->InitialSocketName : PendingWeaponData->UnequippedSocketName;
	if (AttachSocketName != NAME_None)
	{
		SpawnedWeapon->AttachToComponent(OwnerCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName);
	}

	if (SpawnedWeapon->GetHideUntilEquipped())
	{
		SpawnedWeapon->HideWeapon();
	}

	// OnGiven 정책 어빌리티는 GiveAbility 안에서 곧바로 활성화되므로 단계 전환을 먼저 끝내둔다
	if (bPlayEquipMontageRequested)
	{
		SwapPhase = EACWeaponSwapPhase::WaitingEquip;
		StartPhaseWatchdog();
	}

	FGameplayAbilitySpec EquipAbilitySpec(PendingWeaponData->EquipAbility.AbilityToGrant);
	EquipAbilitySpec.SourceObject = SpawnedWeapon;
	EquipAbilitySpec.GetDynamicSpecSourceTags().AddTag(PendingWeaponData->EquipAbility.InputTag);

	PendingEquipHandle = ASC->GiveAbility(EquipAbilitySpec);
	SpawnedWeapon->SetEquipAbilitySpecHandle(PendingEquipHandle);

	// GiveAbility 안에서 이미 장착까지 끝났을 수 있으므로 상태로 확인한다
	if (GetCharacterCurrentEquippedWeapon() == SpawnedWeapon)
	{
		FinishSwap(true);
		return;
	}

	if (!bPlayEquipMontageRequested)
	{
		// 몽타주 없이 장착 로직만 즉시 실행한다. 장착 어빌리티는 나중에 수동으로 다시 뽑을 수 있도록 부여된 채로 둔다
		ApplyEquipEffects(SpawnedWeapon);

		if (GetCharacterCurrentEquippedWeapon() == SpawnedWeapon)
		{
			FinishSwap(true);
		}
		else
		{
			AbortSwap(TEXT("즉시 장착에 실패함"));
		}

		return;
	}

	if (!ASC->TryActivateAbility(PendingEquipHandle))
	{
		AbortSwap(TEXT("장착 어빌리티 활성화가 거부됨"));
	}
}

void UPlayerCombatComponent::ApplyEquipEffects(AACWeapon* InWeapon)
{
	UACAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();

	if (!ASC || !InWeapon)
	{
		return;
	}

	const UACDataAsset_WeaponData* WeaponData = InWeapon->WeaponData;
	if (!WeaponData || !WeaponData->WeaponTypeTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerCombatComponent] %s의 WeaponData 또는 WeaponTypeTag가 유효하지 않아 장착하지 못했습니다."), *InWeapon->GetName());
		return;
	}

	// 같은 이벤트가 두 번 와도 어빌리티가 중복 지급되지 않게 막는다
	if (CurrentEquippedWeaponTag == WeaponData->WeaponTypeTag)
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> GrantedAbilitySpecHandles;
	GrantedAbilitySpecHandles.Reserve(WeaponData->DefaultWeaponAbilities.Num());

	for (const FACPlayerAbilitySet& AbilitySet : WeaponData->DefaultWeaponAbilities)
	{
		if (!AbilitySet.IsValid())
		{
			continue;
		}

		FGameplayAbilitySpec AbilitySpec(AbilitySet.AbilityToGrant);
		AbilitySpec.SourceObject = InWeapon;
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilitySet.InputTag);

		GrantedAbilitySpecHandles.Add(ASC->GiveAbility(AbilitySpec));
	}

	InWeapon->AssignGrantedAbilitySpecHandles(GrantedAbilitySpecHandles);

	// 소켓 부착 / 입력 컨텍스트 / 애님 레이어 / 장착 이펙트는 여기서 이어진다
	SetCurrentEquippedWeaponTag(WeaponData->WeaponTypeTag);

	if (const AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(GetOwnerCharacter()))
	{
		if (const UPlayerUIComponent* PlayerUIComponent = PlayerCharacter->GetPlayerUIComponent())
		{
			PlayerUIComponent->OnEquippedWeaponChangedDelegate.Broadcast(WeaponData->SoftWeaponIconTexture);
		}
	}

	ASC->AddLooseGameplayTag(ACGameplayTags::Player_Ability_EquipWeapon);
	ASC->RemoveLooseGameplayTag(ACGameplayTags::Player_Weapon_Unarmed);
	ASC->AddLooseGameplayTag(WeaponData->WeaponTypeTag);
}

void UPlayerCombatComponent::HandleAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (SwapPhase != EACWeaponSwapPhase::WaitingEquip || EndedData.AbilitySpecHandle != PendingEquipHandle)
	{
		return;
	}

	// GAS 콜스택 안에서 무기를 파괴할 수 있으므로 성공/실패 판정도 다음 틱으로 미룬다
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::OnEquipPhaseFinished));
	}
}

void UPlayerCombatComponent::OnEquipPhaseFinished()
{
	ClearPhaseWatchdog();

	if (SwapPhase != EACWeaponSwapPhase::WaitingEquip)
	{
		return;
	}

	// 어빌리티가 취소로 끝났더라도 노티파이에서 장착 로직이 이미 돌았다면 성공이므로 상태로 판정한다
	if (PendingWeapon && GetCharacterCurrentEquippedWeapon() == PendingWeapon.Get())
	{
		FinishSwap(true);
	}
	else
	{
		AbortSwap(TEXT("장착 어빌리티가 무기를 장착하지 못한 채 종료됨"));
	}
}

void UPlayerCombatComponent::AbortSwap(const FString& InReason)
{
	UE_LOG(LogTemp, Error, TEXT("[PlayerCombatComponent] 무기 교체 실패: %s"), *InReason);

	if (AACWeapon* WeaponToDiscard = PendingWeapon)
	{
		ClearWeaponAbilitySpecs(WeaponToDiscard);

		if (PendingWeaponData)
		{
			UnregisterWeapon(PendingWeaponData->WeaponTypeTag);
		}

		WeaponToDiscard->Destroy();
		PendingWeapon = nullptr;
	}

	if (UACAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent())
	{
		ASC->RemoveLooseGameplayTag(ACGameplayTags::Player_Ability_EquipWeapon);
		ASC->AddLooseGameplayTag(ACGameplayTags::Player_Weapon_Unarmed);
	}

	FinishSwap(false);
}

void UPlayerCombatComponent::FinishSwap(bool bSuccess)
{
	ClearPhaseWatchdog();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SwapContinuationTimerHandle);
	}
	SwapContinuationTimerHandle.Invalidate();

	UACDataAsset_WeaponData* FinishedWeaponData = PendingWeaponData;

	SwapPhase = EACWeaponSwapPhase::Idle;
	bPlayUnequipMontageRequested = true;
	bPlayEquipMontageRequested = true;
	PendingWeaponData = nullptr;
	PendingWeapon = nullptr;
	PendingEquipHandle = FGameplayAbilitySpecHandle();

	if (UACWeaponSelectionSubsystem* Subsystem = GetWeaponSelectionSubsystem())
	{
		// 선택 확정은 실제 스폰과 장착이 성공한 뒤에만 이루어진다
		if (bSuccess && FinishedWeaponData)
		{
			Subsystem->SetSelectedWeaponData(FinishedWeaponData);
		}

		Subsystem->SetWeaponChangeInProgress(false);
	}

	OnWeaponSwapFinishedDelegate.Broadcast(bSuccess);
}

void UPlayerCombatComponent::StartPhaseWatchdog()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SwapWatchdogTimerHandle, FTimerDelegate::CreateUObject(this, &ThisClass::HandlePhaseWatchdogExpired), FMath::Max(SwapPhaseTimeoutSeconds, 1.f), false);
	}
}

void UPlayerCombatComponent::ClearPhaseWatchdog()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SwapWatchdogTimerHandle);
	}
	SwapWatchdogTimerHandle.Invalidate();
}

void UPlayerCombatComponent::HandlePhaseWatchdogExpired()
{
	SwapWatchdogTimerHandle.Invalidate();

	switch (SwapPhase)
	{
		case EACWeaponSwapPhase::WaitingUnequip:
			// 중단하지 않고 강제 해제 경로로 계속 진행해 요청받은 무기를 결국 쥐어준다
			UE_LOG(LogTemp, Error, TEXT("[PlayerCombatComponent] 해제 단계가 시간 초과되어 강제로 진행합니다."));
			ScheduleSwapContinuation();
			break;

		case EACWeaponSwapPhase::WaitingEquip:
			UE_LOG(LogTemp, Error, TEXT("[PlayerCombatComponent] 장착 단계가 시간 초과되었습니다."));
			OnEquipPhaseFinished();
			break;

		default:
			break;
	}
}

UACWeaponSelectionSubsystem* UPlayerCombatComponent::GetWeaponSelectionSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;

	return GameInstance ? GameInstance->GetSubsystem<UACWeaponSelectionSubsystem>() : nullptr;
}

UACAbilitySystemComponent* UPlayerCombatComponent::GetOwnerAbilitySystemComponent() const
{
	const AACCharacterBase* OwnerCharacter = GetOwnerCharacter();

	return OwnerCharacter ? OwnerCharacter->GetACAbilitySystemComponent() : nullptr;
}

void UPlayerCombatComponent::EnsureAbilityEndedBinding()
{
	if (AbilityEndedDelegateHandle.IsValid())
	{
		return;
	}

	if (UACAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent())
	{
		AbilityEndedDelegateHandle = ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded);
	}
}