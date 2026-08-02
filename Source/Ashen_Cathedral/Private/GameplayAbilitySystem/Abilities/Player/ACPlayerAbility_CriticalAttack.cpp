// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_CriticalAttack.h"

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

#if AC_WEB_DEBUG
	#include "Debug/ACRunLogSubsystem.h"
	#include "Debug/ACWebDebugSubsystem.h"
#endif

UACPlayerAbility_CriticalAttack::UACPlayerAbility_CriticalAttack()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Player_Ability_CriticalAttack);
	SetAssetTags(TagsToAdd);

	// 크리티컬 어택 실행 중 Player ASC에 부여되는 상태 태그
	ActivationOwnedTags.AddTag(ACGameplayTags::Player_Status_CriticalAttacking);

	// 아래 태그가 Player ASC에 있으면 크리티컬 어택 발동 불가
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Dead);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_HitReact);   // 피격 중
	ActivationBlockedTags.AddTag(ACGameplayTags::Player_Status_Rolling);    // 회피 중
	ActivationBlockedTags.AddTag(ACGameplayTags::Player_Status_CriticalAttacking);  // 이미 크리티컬 어택 중 (중복 방지)
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_SuperArmor); // 몽타주/특수 상태에서 슈퍼아머 보유 중
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_PostureBroken);     // 체간 붕괴 중

	// 크리티컬 어택 중 다른 플레이어 어빌리티 차단
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Attack_Light);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Attack_Heavy);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Roll);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Block);
	BlockAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Sprint);

	CancelAbilitiesWithTag.AddTag(ACGameplayTags::Player_Ability_Sprint);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UACPlayerAbility_CriticalAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
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

	// 재생할 조합이 없으면 처형을 포기하고 LightAttack에 차례를 넘긴다
	if (PickRandomMontagePairIndex() == INDEX_NONE)
	{
		return false;
	}

	return FindCriticalAttackTarget(PlayerCharacter) != nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// ActivateAbility
// ─────────────────────────────────────────────────────────────────────────────

void UACPlayerAbility_CriticalAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bCriticalAttackFinished = false;

	AACPlayerCharacter* PlayerCharacter = GetPlayerCharacterFromActorInfo();
	if (!PlayerCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// CanActivateAbility와 ActivateAbility 사이에 상태가 바뀔 수 있으므로 재탐색
	AACEnemyCharacter* TargetEnemy = FindCriticalAttackTarget(PlayerCharacter);
	const int32 PairIndex = PickRandomMontagePairIndex();
	if (!TargetEnemy || PairIndex == INDEX_NONE)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 이후 단계와 종료 콜백이 모두 이 조합을 참조한다 — 재추첨하면 Player/Enemy 몽타주가 어긋난다
	SelectedMontagePair = CriticalAttackMontagePairs[PairIndex];
	bEnemyGetUpStarted = false;

	CachedTargetEnemy = TargetEnemy;

#if AC_WEB_DEBUG
	// 처형이 실제로 성립한 시점에만 마커를 남긴다
	if (UACWebDebugSubsystem* WebDebug = UACWebDebugSubsystem::Get(PlayerCharacter))
	{
		WebDebug->RecordMarker(PlayerCharacter, TEXT("CriticalAttack"));
	}
	if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(PlayerCharacter))
	{
		RunLog->NotifyCriticalAttack(PlayerCharacter);
	}
#endif

	// 1. Enemy 상태 잠금 (Executed 태그 부여 → 체간 붕괴 취소 → 이동 잠금 → BT 정지)
	LockEnemyForCriticalAttack(TargetEnemy);

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
	//    Player의 실제 이동은 크리티컬 어택 몽타주의 루트 모션 + MotionWarping이 담당
	SetupCriticalAttackMotionWarp(PlayerCharacter, TargetEnemy);

	// 4. Shared.Event.CriticalAttackDamage 이벤트 대기 (AnimNotify가 이 이벤트를 발송해야 함)
	WaitDamageEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ACGameplayTags::Shared_Event_CriticalAttackDamage,
		nullptr,
		true // OnlyTriggerOnce
		);
	WaitDamageEventTask->EventReceived.AddDynamic(this, &ThisClass::OnCriticalAttackDamageEventReceived);
	WaitDamageEventTask->ReadyForActivation();

	// 5. Enemy 크리티컬 어택 당하는 몽타주 재생 — 종료 시 이동/BT 복구
	if (UAnimInstance* EnemyAnim = TargetEnemy->GetMesh() ? TargetEnemy->GetMesh()->GetAnimInstance() : nullptr)
	{
		// 동일 Enemy에게 반복 발동될 경우 이전 활성화에서 남은 바인딩이 중복 등록되지 않도록 먼저 해제한다.
		EnemyAnim->OnMontageEnded.RemoveDynamic(this, &ThisClass::OnEnemyMontageEnded);
		EnemyAnim->Montage_Play(SelectedMontagePair.EnemyMontage, 1.0f);
		EnemyAnim->OnMontageEnded.AddDynamic(this, &ThisClass::OnEnemyMontageEnded);

		// 블렌드아웃 델리게이트는 재생 중인 인스턴스에 붙으므로 Montage_Play 뒤에 설정한다
		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindUObject(this, &ThisClass::OnEnemyMontageBlendingOut);
		EnemyAnim->Montage_SetBlendingOutDelegate(BlendingOutDelegate, SelectedMontagePair.EnemyMontage);
	}

	// 6. Player 크리티컬 어택 몽타주 재생 (AbilityTask로 종료 콜백 처리)
	PlayerMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		SelectedMontagePair.PlayerMontage,
		1.0f,
		NAME_None,
		false
		);
	PlayerMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnPlayerMontageCompleted);
	PlayerMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnPlayerMontageCancelled);
	PlayerMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnPlayerMontageCancelled);
	PlayerMontageTask->ReadyForActivation();
}

void UACPlayerAbility_CriticalAttack::EndAbility(
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

AACEnemyCharacter* UACPlayerAbility_CriticalAttack::FindCriticalAttackTarget(const AACPlayerCharacter* InPlayer) const
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
		FCollisionShape::MakeSphere(CriticalAttackDistance),
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
		if (!EnemyASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_PostureBroken))
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

		if (DotValue < CriticalAttackFrontDotThreshold)
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

int32 UACPlayerAbility_CriticalAttack::PickRandomMontagePairIndex() const
{
	// 유효한 항목만 모아서 추첨한다 — 배열 인덱스를 직접 뽑으면 빈 항목에 당첨될 수 있다
	TArray<int32, TInlineAllocator<8>> ValidIndices;
	for (int32 Index = 0; Index < CriticalAttackMontagePairs.Num(); ++Index)
	{
		if (CriticalAttackMontagePairs[Index].IsValid())
		{
			ValidIndices.Add(Index);
		}
	}

	if (ValidIndices.IsEmpty())
	{
		return INDEX_NONE;
	}

	return ValidIndices[FMath::RandRange(0, ValidIndices.Num() - 1)];
}

void UACPlayerAbility_CriticalAttack::LockEnemyForCriticalAttack(const AACEnemyCharacter* Enemy) const
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

	// Shared.Status.Executed 부여 -> 체간 붕괴 EndAbility에서 이동 복구를 건너뜀
	EnemyASC->AddLooseGameplayTag(ACGameplayTags::Shared_Status_Executed);

	// 체간 붕괴 어빌리티 취소 (Executed 태그 덕분에 EndAbility에서 이동 복구 스킵)
	FGameplayTagContainer PostureBrokenFilter;
	PostureBrokenFilter.AddTag(ACGameplayTags::Shared_Ability_PostureBroken);
	EnemyASC->CancelAbilities(&PostureBrokenFilter);

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

void UACPlayerAbility_CriticalAttack::SetupCriticalAttackMotionWarp(const AACPlayerCharacter* Player, AACEnemyCharacter* Enemy) const
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

	// Warp 도달 목표 위치 = Enemy 전방 CriticalAttackSnapOffset, Z는 Player 현재값 유지
	FVector WarpLocation = Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * CriticalAttackSnapOffset;
	WarpLocation.Z = Player->GetActorLocation().Z;

	// Warp 완료 시 Player가 Enemy를 바라보는 방향
	const FVector PlayerToEnemy = (Enemy->GetActorLocation() - WarpLocation).GetSafeNormal();
	const FRotator WarpRotation = PlayerToEnemy.IsNearlyZero()
		? Player->GetActorRotation()
		: FRotator(0.f, PlayerToEnemy.Rotation().Yaw, 0.f);

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, WarpLocation, WarpRotation);

	// 처형 마무리 지점 = Enemy 정면 축 위의 부호 있는 지점. 음수면 Enemy 뒤쪽이 되어 스쳐 지나가는 연출이 된다.
	// 회전은 접근 때 맞춘 값을 그대로 넘겨준다 — 마무리 NotifyState에서 Warp Rotation을 꺼두면 무시되고,
	// 켜져 있더라도 방향이 유지되어 이동 중 몸이 도는 일이 없다.
	if (bUseCriticalAttackEndWarp)
	{
		FVector EndLocation = Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * CriticalAttackEndOffset;
		EndLocation.Z = WarpLocation.Z;

		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(RetreatWarpTargetName, EndLocation, WarpRotation);
	}

	// Enemy에는 MotionWarpingComponent가 없으므로 즉시 회전 보정
	const FVector EnemyToWarp = (WarpLocation - Enemy->GetActorLocation()).GetSafeNormal();
	if (!EnemyToWarp.IsNearlyZero())
	{
		Enemy->SetActorRotation(FRotator(0.f, EnemyToWarp.Rotation().Yaw, 0.f));
	}
}

void UACPlayerAbility_CriticalAttack::UnlockEnemy(const AACEnemyCharacter* Enemy)
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

	// 몽타주가 아직 재생 중이면 강제로 끊지 않고 자연 종료를 기다린다 — OnEnemyMontageEnded에서 이동/BT 복구.
	// Player/Enemy 몽타주 길이가 서로 달라도, 어느 한쪽이 먼저 끝났다고 다른 쪽을 잘라내지 않는다.
	// 기상 몽타주도 같은 콜백으로 종료를 받으므로 함께 검사한다.
	if (UAnimInstance* EnemyAnim = Enemy->GetMesh() ? Enemy->GetMesh()->GetAnimInstance() : nullptr)
	{
		const bool bVictimPlaying = SelectedMontagePair.EnemyMontage && EnemyAnim->Montage_IsPlaying(SelectedMontagePair.EnemyMontage);
		const bool bGetUpPlaying = EnemyGetUpMontage && EnemyAnim->Montage_IsPlaying(EnemyGetUpMontage);

		if (bVictimPlaying || bGetUpPlaying)
		{
			return; // OnMontageEnded 콜백이 자연 종료 시 기상 재생과 이동/BT 복구를 처리한다
		}
	}

	// 몽타주가 이미 종료된 경우 직접 복구
	RestoreEnemyAfterCriticalAttack(Enemy);
}

bool UACPlayerAbility_CriticalAttack::TryPlayEnemyGetUpMontage(const AACEnemyCharacter* Enemy)
{
	if (!EnemyGetUpMontage || !IsValid(Enemy))
	{
		return false;
	}

	UAnimInstance* EnemyAnim = Enemy->GetMesh() ? Enemy->GetMesh()->GetAnimInstance() : nullptr;
	if (!EnemyAnim)
	{
		return false;
	}

	// OnMontageEnded 바인딩은 피처형 몽타주 재생 때 이미 걸어뒀으므로 그대로 기상 종료도 받는다
	return EnemyAnim->Montage_Play(EnemyGetUpMontage, 1.0f) > 0.f;
}

void UACPlayerAbility_CriticalAttack::RestoreEnemyAfterCriticalAttack(const AACEnemyCharacter* Enemy) const
{
	if (!IsValid(Enemy))
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

void UACPlayerAbility_CriticalAttack::FinishCriticalAttack(bool bWasCancelled)
{
	if (bCriticalAttackFinished)
	{
		return;
	}
	bCriticalAttackFinished = true;

	// Enemy 상태 복구
	// CachedTargetEnemy는 리셋하지 않음 — OnEnemyMontageEnded 콜백이 이후에 발화할 수 있음
	if (CachedTargetEnemy.IsValid())
	{
		UnlockEnemy(CachedTargetEnemy.Get());
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UACPlayerAbility_CriticalAttack::OnPlayerMontageCompleted()
{
	if (!IsActive())
	{
		return;
	}
	FinishCriticalAttack(false);
}

void UACPlayerAbility_CriticalAttack::OnPlayerMontageCancelled()
{
	if (!IsActive())
	{
		return;
	}
	FinishCriticalAttack(true);
}

void UACPlayerAbility_CriticalAttack::OnCriticalAttackDamageEventReceived(FGameplayEventData Payload)
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
	if (CriticalAttackCameraShakeClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(CurrentActorInfo->PlayerController.Get()))
		{
			PC->ClientStartCameraShake(CriticalAttackCameraShakeClass);
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

	// 크리티컬 어택 데미지 적용
	if (!CriticalAttackDamageEffect)
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CriticalAttackDamageEffect, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// Shared.SetByCaller.BaseDamage로 전달 → ACCalculation_DamageTaken이 정상 처리
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		ACGameplayTags::Shared_SetByCaller_BaseDamage,
		CriticalAttackDamage
		);

	GetACAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(),
		EnemyASC
		);
}

void UACPlayerAbility_CriticalAttack::RestoreEnemyTimeDilation()
{
	HitStopTimerHandle.Invalidate();

	if (CachedTargetEnemy.IsValid())
	{
		CachedTargetEnemy->CustomTimeDilation = 1.0f;
	}
}

void UACPlayerAbility_CriticalAttack::OnEnemyMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	const bool bVictimMontageEnded = Montage == SelectedMontagePair.EnemyMontage;
	const bool bGetUpMontageEnded = EnemyGetUpMontage && Montage == EnemyGetUpMontage;

	if ((!bVictimMontageEnded && !bGetUpMontageEnded) || !CachedTargetEnemy.IsValid())
	{
		return;
	}

	// 기상은 피처형 몽타주의 블렌드아웃 시점에 이미 얹혔다.
	// 그 여파로 피처형 몽타주가 중단 종료되는데, 여기서 복구하면 기상 도중에 AI가 살아난다.
	if (bVictimMontageEnded && bEnemyGetUpStarted)
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

	// 블렌드아웃 시간이 0이면 블렌드아웃 델리게이트를 못 받을 수 있으므로 여기서 한 번 더 시도한다
	if (bVictimMontageEnded && !bInterrupted && TryPlayEnemyGetUpMontage(Enemy))
	{
		bEnemyGetUpStarted = true;
		return;
	}

	bEnemyGetUpStarted = false;
	RestoreEnemyAfterCriticalAttack(Enemy);
}

void UACPlayerAbility_CriticalAttack::OnEnemyMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	// 중단된 경우(다른 몽타주가 슬롯을 가져간 경우)에는 기상을 얹지 않는다 — 서로 밀어내며 어긋난다
	if (bInterrupted || Montage != SelectedMontagePair.EnemyMontage || !CachedTargetEnemy.IsValid())
	{
		return;
	}

	AACEnemyCharacter* Enemy = CachedTargetEnemy.Get();

	const UACAbilitySystemComponent* EnemyASC = Enemy->GetACAbilitySystemComponent();
	if (EnemyASC && EnemyASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead))
	{
		return;
	}

	if (TryPlayEnemyGetUpMontage(Enemy))
	{
		bEnemyGetUpStarted = true;
	}
}
