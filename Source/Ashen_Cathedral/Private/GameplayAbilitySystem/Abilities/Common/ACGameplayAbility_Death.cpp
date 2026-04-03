// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbilitySystem/Abilities/Common/ACGameplayAbility_Death.h"
#include "ACGameplayTags.h"
#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/ACCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Interfaces/PawnDeathInterface.h"
#include "Items/Weapon/ACWeaponBase.h"

UACGameplayAbility_Death::UACGameplayAbility_Death()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Shared_Ability_Death);
	SetAssetTags(TagsToAdd);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Shared_Status_Dead 태그가 ASC에 추가될 때 어빌리티 자동 트리거
	// AttributeSet에서 Health <= 0 판정 시 해당 태그를 부여함
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = ACGameplayTags::Shared_Status_Dead;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagAdded;
	AbilityTriggers.Add(TriggerData);
}

void UACGameplayAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AACCharacterBase* CharacterBase = GetACCharacterFromActorInfo();
	if (!CharacterBase)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 1. CharacterMovement 비활성화
	if (UCharacterMovementComponent* MovementComponent = CharacterBase->GetCharacterMovement())
	{
		MovementComponent->DisableMovement();
		MovementComponent->StopMovementImmediately();
	}

	// 2. 플레이어 입력 비활성화
	if (APlayerController* PlayerController = Cast<APlayerController>(CharacterBase->GetController()))
	{
		CharacterBase->DisableInput(PlayerController);
	}

	// 3. 캐릭터 비활성화 처리 (콜리전, AI, 무기, 인터페이스, 브로드캐스트)
	HandleDeath();

	// 4. 몽타주 랜덤 선택
	UAnimMontage* SelectedMontage = nullptr;
	if (DeathMontages.Num() > 0)
	{
		const int32 RandomIndex = FMath::RandRange(0, DeathMontages.Num() - 1);
		SelectedMontage = DeathMontages[RandomIndex];
	}

	if (!SelectedMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 5. AbilityTask로 몽타주 재생
	//    자동 블렌드아웃 비활성화 상태이므로 OnCompleted/OnBlendOut은 호출되지 않음
	//    OnCancelled/OnInterrupted는 외부에서 강제 중단될 때만 호출됨
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		SelectedMontage,
		1.0f,
		NAME_None,
		false,
		1.0f
		);

	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);

	MontageTask->ReadyForActivation();

	// 6. 몽타주 길이만큼 대기 후 OnMontageCompleted 직접 호출
	//    자동 블렌드아웃이 꺼져 있으면 태스크 콜백이 오지 않으므로 타이머로 대체함
	const float MontageLength = SelectedMontage->GetPlayLength();
	GetWorld()->GetTimerManager().SetTimer(
		MontageEndTimerHandle,
		this,
		&ThisClass::OnMontageCompleted,
		MontageLength,
		false
		);
}

void UACGameplayAbility_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		// 몽타주 종료 타이머 정리 — 외부 중단으로 EndAbility가 먼저 호출된 경우
		World->GetTimerManager().ClearTimer(MontageEndTimerHandle);

		// 파괴 타이머 정리 — 중단으로 종료된 경우에만 (정상 종료 시에는 파괴 진행)
		if (bWasCancelled)
		{
			World->GetTimerManager().ClearTimer(DestroyTimerHandle);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UACGameplayAbility_Death::HandleDeath() const
{
	AACCharacterBase* CharacterBase = GetACCharacterFromActorInfo();
	if (!CharacterBase)
	{
		return;
	}

	// 1. AI 컨트롤러 비활성화 (AI인 경우)
	if (AAIController* AIController = Cast<AAIController>(CharacterBase->GetController()))
	{
		AIController->UnPossess();
	}

	// 2. 캡슐 콜리전 비활성화
	if (UCapsuleComponent* CapsuleComponent = CharacterBase->GetCapsuleComponent())
	{
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	// 3. 메시 콜리전 설정
	if (USkeletalMeshComponent* MeshComponent = CharacterBase->GetMesh())
	{
		MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	// 4. ASC의 모든 활성 어빌리티 취소 (사망 어빌리티 자신은 제외)
	if (UACAbilitySystemComponent* ASC = GetACAbilitySystemComponentFromActorInfo())
	{
		ASC->CancelAbilities(nullptr, &ActivationOwnedTags);
	}

	// 5. 현재 장착된 무기 액터 파괴
	if (const UPawnCombatComponent* CombatComponent = GetPawnCombatComponentFromActorInfo())
	{
		if (AACWeaponBase* EquippedWeapon = CombatComponent->GetCharacterCurrentEquippedWeapon())
		{
			EquippedWeapon->Destroy();
		}
	}

	// 6. [인터페이스] 캐릭터 내부 사망 후처리 호출
	//    IPawnDeathInterface를 구현한 캐릭터에서만 실행됨
	//    (PlayerCharacter: 게임오버 UI 등 / EnemyCharacter: 드롭 아이템 등)
	if (IPawnDeathInterface* DeathInterface = Cast<IPawnDeathInterface>(CharacterBase))
	{
		DeathInterface->OnDeath();
	}

	// 7. [브로드캐스트] 외부 시스템(UI, GameMode, 퀘스트 등)에 사망 이벤트 전파
	//    수신자는 OnDeathDelegate.AddDynamic()으로 구독
	CharacterBase->OnDeathDelegate.Broadcast(CharacterBase);
}

void UACGameplayAbility_Death::OnMontageCompleted()
{
	MontageEndTimerHandle.Invalidate();

	// 몽타주가 끝까지 재생된 후 DestroyDelay만큼 대기했다가 액터 파괴
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DestroyTimerHandle,
			this,
			&ThisClass::HandleCharacterDestruction,
			DestroyDelay,
			false
			);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UACGameplayAbility_Death::OnMontageCancelled()
{
	// 외부 중단 시 파괴 없이 어빌리티만 종료 (EndAbility에서 타이머 정리)
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UACGameplayAbility_Death::HandleCharacterDestruction()
{
	DestroyTimerHandle.Invalidate();

	if (!IsValid(this) || !CurrentActorInfo)
	{
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar))
	{
		return;
	}

	Avatar->Destroy();
}
