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

	// UStateTreeComponent는 UBrainComponent 초기화를 건너뛰어 AAIController::BrainComponent에 등록되지 않는다.
	// 따라서 언포제스 시 자동으로 정지하지 않으므로 여기서 명시적으로 중단한다
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

	// 사망/체간 붕괴/공격 중/페이즈 전환 중이면 예고를 무시한다
	if (ASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead)
		|| ASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_PostureBroken)
		|| ASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_Status_Attacking)
		|| ASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_State_Phase2))
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