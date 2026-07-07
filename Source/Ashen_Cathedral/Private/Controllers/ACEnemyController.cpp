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