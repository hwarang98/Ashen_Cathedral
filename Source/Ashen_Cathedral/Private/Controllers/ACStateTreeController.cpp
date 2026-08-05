// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/ACStateTreeController.h"
#include "ACGameplayTags.h"
#include "Components/StateTreeAIComponent.h"
#include "StructUtils/StructView.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AACStateTreeController::AACStateTreeController()
{
	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>("StateTreeAIComponent");

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>("Boss SenseConfig Sight");
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;     // 적 감지 활성화
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false; // 아군 감지 비활성화
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;   // 중립 감지 비활성화

	BossPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>("BossPerceptionComponent");
	BossPerceptionComponent->ConfigureSense(*SightConfig);
	BossPerceptionComponent->SetDominantSense(UAISenseConfig_Sight::StaticClass());
	BossPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ThisClass::OnPerceptionUpdated);
}

void AACStateTreeController::BeginPlay()
{
	Super::BeginPlay();

	SetGenericTeamId(FGenericTeamId(TeamId));
	ApplyPerceptionSettings();
}

void AACStateTreeController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CachedEnemyCharacter = Cast<AACEnemyCharacter>(InPawn);

	if (UACAbilitySystemComponent* ASC = CachedEnemyCharacter ? CachedEnemyCharacter->GetACAbilitySystemComponent() : nullptr)
	{
		ASC->RegisterGameplayTagEvent(ACGameplayTags::Enemy_State_PressureReady, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnPressureReadyTagChanged);
		ASC->GenericGameplayEventCallbacks.FindOrAdd(ACGameplayTags::Shared_Event_Combat_IncomingAttack).AddUObject(this, &ThisClass::OnIncomingAttackEventReceived);
	}

	// 런타임 스폰 경로에서는 SpawnDefaultController가 컨트롤러를 스폰하는 시점에 BeginPlay가 즉시 발화하고,
	// Possess는 그 뒤에 온다(Pawn.cpp:377-382). 그래서 StateTree가 Pawn 없이 StartLogic을 돌다가
	// 스키마의 Actor 컨텍스트를 못 채워 실패하고 틱까지 꺼버린다(StateTreeComponentSchema.cpp:138).
	// bStartAILogicOnPossess가 기본 false라 엔진은 재시도하지 않으므로 여기서 직접 다시 시작한다.
	// 레벨에 배치된 경우에는 아직 BeginPlay가 오지 않았으므로 건드리지 않는다 — 그대로 두면 자동 시작이 정상 동작하고,
	// 여기서 시작해버리면 이후 BeginPlay의 StartLogic과 겹쳐 트리가 두 번 진입한다.
	if (StateTreeAIComponent && HasActorBegunPlay() && !StateTreeAIComponent->IsRunning())
	{
		StateTreeAIComponent->RestartLogic();
	}
}

void AACStateTreeController::OnUnPossess()
{
	if (UACAbilitySystemComponent* ASC = CachedEnemyCharacter ? CachedEnemyCharacter->GetACAbilitySystemComponent() : nullptr)
	{
		ASC->RegisterGameplayTagEvent(ACGameplayTags::Enemy_State_PressureReady, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);

		if (FGameplayEventMulticastDelegate* EventDelegate = ASC->GenericGameplayEventCallbacks.Find(ACGameplayTags::Shared_Event_Combat_IncomingAttack))
		{
			EventDelegate->RemoveAll(this);
		}
	}

	// Super::OnUnPossess가 bStopAILogicOnUnposses 경로로 CleanupBrainComponent()를 부르지만,
	// 그 시점에는 CachedEnemyCharacter가 이미 정리된 뒤라 순서를 보장하려고 여기서 먼저 중단한다.
	// (StopLogic은 bIsRunning 가드가 있어 뒤따르는 엔진 호출은 무시된다)
	if (StateTreeAIComponent)
	{
		StateTreeAIComponent->StopLogic(TEXT("UnPossess"));
	}

	CachedEnemyCharacter = nullptr;
	TargetActor = nullptr;

	Super::OnUnPossess();
}

ETeamAttitude::Type AACStateTreeController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const APawn* PawnToCheck = Cast<const APawn>(&Other);
	if (!PawnToCheck)
	{
		return ETeamAttitude::Neutral;
	}

	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(PawnToCheck->GetController());

	// 나보다 "작은 숫자"의 팀만 적대적
	if (OtherTeamAgent && OtherTeamAgent->GetGenericTeamId() < GetGenericTeamId())
	{
		return ETeamAttitude::Hostile;
	}

	return ETeamAttitude::Friendly;
}

void AACStateTreeController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !StateTreeAIComponent)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// 이미 타겟이 있으면 교체하지 않는다 (보스전은 단일 타겟 전제)
		if (TargetActor)
		{
			return;
		}

		TargetActor = Actor;
		StateTreeAIComponent->SendStateTreeEvent(ACGameplayTags::Enemy_StateTree_Event_TargetAcquired);
		return;
	}

	if (TargetActor == Actor)
	{
		TargetActor = nullptr;
		StateTreeAIComponent->SendStateTreeEvent(ACGameplayTags::Enemy_StateTree_Event_TargetLost);
	}
}

void AACStateTreeController::OnPressureReadyTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 태그가 제거될 때는 이벤트를 보내지 않는다. 반응 종료는 StateTree 상태가 스스로 판단한다
	if (NewCount > 0 && StateTreeAIComponent)
	{
		StateTreeAIComponent->SendStateTreeEvent(ACGameplayTags::Enemy_StateTree_Event_PressureReady);
	}
}

void AACStateTreeController::OnIncomingAttackEventReceived(const FGameplayEventData* Payload)
{
	UACAbilitySystemComponent* ASC = CachedEnemyCharacter ? CachedEnemyCharacter->GetACAbilitySystemComponent() : nullptr;
	if (!Payload || !ASC || !StateTreeAIComponent)
	{
		return;
	}

	// 사망/체간 붕괴/공격 중/페이즈 전환 중이면 예고를 무시한다.
	// 전환 판정에는 어빌리티 수명에 묶인 Enemy.Status.Phase2를 쓴다 — 영구 상태인 Enemy.State.Phase2로 막으면
	// 전환이 끝난 뒤에도 2페이즈 내내 예고가 차단되어 방어/회피 전이가 발동하지 않는다.
	if (ASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead)
		|| ASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_PostureBroken)
		|| ASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_Status_Attacking)
		|| ASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_Status_PhaseTransition)
		|| ASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_Status_Phase2))
	{
		return;
	}

	FACIncomingAttackStateTreePayload EventPayload;
	EventPayload.Instigator = const_cast<AActor*>(Payload->Instigator.Get());
	EventPayload.TimeToImpact = Payload->EventMagnitude;
	EventPayload.bParryable = Payload->InstigatorTags.HasTag(ACGameplayTags::Shared_Attack_Parryable) && !Payload->InstigatorTags.HasTag(ACGameplayTags::Shared_Attack_Unparryable);
	EventPayload.bBlockable = Payload->InstigatorTags.HasTag(ACGameplayTags::Shared_Attack_Blockable) && !Payload->InstigatorTags.HasTag(ACGameplayTags::Shared_Attack_Unblockable);

	StateTreeAIComponent->SendStateTreeEvent(ACGameplayTags::Enemy_StateTree_Event_IncomingAttack, FConstStructView::Make(EventPayload));
}

void AACStateTreeController::ApplyPerceptionSettings()
{
	if (!SightConfig || !BossPerceptionComponent)
	{
		return;
	}

	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;

	BossPerceptionComponent->ConfigureSense(*SightConfig);
	BossPerceptionComponent->RequestStimuliListenerUpdate();
}