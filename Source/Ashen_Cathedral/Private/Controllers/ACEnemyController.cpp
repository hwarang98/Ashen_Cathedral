// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/ACEnemyController.h"

#include "ACGameplayTags.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AACEnemyController::AACEnemyController()
{
	AISenseConfig_Sight = CreateDefaultSubobject<UAISenseConfig_Sight>("Enemy SenseConfig Sight");
	AISenseConfig_Sight->DetectionByAffiliation.bDetectEnemies = true;     // 적 감지 활성화
	AISenseConfig_Sight->DetectionByAffiliation.bDetectFriendlies = false; // 아군 감지 비활성화
	AISenseConfig_Sight->DetectionByAffiliation.bDetectNeutrals = false;   // 중립 감지 비활성화
	AISenseConfig_Sight->SightRadius = 3000.f;                             // 시야 범위 (감지 거리)
	AISenseConfig_Sight->LoseSightRadius = 3500.f;                         // 시야 상실 범위 (추적 해제 거리)
	AISenseConfig_Sight->PeripheralVisionAngleDegrees = 180.f;             // 주변 시야각

	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>("EnemyPerceptionComponent");
	EnemyPerceptionComponent->ConfigureSense(*AISenseConfig_Sight);
	EnemyPerceptionComponent->SetDominantSense(UAISenseConfig_Sight::StaticClass());
	EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ThisClass::OnEnemyPerceptionUpdated);
}


void AACEnemyController::BeginPlay()
{
	Super::BeginPlay();
	SetGenericTeamId(FGenericTeamId(1));
}

void AACEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CachedEnemyCharacter = Cast<AACEnemyCharacter>(InPawn);

	if (UACAbilitySystemComponent* ASC = CachedEnemyCharacter ? CachedEnemyCharacter->GetACAbilitySystemComponent() : nullptr)
	{
		ASC->RegisterGameplayTagEvent(ACGameplayTags::Enemy_State_PressureReady, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnPressureReadyTagChanged);
		ASC->GenericGameplayEventCallbacks.FindOrAdd(ACGameplayTags::Shared_Event_Combat_IncomingAttack).AddUObject(this, &ThisClass::OnIncomingAttackEventReceived);
	}
}

ETeamAttitude::Type AACEnemyController::GetTeamAttitudeTowards(const AActor& Other) const
{
	const APawn* PawnToCheck = Cast<const APawn>(&Other);

	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(PawnToCheck->GetController());

	// 나보다 "작은 숫자"의 팀만 적대적
	if (OtherTeamAgent && OtherTeamAgent->GetGenericTeamId() < GetGenericTeamId())
	{
		return ETeamAttitude::Hostile;
	}

	return ETeamAttitude::Friendly;
}

void AACEnemyController::OnEnemyPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
		if (!BlackboardComponent->GetValueAsObject(FName("TargetActor")))
		{
			if (Stimulus.WasSuccessfullySensed() && Actor)
			{
				BlackboardComponent->SetValueAsObject(FName("TargetActor"), Actor);
			}
		}
	}
}

void AACEnemyController::OnPressureReadyTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
		BlackboardComponent->SetValueAsBool(FName("bPressureResponseRequested"), NewCount > 0);
	}
}

void AACEnemyController::OnIncomingAttackEventReceived(const FGameplayEventData* Payload)
{
	UACAbilitySystemComponent* ASC = CachedEnemyCharacter ? CachedEnemyCharacter->GetACAbilitySystemComponent() : nullptr;
	if (!Payload || !ASC)
	{
		return;
	}

	// 사망/체간 붕괴/공격 중/페이즈 전환 중이면 예고를 무시한다 (ACPressureDetectionComponent와 동일한 가드 스타일)
	// 전환 판정에는 전환 연출 동안만 유지되는 Enemy.Status.PhaseTransition을 쓴다 — 영구 상태인 Enemy.State.Phase2로 막으면
	// 전환이 끝난 뒤에도 2페이즈 내내 예고가 차단되어 방어/회피 대응이 발동하지 않는다.
	if (ASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_Dead)
		|| ASC->HasMatchingGameplayTag(ACGameplayTags::Shared_Status_PostureBroken)
		|| ASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_Status_Attacking)
		|| ASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_Status_PhaseTransition)
		|| ASC->HasMatchingGameplayTag(ACGameplayTags::Enemy_Status_Phase2))
	{
		ResetIncomingAttackBlackboard();
		return;
	}

	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return;
	}

	const bool bParryable = Payload->InstigatorTags.HasTag(ACGameplayTags::Shared_Attack_Parryable) && !Payload->InstigatorTags.HasTag(ACGameplayTags::Shared_Attack_Unparryable);
	const bool bBlockable = Payload->InstigatorTags.HasTag(ACGameplayTags::Shared_Attack_Blockable) && !Payload->InstigatorTags.HasTag(ACGameplayTags::Shared_Attack_Unblockable);

	BlackboardComponent->SetValueAsObject(FName("IncomingAttackActor"), const_cast<AActor*>(Payload->Instigator.Get()));
	BlackboardComponent->SetValueAsFloat(FName("IncomingAttackTimeToImpact"), Payload->EventMagnitude);
	BlackboardComponent->SetValueAsBool(FName("bIncomingAttackParryable"), bParryable);
	BlackboardComponent->SetValueAsBool(FName("bIncomingAttackBlockable"), bBlockable);

	const int32 WarningId = ++IncomingAttackWarningId;
	const float ClearDelay = FMath::Max(static_cast<float>(Payload->EventMagnitude) + IncomingAttackBlackboardGraceTime, 0.01f);
	FTimerDelegate ClearDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::ClearIncomingAttackBlackboard, WarningId);
	GetWorldTimerManager().SetTimer(IncomingAttackClearTimerHandle, ClearDelegate, ClearDelay, false);
}

void AACEnemyController::ClearIncomingAttackBlackboard(int32 ExpectedWarningId)
{
	if (ExpectedWarningId != IncomingAttackWarningId)
	{
		return;
	}

	ResetIncomingAttackBlackboard();
}

void AACEnemyController::ResetIncomingAttackBlackboard()
{
	GetWorldTimerManager().ClearTimer(IncomingAttackClearTimerHandle);

	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
		BlackboardComponent->ClearValue(FName("IncomingAttackActor"));
		BlackboardComponent->SetValueAsFloat(FName("IncomingAttackTimeToImpact"), 0.f);
		BlackboardComponent->SetValueAsBool(FName("bIncomingAttackParryable"), false);
		BlackboardComponent->SetValueAsBool(FName("bIncomingAttackBlockable"), false);
	}
}
