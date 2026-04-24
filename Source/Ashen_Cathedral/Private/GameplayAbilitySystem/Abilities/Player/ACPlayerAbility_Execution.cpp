// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_Execution.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "ACGameplayTags.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "MotionWarpingComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"

UACPlayerAbility_Execution::UACPlayerAbility_Execution()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Player_Ability_Execution);
	SetAssetTags(TagsToAdd);

	// 처형 실행 중 Player ASC에 부여되는 상태 태그
	ActivationOwnedTags.AddTag(ACGameplayTags::Player_Status_Executing);

	// 아래 태그가 Player ASC에 있으면 처형 발동 불가
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Dead);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_HitReact);   // 피격 중
	ActivationBlockedTags.AddTag(ACGameplayTags::Player_Status_Rolling);    // 회피 중
	ActivationBlockedTags.AddTag(ACGameplayTags::Player_Status_Executing);  // 이미 처형 중 (중복 방지)
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_SuperArmor); // 공격 중 (Attack ActivationOwnedTags)
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Groggy);     // 그로기 중

	// 처형 중 다른 플레이어 어빌리티 차단
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Attack_Light);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Attack_Heavy);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Roll);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Block);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Sprint);

	CancelAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Sprint);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UACPlayerAbility_Execution::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AACPlayerCharacter* PlayerCharacter = GetACPlayerFromActorInfo();
	if (!PlayerCharacter)
	{
		return false;
	}

	return FindExecutableTarget(PlayerCharacter) != nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// ActivateAbility
// ─────────────────────────────────────────────────────────────────────────────

void UACPlayerAbility_Execution::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bExecutionFinished = false;

	AACPlayerCharacter* PlayerCharacter = GetPlayerCharacterFromActorInfo();
	if (!PlayerCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// CanActivateAbility와 ActivateAbility 사이에 상태가 바뀔 수 있으므로 재탐색
	AACEnemyCharacter* TargetEnemy = FindExecutableTarget(PlayerCharacter);
	if (!TargetEnemy || !PlayerExecutionMontage || !EnemyExecutedMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedTargetEnemy = TargetEnemy;

	// 1. Enemy 상태 잠금 (Executed 태그 부여 → Groggy 취소 → 이동 잠금 → BT 정지)
	LockEnemyForExecution(TargetEnemy);

	// 2. Player 입력 잠금 + 현재 속도 즉시 정지
	//    DisableMovement()는 MOVE_None으로 바꿔 루트 모션을 차단하므로 사용하지 않음
	if (UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	if (APlayerController* PC = Cast<APlayerController>(PlayerCharacter->GetController()))
	{
		PlayerCharacter->DisableInput(PC);
	}

	// 3. MotionWarping 타겟 등록 + Enemy 회전 보정
	//    Player의 실제 이동은 처형 몽타주의 루트 모션 + MotionWarping이 담당
	SetupExecutionMotionWarp(PlayerCharacter, TargetEnemy);

	// 4. Shared.Event.ExecutionDamage 이벤트 대기 (AnimNotify가 이 이벤트를 발송해야 함)
	WaitDamageEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ACGameplayTags::Shared_Event_ExecutionDamage,
		nullptr,
		true // OnlyTriggerOnce
		);
	WaitDamageEventTask->EventReceived.AddDynamic(this, &ThisClass::OnExecutionDamageEventReceived);
	WaitDamageEventTask->ReadyForActivation();

	// 5. Enemy 처형 당하는 몽타주 재생 — 종료 시 이동/BT 복구
	if (UAnimInstance* EnemyAnim = TargetEnemy->GetMesh() ? TargetEnemy->GetMesh()->GetAnimInstance() : nullptr)
	{
		EnemyAnim->Montage_Play(EnemyExecutedMontage, 1.0f);
		EnemyAnim->OnMontageEnded.AddDynamic(this, &ThisClass::OnEnemyMontageEnded);
	}

	// 6. Player 처형 몽타주 재생 (AbilityTask로 종료 콜백 처리)
	PlayerMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		PlayerExecutionMontage,
		1.0f,
		NAME_None,
		false
		);
	PlayerMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnPlayerMontageCompleted);
	PlayerMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnPlayerMontageCancelled);
	PlayerMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnPlayerMontageCancelled);
	PlayerMontageTask->ReadyForActivation();
}

void UACPlayerAbility_Execution::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitStopTimerHandle);
	}

	// HitStop 잔여 복구
	RestoreEnemyTimeDilation();

	// Player 이동/입력 복구
	if (AACPlayerCharacter* PlayerCharacter = GetPlayerCharacterFromActorInfo())
	{
		if (UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
		if (APlayerController* PC = Cast<APlayerController>(PlayerCharacter->GetController()))
		{
			PlayerCharacter->EnableInput(PC);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AACEnemyCharacter* UACPlayerAbility_Execution::FindExecutableTarget(const AACPlayerCharacter* InPlayer) const
{
	if (!InPlayer)
	{
		return nullptr;
	}

	const UWorld* World = InPlayer->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector PlayerLocation = InPlayer->GetActorLocation();

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(InPlayer);

	World->OverlapMultiByChannel(
		OverlapResults,
		PlayerLocation,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(ExecutionDistance),
		QueryParams
		);

	AACEnemyCharacter* BestTarget = nullptr;
	float BestDistSquared = FLT_MAX;

	for (const FOverlapResult& Result : OverlapResults)
	{
		AACEnemyCharacter* Enemy = Cast<AACEnemyCharacter>(Result.GetActor());
		if (!Enemy)
		{
			continue;
		}

		const UACAbilitySystemComponent* EnemyASC = Enemy->GetACAbilitySystemComponent();
		if (!EnemyASC)
		{
			continue;
		}

		// Enemy 상태 조건
		if (!EnemyASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Groggy))
		{
			continue;
		}
		if (EnemyASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead))
		{
			continue;
		}
		if (EnemyASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Executed))
		{
			continue;
		}
		if (EnemyASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Invincible))
		{
			continue;
		}

		// 정면 판정: EnemyForward, (PlayerPos - EnemyPos).normalized
		const FVector EnemyToPlayer = (PlayerLocation - Enemy->GetActorLocation()).GetSafeNormal();
		const float DotValue = FVector::DotProduct(Enemy->GetActorForwardVector(), EnemyToPlayer);

		if (DotValue < ExecutionFrontDotThreshold)
		{
			continue;
		}

		// 가장 가까운 대상 선택
		const float DistSquared = FVector::DistSquared(PlayerLocation, Enemy->GetActorLocation());
		if (DistSquared < BestDistSquared)
		{
			BestDistSquared = DistSquared;
			BestTarget = Enemy;
		}
	}

	return BestTarget;
}

void UACPlayerAbility_Execution::LockEnemyForExecution(const AACEnemyCharacter* Enemy) const
{
	if (!Enemy)
	{
		return;
	}

	UACAbilitySystemComponent* EnemyASC = Enemy->GetACAbilitySystemComponent();
	if (!EnemyASC)
	{
		return;
	}

	// Shared.Status.Executed 부여 -> Groggy EndAbility에서 이동 복구를 건너뜀
	EnemyASC->AddLooseGameplayTag(ACGameplayTags::Shared_Status_Executed);

	// Groggy 어빌리티 취소 (Executed 태그 덕분에 EndAbility에서 이동 복구 스킵)
	FGameplayTagContainer GroggyFilter;
	GroggyFilter.AddTag(ACGameplayTags::Shared_Ability_Groggy);
	EnemyASC->CancelAbilities(&GroggyFilter);

	// 이동 명시적 잠금 (이중 안전망)
	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->DisableMovement();
		Movement->StopMovementImmediately();
	}

	// Enemy BT 일시정지
	if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
	{
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->PauseLogic("ExecutionInProgress");
		}
	}
}

void UACPlayerAbility_Execution::SetupExecutionMotionWarp(const AACPlayerCharacter* Player, AACEnemyCharacter* Enemy) const
{
	if (!Player || !Enemy)
	{
		return;
	}

	UMotionWarpingComponent* MotionWarpingComponent = Player->GetMotionWarpingComponent();
	if (!MotionWarpingComponent)
	{
		return;
	}

	// Warp 도달 목표 위치 = Enemy 전방 ExecutionSnapOffset, Z는 Player 현재값 유지
	FVector WarpLocation = Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * ExecutionSnapOffset;
	WarpLocation.Z = Player->GetActorLocation().Z;

	// Warp 완료 시 Player가 Enemy를 바라보는 방향
	const FVector PlayerToEnemy = (Enemy->GetActorLocation() - WarpLocation).GetSafeNormal();
	const FRotator WarpRotation = PlayerToEnemy.IsNearlyZero()
		? Player->GetActorRotation()
		: FRotator(0.f, PlayerToEnemy.Rotation().Yaw, 0.f);

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, WarpLocation, WarpRotation);

	// Enemy에는 MotionWarpingComponent가 없으므로 즉시 회전 보정
	const FVector EnemyToWarp = (WarpLocation - Enemy->GetActorLocation()).GetSafeNormal();
	if (!EnemyToWarp.IsNearlyZero())
	{
		Enemy->SetActorRotation(FRotator(0.f, EnemyToWarp.Rotation().Yaw, 0.f));
	}
}

void UACPlayerAbility_Execution::UnlockEnemy(const AACEnemyCharacter* Enemy)
{
	if (!IsValid(Enemy))
	{
		return;
	}

	// Shared.Status.Executed 제거
	if (UACAbilitySystemComponent* EnemyASC = Enemy->GetACAbilitySystemComponent())
	{
		EnemyASC->RemoveLooseGameplayTag(ACGameplayTags::Shared_Status_Executed);

		// 사망 상태면 Death 어빌리티가 이미 이동/AI 처리 → 복구 불필요
		if (EnemyASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead))
		{
			return;
		}
	}

	// 몽타주가 아직 재생 중이면 중단 — OnEnemyMontageEnded에서 이동/BT 복구
	if (EnemyExecutedMontage)
	{
		if (UAnimInstance* EnemyAnim = Enemy->GetMesh() ? Enemy->GetMesh()->GetAnimInstance() : nullptr)
		{
			if (EnemyAnim->Montage_IsPlaying(EnemyExecutedMontage))
			{
				EnemyAnim->Montage_Stop(0.2f, EnemyExecutedMontage);
				return; // OnMontageEnded 콜백에서 이동/BT 복구
			}
		}
	}

	// 몽타주가 이미 종료된 경우 직접 복구
	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
	if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
	{
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->ResumeLogic("ExecutionEnd");
		}
	}
}

void UACPlayerAbility_Execution::FinishExecution(bool bWasCancelled)
{
	if (bExecutionFinished)
	{
		return;
	}
	bExecutionFinished = true;

	// Enemy 상태 복구
	// CachedTargetEnemy는 리셋하지 않음 — OnEnemyMontageEnded 콜백이 이후에 발화할 수 있음
	if (CachedTargetEnemy.IsValid())
	{
		UnlockEnemy(CachedTargetEnemy.Get());
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UACPlayerAbility_Execution::OnPlayerMontageCompleted()
{
	if (!IsActive())
	{
		return;
	}
	FinishExecution(false);
}

void UACPlayerAbility_Execution::OnPlayerMontageCancelled()
{
	if (!IsActive())
	{
		return;
	}
	FinishExecution(true);
}

void UACPlayerAbility_Execution::OnExecutionDamageEventReceived(FGameplayEventData Payload)
{
	if (!IsActive() || !CachedTargetEnemy.IsValid())
	{
		return;
	}

	AACEnemyCharacter* TargetEnemy = CachedTargetEnemy.Get();
	UAbilitySystemComponent* EnemyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetEnemy);
	if (!EnemyASC)
	{
		return;
	}

	// CameraShake
	if (ExecutionCameraShakeClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(CurrentActorInfo->PlayerController.Get()))
		{
			PC->ClientStartCameraShake(ExecutionCameraShakeClass);
		}
	}

	// HitStop: Enemy CustomTimeDilation 낮추고 타이머로 복원
	if (HitStopDuration > 0.f)
	{
		TargetEnemy->CustomTimeDilation = HitStopTimeDilation;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				HitStopTimerHandle,
				this,
				&ThisClass::RestoreEnemyTimeDilation,
				HitStopDuration,
				false
				);
		}
	}

	// 처형 데미지 적용
	if (!ExecutionDamageEffect)
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(ExecutionDamageEffect, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// Shared.SetByCaller.BaseDamage로 전달 → ACCalculation_DamageTaken이 정상 처리
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		ACGameplayTags::Shared_SetByCaller_BaseDamage,
		ExecutionDamage
		);

	GetACAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(),
		EnemyASC
		);
}

void UACPlayerAbility_Execution::RestoreEnemyTimeDilation()
{
	HitStopTimerHandle.Invalidate();

	if (CachedTargetEnemy.IsValid())
	{
		CachedTargetEnemy->CustomTimeDilation = 1.0f;
	}
}

void UACPlayerAbility_Execution::OnEnemyMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != EnemyExecutedMontage || !CachedTargetEnemy.IsValid())
	{
		return;
	}

	AACEnemyCharacter* Enemy = CachedTargetEnemy.Get();

	// 사망 상태면 Death 어빌리티가 이동/AI 처리
	const UACAbilitySystemComponent* EnemyASC = Enemy->GetACAbilitySystemComponent();
	if (EnemyASC && EnemyASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead))
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
	if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
	{
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->ResumeLogic("ExecutionEnd");
		}
	}
}