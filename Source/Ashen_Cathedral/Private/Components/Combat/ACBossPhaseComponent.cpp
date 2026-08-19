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
#include "Components/UI/EnemyUIComponent.h"
#include "Components/UI/PawnUIComponent.h"
#include "Components/UI/PlayerUIComponent.h"
#include "Components/WidgetComponent.h"
#include "Controllers/ACStateTreeController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "Interfaces/PawnUIInterface.h"
#include "Kismet/GameplayStatics.h"

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

bool UACBossPhaseComponent::WillGrantPhaseStateTagOnPendingTransition() const
{
	return PhaseTransitions.IsValidIndex(PendingTransitionIndex) && PhaseTransitions[PendingTransitionIndex].PhaseStateTag.IsValid();
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

#if !UE_BUILD_SHIPPING
bool UACBossPhaseComponent::DebugForceNextPhase()
{
	if (!HasAuthority() || bFinalDeathStarted || bPhaseTransitionInProgress)
	{
		return false;
	}

	const int32 NextIndex = GetNextTransitionIndex();
	if (NextIndex == INDEX_NONE)
	{
		return false;
	}

	BeginPhaseTransition(NextIndex);
	return true;
}
#endif

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
	StopAILogicForTransition();

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

	// 컷신이 첫 프레임부터 깨끗하게 보이도록 재생 직전에 HUD와 보스 체력바를 걷어낸다
	SetTransitionUIVisible(false);

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

	// 어빌리티 단계는 다음 틱으로 미룬다. 이 함수는 시퀀스 플레이어의 종료 콜스택 안에서 불리며,
	// 그 안에서 전환을 끝내고 AI를 재개하면 뒤이어 도는 시퀀서의 Restore State가 방금 시작된 행동과
	// AnimInstance를 되돌려 버려 보스가 굳은 채 남는다. 시퀀스 액터 파괴를 다음 틱으로 미루는 것과 같은 이유다.
	// 전환 어빌리티에 몽타주가 없으면 활성화와 동시에 완료 처리까지 이 콜스택에서 끝나므로 특히 문제가 된다
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::StartTransitionAbilityStep);
		return;
	}

	StartTransitionAbilityStep();
}

void UACBossPhaseComponent::StartTransitionAbilityStep()
{
	// 다음 틱으로 미뤄진 사이에 안전망 타이머가 전환을 끝냈을 수 있다 — 뒤늦게 어빌리티를 켜지 않는다
	if (!bPhaseTransitionInProgress)
	{
		return;
	}

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

	// 활성화 도중 동기적으로 끝나버린 경우를 잡는다. 이미 예약됐다면 CompletePhaseTransition이 스스로 무시한다
	const FGameplayAbilitySpec* ActivatedSpec = ASC->FindAbilitySpecFromHandle(TransitionAbilityHandle);
	if (!ActivatedSpec || !ActivatedSpec->IsActive())
	{
		RequestCompletePhaseTransition();
	}
}

void UACBossPhaseComponent::OnTransitionAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (EndedData.AbilitySpecHandle != TransitionAbilityHandle)
	{
		return;
	}

	RequestCompletePhaseTransition();
}

void UACBossPhaseComponent::RequestCompletePhaseTransition()
{
	if (!bPhaseTransitionInProgress)
	{
		return;
	}

	// 전환 어빌리티에 몽타주가 없으면 ActivateAbility가 EndAbility까지 동기로 끝내, OnAbilityEnded가
	// TryActivateAbility 콜스택 '안에서' 터진다. 거기서 완료 처리를 하면 어빌리티가 아직 활성화 중인 상태로
	// 종료 구독 해제·Spec 회수·태그 제거·AI 재개가 전부 돌아 ASC와 AI가 서로 어긋난 상태를 본다.
	// 실제로 로그에서도 "전환 완료"가 "어빌리티 활성화 성공"보다 먼저 찍혀 순서가 뒤집혀 있었다
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::CompletePhaseTransition);
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

	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		// 새 페이즈의 영구 상태 태그를 부여한다. 누적이므로 이전 페이즈 태그는 지우지 않는다 —
		// 그래야 "N페이즈 이상"을 태그 하나로 물을 수 있고, GAS의 태그 기반 게이트도 같은 값을 본다
		if (PhaseTransitions.IsValidIndex(PendingTransitionIndex))
		{
			const FGameplayTag& PhaseStateTag = PhaseTransitions[PendingTransitionIndex].PhaseStateTag;
			if (PhaseStateTag.IsValid() && !ASC->HasMatchingGameplayTag(PhaseStateTag))
			{
				ASC->AddLooseGameplayTag(PhaseStateTag);
			}
		}

		// 임시 상태 태그 제거 — BeginPhaseTransition에서 부여한 것과 정확히 대칭이다.
		// AI를 재개하기 전에 지워야, 다시 돌기 시작한 BT/StateTree가 "아직 전환 중"인 태그를 보고 판단하지 않는다
		ASC->RemoveLooseGameplayTag(ACGameplayTags::Enemy_Status_PhaseTransition);
		ASC->RemoveLooseGameplayTag(ACGameplayTags::Shared_Status_Invincible);
	}

	// AI와 전투 행동 재개
	RestartAILogicAfterTransition();

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

	// 컷신이 끝났으니 UI를 되돌린다. 컷신 뒤 전환 몽타주가 남아 있어도 그때는 게임플레이 카메라이므로 HUD가 보여야 한다.
	// 정상 종료·스킵·안전망·보스 파괴 어느 경로로 들어와도 이 함수를 지나므로 HUD가 숨은 채 남지 않는다
	SetTransitionUIVisible(true);

	ActiveSequencePlayer.Reset();
	ActiveSequenceActor.Reset();
}

void UACBossPhaseComponent::SetTransitionUIVisible(bool bVisible)
{
	// 숨긴 적이 없는데 복원하지 않는다 — 다른 연출(조우 컷신 등)이 걸어 둔 숨김 상태를 멋대로 되돌리게 된다
	if (bTransitionUIHidden == !bVisible)
	{
		return;
	}

	bTransitionUIHidden = !bVisible;

	// 플레이어 오버레이는 BP가 뷰포트에 올리므로 RegisterHUDWidget으로 등록된 위젯을 통째로 토글한다
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (IPawnUIInterface* PlayerUIInterface = Cast<IPawnUIInterface>(PlayerPawn))
	{
		if (UPlayerUIComponent* PlayerUIComponent = Cast<UPlayerUIComponent>(PlayerUIInterface->GetPawnUIComponent()))
		{
			PlayerUIComponent->SetHUDVisible(bVisible);
		}
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// 보스 체력바는 보스에 붙은 WidgetComponent가 그리므로 컴포넌트 자체를 토글해야 한다
	TArray<UWidgetComponent*> WidgetComponents;
	OwnerActor->GetComponents<UWidgetComponent>(WidgetComponents);
	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		WidgetComponent->SetVisibility(bVisible);
	}

	// BP가 별도로 등록해 둔 적 UI가 있으면 함께 처리한다
	if (UEnemyUIComponent* EnemyUIComponent = OwnerActor->FindComponentByClass<UEnemyUIComponent>())
	{
		EnemyUIComponent->SetEnemyWidgetsVisible(bVisible);
	}
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

void UACBossPhaseComponent::StopAILogicForTransition()
{
	if (AAIController* AIController = GetOwningAIController())
	{
		AIController->StopMovement();
	}

	// PauseLogic은 틱만 끄고 트리는 살려 두기 때문에 컷신 도중에 상태가 진입하는 일이 있었다.
	// 아예 멈춰 두고 전환이 끝나면 다시 시작하는 편이 컷신 구간의 행동을 확실히 막고,
	// 재개 경로도 조우 컷신(StartEncounter)과 같은 모양이 되어 다루기 쉽다
	if (UBrainComponent* BrainComponent = GetOwningBrainComponent())
	{
		BrainComponent->StopLogic(TEXT("BossPhaseTransition"));
	}

	// 컷신 동안에는 CharacterMovement도 재운다.
	// 시퀀서는 액터 틱보다 먼저 돌면서(LevelTick.cpp의 MovieSceneSequenceTick) Transform 트랙 값을
	// 루트 컴포넌트(= 캡슐)에 직접 써 넣는데, 그 뒤 CharacterMovement가 AdjustFloorHeight로 캡슐을
	// 바닥에서 일정 높이(MIN/MAX_FLOOR_DIST 사이)로 되밀어 올린다. 매 프레임 둘이 번갈아 밀어
	// 보스가 밀리미터 단위로 떨린다. MOVE_None이면 PerformMovement가 즉시 반환해 쓰는 주체가 시퀀서 하나로 줄어든다
	if (const ACharacter* OwningCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = OwningCharacter->GetCharacterMovement())
		{
			PreTransitionMovementMode = Movement->MovementMode;
			bDisabledMovementForTransition = true;

			// 남은 속도를 끊지 않으면 이동을 되살리는 순간 그만큼 미끄러진다
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}
}

void UACBossPhaseComponent::RestartAILogicAfterTransition()
{
	// AI를 깨우기 전에 이동부터 되돌린다 — 이동이 꺼진 채로 트리가 시작하면 첫 이동 태스크가 헛돈다
	if (bDisabledMovementForTransition)
	{
		bDisabledMovementForTransition = false;

		if (const ACharacter* OwningCharacter = Cast<ACharacter>(GetOwner()))
		{
			if (UCharacterMovementComponent* Movement = OwningCharacter->GetCharacterMovement())
			{
				Movement->SetMovementMode(PreTransitionMovementMode);
			}
		}
	}

	if (UBrainComponent* BrainComponent = GetOwningBrainComponent())
	{
		BrainComponent->RestartLogic();
	}

	// 트리를 다시 시작하면 대기 상태에서 출발한다. Combat 진입은 TargetAcquired 이벤트가 필수 조건이므로
	// 잡아둔 타겟을 다시 알려야 한다 — 플레이어가 계속 시야에 있으면 Perception은 새 이벤트를 보내지 않는다.
	// 반드시 재시작 뒤에 보낸다: SendStateTreeEvent는 트리가 돌고 있지 않으면 이벤트를 버린다
	if (AACStateTreeController* StateTreeController = Cast<AACStateTreeController>(GetOwningAIController()))
	{
		StateTreeController->ResendTargetAcquiredEvent();
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
