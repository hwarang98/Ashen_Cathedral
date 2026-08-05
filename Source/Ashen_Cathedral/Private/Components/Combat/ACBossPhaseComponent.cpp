// 보스의 페이즈 전환을 관리하는 컴포넌트 — 체력 0을 사망 대신 다음 페이즈 진입으로 바꾼다

#include "Components/Combat/ACBossPhaseComponent.h"
#include "ACGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Abilities/GameplayAbility.h"
#include "Components/UI/PawnUIComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "Interfaces/PawnUIInterface.h"

UACBossPhaseComponent::UACBossPhaseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UACBossPhaseComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UACBossPhaseComponent, CurrentPhase);
	DOREPLIFETIME(UACBossPhaseComponent, bPhaseTransitionInProgress);
}

void UACBossPhaseComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentPhase = FMath::Max(CurrentPhase, 1);

	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!OwningPawn)
	{
		return;
	}

	UAbilitySystemComponent* ASC = OwningPawn->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		return;
	}

	CachedASC = ASC;

	// 판정은 서버에서만 한다. 클라이언트는 복제된 CurrentPhase / bPhaseTransitionInProgress만 읽는다
	if (!HasAuthority())
	{
		return;
	}

	// HealthPercentage 조건용 구독. 체력이 임계 비율을 처음 통과하는 순간을 잡는다
	HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UACAttributeSet::GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthAttributeChanged);
}

void UACBossPhaseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 컷신 도중 보스가 제거되어도 스폰해둔 시퀀스 액터와 델리게이트가 남지 않게 한다
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TransitionFailsafeTimerHandle);
	}

	CleanupTransitionSequence();
	CleanupTransitionAbility();

	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		if (HealthChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UACAttributeSet::GetHealthAttribute()).Remove(HealthChangedHandle);
			HealthChangedHandle.Reset();
		}
	}

	Super::EndPlay(EndPlayReason);
}

UACBossPhaseComponent* UACBossPhaseComponent::FindBossPhaseComponent(const AActor* InActor)
{
	return InActor ? InActor->FindComponentByClass<UACBossPhaseComponent>() : nullptr;
}

bool UACBossPhaseComponent::HasAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

int32 UACBossPhaseComponent::GetNextTransitionIndex() const
{
	// 페이즈 N에서 소비할 전환은 항상 인덱스 N-1이다. 전환이 끝나면 CurrentPhase가 올라가며 다음 인덱스로 넘어간다
	const int32 Index = CurrentPhase - 1;
	return PhaseTransitions.IsValidIndex(Index) ? Index : INDEX_NONE;
}

bool UACBossPhaseComponent::WillRestoreHealthOnPendingTransition() const
{
	return PhaseTransitions.IsValidIndex(PendingTransitionIndex) && PhaseTransitions[PendingTransitionIndex].RestoreHealthPercent > 0.f;
}

bool UACBossPhaseComponent::TryHandleZeroHealth(AActor* DamageInstigator)
{
	if (!HasAuthority())
	{
		return false;
	}

	// 최종 사망이 이미 시작됐다면 더 이상 가로채지 않는다
	if (bFinalDeathStarted)
	{
		return false;
	}

	// 컷신 도중 들어온 추가 피해 — 사망시키지 않는다.
	// 무적 태그가 대부분 걸러주지만, 태그가 붙기 전 같은 프레임에 밀려든 피해까지 여기서 막는다
	if (bPhaseTransitionInProgress)
	{
		return true;
	}

	const int32 NextIndex = GetNextTransitionIndex();
	if (NextIndex == INDEX_NONE || PhaseTransitions[NextIndex].TriggerType != EACPhaseTransitionTrigger::HealthZero)
	{
		// 남은 전환이 없거나, 남은 전환이 체력 0으로 발동하는 것이 아니다 → 기존 사망 처리로 넘긴다
		bFinalDeathStarted = true;
		if (UAbilitySystemComponent* ASC = CachedASC.Get())
		{
			ASC->AddLooseGameplayTag(ACGameplayTags::Enemy_Status_Dying);
		}
		return false;
	}

	BeginPhaseTransition(NextIndex);
	return true;
}

bool UACBossPhaseComponent::RequestPhaseTransition()
{
	if (!HasAuthority() || bFinalDeathStarted || bPhaseTransitionInProgress)
	{
		return false;
	}

	const int32 NextIndex = GetNextTransitionIndex();
	if (NextIndex == INDEX_NONE || PhaseTransitions[NextIndex].TriggerType != EACPhaseTransitionTrigger::Manual)
	{
		return false;
	}

	BeginPhaseTransition(NextIndex);
	return true;
}

void UACBossPhaseComponent::OnHealthAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!HasAuthority() || bFinalDeathStarted || bPhaseTransitionInProgress)
	{
		return;
	}

	const int32 NextIndex = GetNextTransitionIndex();
	if (NextIndex == INDEX_NONE || PhaseTransitions[NextIndex].TriggerType != EACPhaseTransitionTrigger::HealthPercentage)
	{
		return;
	}

	const UAbilitySystemComponent* ASC = CachedASC.Get();
	const UACAttributeSet* AttributeSet = ASC ? ASC->GetSet<UACAttributeSet>() : nullptr;
	if (!AttributeSet)
	{
		return;
	}

	const float MaxHealth = AttributeSet->GetMaxHealth();
	if (MaxHealth <= 0.f)
	{
		return;
	}

	// 임계 비율을 통과한 최초 1회만 발동한다 — 전환이 끝나면 다음 인덱스로 넘어가므로 같은 항목이 다시 잡히지 않는다
	if (ChangeData.NewValue / MaxHealth <= PhaseTransitions[NextIndex].HealthPercentage)
	{
		BeginPhaseTransition(NextIndex);
	}
}

void UACBossPhaseComponent::BeginPhaseTransition(int32 TransitionIndex)
{
	if (!PhaseTransitions.IsValidIndex(TransitionIndex))
	{
		return;
	}

	// 1. 중복 전환 차단 + 추가 피해로 인한 사망 차단.
	//    이 두 플래그를 가장 먼저 세워야 아래 단계에서 발생하는 콜백이 다시 전환을 시작하지 못한다
	bPhaseTransitionInProgress = true;
	PendingTransitionIndex = TransitionIndex;
	bTransitionSequenceEndHandled = false;

	const FACBossPhaseTransition& Transition = PhaseTransitions[TransitionIndex];

	// 2. AI 행동 중지 — 어빌리티를 취소하기 전에 멈춰야 취소 통보를 받은 AI가 곧바로 다음 행동을 고르지 않는다
	PauseAILogic();

	// 3. 진행 중인 공격 어빌리티 중지
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		const FGameplayTagContainer* KeepTags = AbilityTagsToKeepDuringTransition.IsEmpty() ? nullptr : &AbilityTagsToKeepDuringTransition;
		ASC->CancelAbilities(nullptr, KeepTags);

		// 4. 임시 상태 태그 — 전환 중임을 알리고(AI 반응 차단) 추가 피해를 무효화한다.
		//    둘 다 이 컴포넌트가 소유하며, CompletePhaseTransition에서 대칭으로 제거한다
		ASC->AddLooseGameplayTag(ACGameplayTags::Enemy_Status_PhaseTransition);
		ASC->AddLooseGameplayTag(ACGameplayTags::Shared_Status_Invincible);
	}

	// 5. 콜백이 하나도 오지 않는 상황(시퀀스 정지 실패, 어빌리티 무한 대기 등)을 대비한 안전망
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TransitionFailsafeTimerHandle, this, &ThisClass::OnTransitionFailsafeElapsed, MaxTransitionDuration, false);
	}

	OnPhaseTransitionStarted.Broadcast(CurrentPhase);

	// 6. 컷신 재생. 시퀀스가 없거나 재생에 실패하면 곧바로 어빌리티 단계로 넘어간다
	if (!PlayTransitionSequence(Transition))
	{
		StartTransitionAbilityStep();
	}
}

bool UACBossPhaseComponent::PlayTransitionSequence(const FACBossPhaseTransition& Transition)
{
	if (!Transition.TransitionSequence)
	{
		return false;
	}

	ALevelSequenceActor* SpawnedActor = nullptr;
	ULevelSequencePlayer* SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetOwner(), Transition.TransitionSequence, FMovieSceneSequencePlaybackSettings(), SpawnedActor);
	if (!SequencePlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossPhase] TransitionSequence 재생에 실패했습니다. 컷신을 건너뛰고 전환을 계속합니다: %s"), *GetNameSafe(GetOwner()));
		return false;
	}

	ActiveSequencePlayer = SequencePlayer;
	ActiveSequenceActor = SpawnedActor;

	// 정상 종료(OnFinished)와 스킵·강제 중단(OnStop)을 같은 핸들러로 받는다.
	// 두 이벤트가 연달아 들어와도 bTransitionSequenceEndHandled가 진행을 정확히 한 번으로 묶는다
	SequencePlayer->OnFinished.AddDynamic(this, &ThisClass::OnTransitionSequenceEnded);
	SequencePlayer->OnStop.AddDynamic(this, &ThisClass::OnTransitionSequenceEnded);

	SequencePlayer->Play();

	return true;
}

void UACBossPhaseComponent::OnTransitionSequenceEnded()
{
	if (bTransitionSequenceEndHandled)
	{
		return;
	}

	bTransitionSequenceEndHandled = true;

	// 전환이 이미 끝난 뒤(안전망 타이머 등)에 뒤늦게 들어온 종료 통보는 무시한다
	if (!bPhaseTransitionInProgress)
	{
		return;
	}

	CleanupTransitionSequence();
	StartTransitionAbilityStep();
}

void UACBossPhaseComponent::StartTransitionAbilityStep()
{
	const FACBossPhaseTransition* Transition = PhaseTransitions.IsValidIndex(PendingTransitionIndex) ? &PhaseTransitions[PendingTransitionIndex] : nullptr;
	UAbilitySystemComponent* ASC = CachedASC.Get();

	// 어빌리티가 설정되지 않았거나 ASC가 없으면 진행이 막히지 않도록 곧바로 완료 처리한다
	if (!Transition || !Transition->TransitionAbility || !ASC)
	{
		CompletePhaseTransition();
		return;
	}

	// 이미 부여된 어빌리티면 그 Spec을 쓰고, 없으면 이 전환을 위해 임시로 부여한다
	if (const FGameplayAbilitySpec* ExistingSpec = ASC->FindAbilitySpecFromClass(Transition->TransitionAbility))
	{
		TransitionAbilityHandle = ExistingSpec->Handle;
	}
	else
	{
		TransitionAbilityHandle = ASC->GiveAbility(FGameplayAbilitySpec(Transition->TransitionAbility, 1, INDEX_NONE, GetOwner()));
		bGrantedTransitionAbility = TransitionAbilityHandle.IsValid();
	}

	if (!TransitionAbilityHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossPhase] TransitionAbility를 부여하지 못했습니다. 전환을 즉시 완료합니다: %s"), *GetNameSafe(GetOwner()));
		CompletePhaseTransition();
		return;
	}

	// 종료 구독을 활성화보다 먼저 건다 — 어빌리티가 같은 콜스택에서 즉시 끝나는 경우도 놓치지 않는다
	AbilityEndedHandle = ASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnTransitionAbilityEnded);

	if (!ASC->TryActivateAbility(TransitionAbilityHandle))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossPhase] TransitionAbility 활성화에 실패했습니다. 전환을 즉시 완료합니다: %s"), *GetNameSafe(GetOwner()));
		CompletePhaseTransition();
		return;
	}

	// 활성화 도중 동기적으로 끝나버린 경우를 잡는다. 이미 완료됐다면 CompletePhaseTransition이 스스로 무시한다
	const FGameplayAbilitySpec* ActivatedSpec = ASC->FindAbilitySpecFromHandle(TransitionAbilityHandle);
	if (!ActivatedSpec || !ActivatedSpec->IsActive())
	{
		CompletePhaseTransition();
	}
}

void UACBossPhaseComponent::OnTransitionAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (EndedData.AbilitySpecHandle != TransitionAbilityHandle)
	{
		return;
	}

	CompletePhaseTransition();
}

void UACBossPhaseComponent::CompletePhaseTransition()
{
	// bPhaseTransitionInProgress가 완료 처리의 1회성 보장 플래그다.
	// 컷신 스킵·어빌리티 종료·안전망 타이머가 겹쳐 들어와도 아래 본문은 정확히 한 번만 실행된다
	if (!bPhaseTransitionInProgress)
	{
		return;
	}

	bPhaseTransitionInProgress = false;

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TransitionFailsafeTimerHandle);
	}

	CleanupTransitionSequence();
	CleanupTransitionAbility();

	// 체력 회복 — 전환 어빌리티가 MaxHealth를 올린 뒤이므로 올라간 최대치를 기준으로 채워진다
	if (PhaseTransitions.IsValidIndex(PendingTransitionIndex))
	{
		RestoreHealth(PhaseTransitions[PendingTransitionIndex].RestoreHealthPercent);
	}

	// 임시 상태 태그 제거 — BeginPhaseTransition에서 부여한 것과 정확히 대칭이다.
	// AI를 재개하기 전에 지워야, 다시 돌기 시작한 BT/StateTree가 "아직 전환 중"인 태그를 보고 판단하지 않는다
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->RemoveLooseGameplayTag(ACGameplayTags::Enemy_Status_PhaseTransition);
		ASC->RemoveLooseGameplayTag(ACGameplayTags::Shared_Status_Invincible);
	}

	// AI와 전투 행동 재개
	ResumeAILogic();

	PendingTransitionIndex = INDEX_NONE;
	++CurrentPhase;

	OnPhaseTransitionCompleted.Broadcast(CurrentPhase);
}

void UACBossPhaseComponent::OnTransitionFailsafeElapsed()
{
	if (!bPhaseTransitionInProgress)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BossPhase] 페이즈 전환이 %.1f초 안에 끝나지 않아 강제로 완료합니다: %s"), MaxTransitionDuration, *GetNameSafe(GetOwner()));

	CompletePhaseTransition();
}

void UACBossPhaseComponent::RestoreHealth(float RestorePercent)
{
	if (RestorePercent <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CachedASC.Get();
	const UACAttributeSet* AttributeSet = ASC ? ASC->GetSet<UACAttributeSet>() : nullptr;
	if (!AttributeSet)
	{
		return;
	}

	const float MaxHealth = AttributeSet->GetMaxHealth();
	if (MaxHealth <= 0.f)
	{
		return;
	}

	const float NewHealth = FMath::Clamp(MaxHealth * RestorePercent, 0.f, MaxHealth);
	ASC->SetNumericAttributeBase(UACAttributeSet::GetHealthAttribute(), NewHealth);

	// Attribute를 직접 세팅하면 PostGameplayEffectExecute를 거치지 않아 체력 위젯이 갱신되지 않는다.
	// GE를 통한 회복과 같은 화면 결과를 내기 위해 여기서 직접 브로드캐스트한다
	if (IPawnUIInterface* UIInterface = Cast<IPawnUIInterface>(GetOwner()))
	{
		if (UPawnUIComponent* PawnUIComponent = UIInterface->GetPawnUIComponent())
		{
			PawnUIComponent->OnCurrentHealthChanged.Broadcast(NewHealth / MaxHealth);
		}
	}
}

void UACBossPhaseComponent::CleanupTransitionSequence()
{
	if (ULevelSequencePlayer* SequencePlayer = ActiveSequencePlayer.Get())
	{
		SequencePlayer->OnFinished.RemoveAll(this);
		SequencePlayer->OnStop.RemoveAll(this);

		if (SequencePlayer->IsPlaying())
		{
			SequencePlayer->Stop();
		}
	}

	// 시퀀스 액터는 CreateLevelSequencePlayer가 스폰한 것이므로 우리가 회수한다.
	// 플레이어 자신의 이벤트 콜스택 안에서 파괴하지 않도록 다음 틱으로 미룬다
	if (ALevelSequenceActor* SequenceActor = ActiveSequenceActor.Get())
	{
		if (UWorld* World = GetWorld())
		{
			TWeakObjectPtr<ALevelSequenceActor> WeakSequenceActor = SequenceActor;
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda(
				[WeakSequenceActor]()
				{
					if (ALevelSequenceActor* ActorToDestroy = WeakSequenceActor.Get())
					{
						ActorToDestroy->Destroy();
					}
				}));
		}
	}

	ActiveSequencePlayer.Reset();
	ActiveSequenceActor.Reset();
}

void UACBossPhaseComponent::CleanupTransitionAbility()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!ASC)
	{
		AbilityEndedHandle.Reset();
		TransitionAbilityHandle = FGameplayAbilitySpecHandle();
		bGrantedTransitionAbility = false;
		return;
	}

	if (AbilityEndedHandle.IsValid())
	{
		ASC->OnAbilityEnded.Remove(AbilityEndedHandle);
		AbilityEndedHandle.Reset();
	}

	// StartupData가 부여한 어빌리티는 건드리지 않고, 이 전환에서 임시로 부여한 것만 회수한다
	if (bGrantedTransitionAbility && TransitionAbilityHandle.IsValid())
	{
		ASC->ClearAbility(TransitionAbilityHandle);
	}

	TransitionAbilityHandle = FGameplayAbilitySpecHandle();
	bGrantedTransitionAbility = false;
}

void UACBossPhaseComponent::PauseAILogic()
{
	if (AAIController* AIController = GetOwningAIController())
	{
		AIController->StopMovement();
	}

	if (UBrainComponent* BrainComponent = GetOwningBrainComponent())
	{
		BrainComponent->PauseLogic(TEXT("BossPhaseTransition"));
	}
}

void UACBossPhaseComponent::ResumeAILogic()
{
	if (UBrainComponent* BrainComponent = GetOwningBrainComponent())
	{
		BrainComponent->ResumeLogic(TEXT("BossPhaseTransition"));
	}
}

AAIController* UACBossPhaseComponent::GetOwningAIController() const
{
	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	return OwningPawn ? Cast<AAIController>(OwningPawn->GetController()) : nullptr;
}

UBrainComponent* UACBossPhaseComponent::GetOwningBrainComponent() const
{
	AAIController* AIController = GetOwningAIController();
	if (!AIController)
	{
		return nullptr;
	}

	if (AIController->BrainComponent)
	{
		return AIController->BrainComponent;
	}

	// StateTree 보스: UStateTreeComponent는 UBrainComponent를 상속하지만 AAIController::BrainComponent 슬롯에 등록되지 않는다
	return AIController->FindComponentByClass<UBrainComponent>();
}
