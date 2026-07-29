// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayAbilitySystem/Abilities/Enemy/ACEnemyAbility_CriticalAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "ACGameplayTags.h"
#include "Camera/CameraShakeBase.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "MotionWarpingComponent.h"

UACEnemyAbility_CriticalAttack::UACEnemyAbility_CriticalAttack()
{
	FGameplayTagContainer TagsToAdd;
	TagsToAdd.AddTag(ACGameplayTags::Enemy_Ability_CriticalAttack);
	SetAssetTags(TagsToAdd);

	ActivationOwnedTags.AddTag(ACGameplayTags::Enemy_Status_CriticalAttacking);

	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_Dead);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_HitReact);
	ActivationBlockedTags.AddTag(ACGameplayTags::Shared_Status_PostureBroken);
	ActivationBlockedTags.AddTag(ACGameplayTags::Enemy_Status_CriticalAttacking);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UACEnemyAbility_CriticalAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return FindCriticalAttackTarget() != nullptr;
}

void UACEnemyAbility_CriticalAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bCriticalAttackFinished = false;
	ActivePlayerVictimMontage = nullptr;

	// CanActivateAbility와 이 시점 사이에 상태가 바뀔 수 있으므로 재탐색한다
	AACPlayerCharacter* TargetPlayer = FindCriticalAttackTarget();
	const FACEnemyCriticalAttackMontagePair* MontagePair = PickMontagePair();

	if (!TargetPlayer || !MontagePair)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedTargetPlayer = TargetPlayer;

	LockPlayerForCriticalAttack(TargetPlayer);
	SetupCriticalAttackMotionWarp(TargetPlayer);

	// 몽타주의 AnimNotify가 발송하는 타격 이벤트를 대기한다
	WaitDamageEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ACGameplayTags::Shared_Event_CriticalAttackDamage,
		nullptr,
		true // OnlyTriggerOnce
		);
	WaitDamageEventTask->EventReceived.AddDynamic(this, &ThisClass::OnCriticalAttackDamageEventReceived);
	WaitDamageEventTask->ReadyForActivation();

	// 플레이어 피격 몽타주 — Enemy 몽타주와 같은 프레임에 시작해야 연출이 맞물린다
	if (UAnimInstance* PlayerAnim = TargetPlayer->GetMesh() ? TargetPlayer->GetMesh()->GetAnimInstance() : nullptr)
	{
		PlayerAnim->Montage_Play(MontagePair->PlayerVictimMontage, 1.0f);
		ActivePlayerVictimMontage = MontagePair->PlayerVictimMontage;
	}

	EnemyMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MontagePair->EnemyMontage,
		1.0f,
		NAME_None,
		false
		);
	EnemyMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnEnemyMontageCompleted);
	EnemyMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnEnemyMontageCompleted);
	EnemyMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnEnemyMontageCancelled);
	EnemyMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnEnemyMontageCancelled);
	EnemyMontageTask->ReadyForActivation();
}

void UACEnemyAbility_CriticalAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 어떤 경로로 끝나든 플레이어가 잠긴 채 남지 않도록 보장한다
	if (CachedTargetPlayer.IsValid())
	{
		UnlockPlayer(CachedTargetPlayer.Get());
	}

	CachedTargetPlayer = nullptr;
	ActivePlayerVictimMontage = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

AACPlayerCharacter* UACEnemyAbility_CriticalAttack::FindCriticalAttackTarget() const
{
	const AACEnemyCharacter* EnemyCharacter = GetEnemyCharacterFromActorInfo();
	if (!EnemyCharacter)
	{
		return nullptr;
	}

	const UWorld* World = EnemyCharacter->GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	AACPlayerCharacter* PlayerCharacter = PlayerController ? Cast<AACPlayerCharacter>(PlayerController->GetPawn()) : nullptr;

	const bool bAccepted = PlayerCharacter && IsValidCriticalAttackTarget(EnemyCharacter, PlayerCharacter);

	if (bDebugDrawDetection)
	{
		DrawDetectionDebug(EnemyCharacter, PlayerCharacter, bAccepted);
	}

	return bAccepted ? PlayerCharacter : nullptr;
}

bool UACEnemyAbility_CriticalAttack::IsValidCriticalAttackTarget(const AACEnemyCharacter* EnemyCharacter, const AACPlayerCharacter* Player) const
{
	if (!EnemyCharacter || !Player)
	{
		return false;
	}

	const UAbilitySystemComponent* PlayerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AACPlayerCharacter*>(Player));
	if (!PlayerASC)
	{
		return false;
	}

	if (!PlayerASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_PostureBroken))
	{
		return false;
	}
	if (PlayerASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead)
		|| PlayerASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Executed)
		|| PlayerASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Invincible))
	{
		return false;
	}

	const FVector EnemyLocation = EnemyCharacter->GetActorLocation();
	const FVector PlayerLocation = Player->GetActorLocation();

	if (FVector::DistSquared(EnemyLocation, PlayerLocation) > FMath::Square(CriticalAttackDistance))
	{
		return false;
	}

	const FVector EnemyToPlayer = (PlayerLocation - EnemyLocation).GetSafeNormal();
	return FVector::DotProduct(EnemyCharacter->GetActorForwardVector(), EnemyToPlayer) >= CriticalAttackFrontDotThreshold;
}

void UACEnemyAbility_CriticalAttack::DrawDetectionDebug(const AACEnemyCharacter* EnemyCharacter, const AActor* TargetPlayer, bool bAccepted) const
{
	const UWorld* World = EnemyCharacter ? EnemyCharacter->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	constexpr float DebugLifeTime = 1.f;
	const FVector EnemyLocation = EnemyCharacter->GetActorLocation();

	DrawDebugSphere(World, EnemyLocation, CriticalAttackDistance, 16, FColor::Cyan, false, DebugLifeTime);

	// Dot 임계값을 반각(라디안)으로 환산해 정면 판정 범위를 원뿔로 표시한다
	const float HalfAngleRadians = FMath::Acos(FMath::Clamp(CriticalAttackFrontDotThreshold, -1.f, 1.f));
	DrawDebugCone(World, EnemyLocation, EnemyCharacter->GetActorForwardVector(), CriticalAttackDistance, HalfAngleRadians, HalfAngleRadians, 16, FColor::Yellow, false, DebugLifeTime);

	if (!TargetPlayer)
	{
		return;
	}

	const FVector PlayerLocation = TargetPlayer->GetActorLocation();

	const FColor ResultColor = bAccepted ? FColor::Green : FColor::Red;
	DrawDebugLine(World, EnemyLocation, PlayerLocation, ResultColor, false, DebugLifeTime, 0, 2.f);
	DrawDebugString(World, EnemyLocation + FVector(0.f, 0.f, 140.f), bAccepted ? TEXT("Critical: OK") : TEXT("Critical: NO"), nullptr, ResultColor, DebugLifeTime);

	// CriticalAttackSnapOffset 미리보기 — 발동 전에도 Enemy가 설 자리를 캡슐로 확인해 오프셋을 조정할 수 있다
	FVector PreviewLocation = PlayerLocation + TargetPlayer->GetActorForwardVector() * CriticalAttackSnapOffset;
	PreviewLocation.Z = EnemyLocation.Z;

	DrawDebugLine(World, PlayerLocation, PreviewLocation, FColor::Magenta, false, DebugLifeTime, 0, 1.f);

	if (const UCapsuleComponent* Capsule = EnemyCharacter->GetCapsuleComponent())
	{
		DrawDebugCapsule(World, PreviewLocation, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), FQuat::Identity, FColor::Magenta, false, DebugLifeTime);
	}
}

const FACEnemyCriticalAttackMontagePair* UACEnemyAbility_CriticalAttack::PickMontagePair() const
{
	TArray<const FACEnemyCriticalAttackMontagePair*> Candidates;
	for (const FACEnemyCriticalAttackMontagePair& Pair : CriticalAttackMontages)
	{
		// 한쪽만 있으면 연출이 어긋나므로 양쪽이 모두 채워진 항목만 후보로 삼는다
		if (Pair.EnemyMontage && Pair.PlayerVictimMontage)
		{
			Candidates.Add(&Pair);
		}
	}

	if (Candidates.IsEmpty())
	{
		return nullptr;
	}

	return Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
}

void UACEnemyAbility_CriticalAttack::LockPlayerForCriticalAttack(AACPlayerCharacter* Player) const
{
	if (!Player)
	{
		return;
	}

	if (UACAbilitySystemComponent* PlayerASC = Player->GetACAbilitySystemComponent())
	{
		// Executed 부여 → 연출 도중 히트리액트가 끼어들지 않는다 (ACAttributeSet의 HitReact 면역 목록)
		PlayerASC->AddLooseGameplayTag(ACGameplayTags::Shared_Status_Executed);

		FGameplayTagContainer PostureBrokenFilter;
		PostureBrokenFilter.AddTag(ACGameplayTags::Shared_Ability_PostureBroken);
		PlayerASC->CancelAbilities(&PostureBrokenFilter);
	}

	// 체간 붕괴 어빌리티가 DisableMovement()로 걸어둔 MOVE_None을 되돌린다.
	// MOVE_None에서는 CharacterMovementComponent가 루트 모션을 적용하지 않아 피격 몽타주가 제자리에서 재생되며,
	// Executed 태그 때문에 그로기의 EndAbility는 이동 복구를 건너뛰므로 여기서 직접 풀어야 한다.
	// 입력은 아래에서 계속 차단하므로 플레이어가 조작할 수는 없고 루트 모션만 통과한다.
	if (UCharacterMovementComponent* Movement = Player->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
		Movement->StopMovementImmediately();
	}

	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		Player->DisableInput(PC);
	}
}

void UACEnemyAbility_CriticalAttack::SetupCriticalAttackMotionWarp(AACPlayerCharacter* Player) const
{
	const AACEnemyCharacter* EnemyCharacter = GetEnemyCharacterFromActorInfo();
	if (!EnemyCharacter || !Player)
	{
		return;
	}

	// Warp 도달 목표 = 플레이어 전방 CriticalAttackSnapOffset 지점, Z는 Enemy 현재값 유지
	const FVector PlayerLocation = Player->GetActorLocation();
	FVector WarpLocation = PlayerLocation + Player->GetActorForwardVector() * CriticalAttackSnapOffset;
	WarpLocation.Z = EnemyCharacter->GetActorLocation().Z;

	const FVector WarpToPlayer = (PlayerLocation - WarpLocation).GetSafeNormal();
	const FRotator WarpRotation = WarpToPlayer.IsNearlyZero()
		? EnemyCharacter->GetActorRotation()
		: FRotator(0.f, WarpToPlayer.Rotation().Yaw, 0.f);

	// Enemy의 실제 이동은 몽타주의 루트 모션 + MotionWarping NotifyState가 처리한다
	if (UMotionWarpingComponent* MotionWarpingComponent = EnemyCharacter->GetMotionWarpingComponent())
	{
		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, WarpLocation, WarpRotation);
	}

	// 피격자는 이동하지 않으므로 회전만 즉시 보정해 마주보게 한다
	if (!WarpToPlayer.IsNearlyZero())
	{
		Player->SetActorRotation(FRotator(0.f, (-WarpToPlayer).Rotation().Yaw, 0.f));
	}

	if (bDebugDrawDetection)
	{
		// 실제로 등록된 워프 타겟 — 탐지 단계의 미리보기(자홍)와 구분되도록 주황으로 그린다
		if (const UWorld* World = EnemyCharacter->GetWorld())
		{
			DrawDebugSphere(World, WarpLocation, 30.f, 12, FColor::Orange, false, 3.f);
			DrawDebugDirectionalArrow(World, WarpLocation, WarpLocation + WarpRotation.Vector() * 80.f, 40.f, FColor::Orange, false, 3.f, 0, 2.f);
		}
	}
}

void UACEnemyAbility_CriticalAttack::UnlockPlayer(AACPlayerCharacter* Player) const
{
	if (!IsValid(Player))
	{
		return;
	}

	if (UACAbilitySystemComponent* PlayerASC = Player->GetACAbilitySystemComponent())
	{
		PlayerASC->RemoveLooseGameplayTag(ACGameplayTags::Shared_Status_Executed);

		// 사망했다면 Death 어빌리티가 이동/입력을 관리하므로 복구하지 않는다
		if (PlayerASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead))
		{
			return;
		}
	}

	if (UCharacterMovementComponent* Movement = Player->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}

	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		Player->EnableInput(PC);
	}
}

void UACEnemyAbility_CriticalAttack::FinishCriticalAttack(bool bWasCancelled)
{
	if (bCriticalAttackFinished)
	{
		return;
	}
	bCriticalAttackFinished = true;

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UACEnemyAbility_CriticalAttack::OnEnemyMontageCompleted()
{
	if (!IsActive())
	{
		return;
	}
	FinishCriticalAttack(false);
}

void UACEnemyAbility_CriticalAttack::OnEnemyMontageCancelled()
{
	if (!IsActive())
	{
		return;
	}

	// Enemy 연출이 끊겼는데 플레이어만 계속 당하는 모션을 재생하면 어긋나므로 같이 멈춘다
	if (ActivePlayerVictimMontage && CachedTargetPlayer.IsValid())
	{
		if (UAnimInstance* PlayerAnim = CachedTargetPlayer->GetMesh() ? CachedTargetPlayer->GetMesh()->GetAnimInstance() : nullptr)
		{
			if (PlayerAnim->Montage_IsPlaying(ActivePlayerVictimMontage))
			{
				PlayerAnim->Montage_Stop(0.2f, ActivePlayerVictimMontage);
			}
		}
	}

	FinishCriticalAttack(true);
}

void UACEnemyAbility_CriticalAttack::OnCriticalAttackDamageEventReceived(FGameplayEventData Payload)
{
	if (!IsActive() || !CachedTargetPlayer.IsValid())
	{
		return;
	}

	AACPlayerCharacter* TargetPlayer = CachedTargetPlayer.Get();

	if (CriticalAttackCameraShakeClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(TargetPlayer->GetController()))
		{
			PC->ClientStartCameraShake(CriticalAttackCameraShakeClass);
		}
	}

	if (!CriticalAttackDamageEffect)
	{
		return;
	}

	UAbilitySystemComponent* PlayerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetPlayer);
	UACAbilitySystemComponent* OwnerASC = GetACAbilitySystemComponentFromActorInfo();
	if (!PlayerASC || !OwnerASC)
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CriticalAttackDamageEffect, GetAbilityLevel());
	if (!SpecHandle.IsValid())
	{
		return;
	}

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle,
		ACGameplayTags::Shared_SetByCaller_BaseDamage,
		CriticalAttackDamage
		);

	OwnerASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), PlayerASC);
}
