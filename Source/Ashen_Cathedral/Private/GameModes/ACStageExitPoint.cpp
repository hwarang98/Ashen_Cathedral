// 보스 아레나에 배치해 클리어 후 상호작용하면 다음 스테이지로 진행시키는 액터


#include "GameModes/ACStageExitPoint.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/RewardCard/ACRewardCardComponent.h"
#include "Components/SphereComponent.h"
#include "DataAssets/Run/ACDataAsset_StageDefinition.h"
#include "EngineUtils.h"
#include "GameModes/ACGameMode.h"
#include "Subsystems/ACRunStateSubsystem.h"
#include "Kismet/GameplayStatics.h"

AACStageExitPoint::AACStageExitPoint()
{
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Sphere"));
	SetRootComponent(InteractionSphere);
	InteractionSphere->SetSphereRadius(150.f);
	InteractionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnInteractionSphereEndOverlap);

	ExitMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Exit Mesh"));
	ExitMesh->SetupAttachment(InteractionSphere);
	ExitMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AACStageExitPoint::BeginPlay()
{
	Super::BeginPlay();

	// 보스를 클리어하기 전에는 존재를 드러내지 않는다.
	SetActivated(false);

	if (AACGameMode* ACGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AACGameMode>() : nullptr)
	{
		if (!ACGameMode->OnBossBattleCompletedDelegate.IsAlreadyBound(this, &ThisClass::HandleBossBattleCompleted))
		{
			ACGameMode->OnBossBattleCompletedDelegate.AddDynamic(this, &ThisClass::HandleBossBattleCompleted);
		}
	}
}

void AACStageExitPoint::Interact(APawn* InstigatorPawn)
{
	if (!bActivated)
	{
		return;
	}

	// 활성화 이후에 정책이 바뀌었을 수 있으므로 진행 직전에 한 번 더 확인한다
	const UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);
	if (RunState && Destination == EACStageExitDestination::NextStage
		&& RunState->GetEffectiveExitPolicy() == EACStageExitPolicy::ForceReturnToLobby)
	{
		return;
	}

	ExecuteExit(InstigatorPawn);
}

void AACStageExitPoint::ExecuteExit(APawn* InstigatorPawn)
{
	// GameMode를 호출하기 전에 아레나의 모든 출구를 닫는다 — 자신만 닫으면 같은 브로드캐스트로 함께 열린
	// 형제 출구(예: 로비 복귀)가 남아 이동이 확정된 뒤에도 다시 상호작용될 수 있다.
	for (TActorIterator<AACStageExitPoint> It(GetWorld()); It; ++It)
	{
		It->Deactivate(InstigatorPawn);
	}

	AACGameMode* ACGameMode = GetWorld()->GetAuthGameMode<AACGameMode>();
	if (!ACGameMode)
	{
		return;
	}

	if (Destination == EACStageExitDestination::ReturnToLobby)
	{
		ACGameMode->RequestReturnToLobby();
		return;
	}

	ACGameMode->RequestProgressAfterBossClear();
}

FText AACStageExitPoint::GetInteractionText() const
{
	if (Destination == EACStageExitDestination::ReturnToLobby)
	{
		// 최종 보스를 잡은 뒤에는 어차피 로비로만 나갈 수 있으므로 마무리 문구를 쓴다
		return bIsFinalBoss ? FinalStageInteractionText : ReturnToLobbyInteractionText;
	}

	return InteractionText;
}

void AACStageExitPoint::HandleBossBattleCompleted(bool bInIsFinalBoss)
{
	bIsFinalBoss = bInIsFinalBoss;

	const UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);
	const EACStageExitPolicy ExitPolicy = RunState ? RunState->GetEffectiveExitPolicy() : EACStageExitPolicy::NormalChoice;

	if (Destination == EACStageExitDestination::NextStage)
	{
		// 최종 스테이지 뒤에는 이어질 스테이지가 없으므로 진행용 출구는 열지 않는다 — 로비 출구만 남는다
		if (bInIsFinalBoss || ExitPolicy == EACStageExitPolicy::ForceReturnToLobby)
		{
			return;
		}

		if (ExitPolicy == EACStageExitPolicy::AutoNextStage)
		{
			// 선택지를 노출하지 않고 자동으로 넘어간다. 보상 카드 선택이 열려 있으면 그것이 끝나기를 먼저 기다린다
			if (bAutoAdvanceScheduled)
			{
				return;
			}
			bAutoAdvanceScheduled = true;

			const AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
			UACRewardCardComponent* RewardCardComponent = PlayerCharacter ? PlayerCharacter->GetRewardCardComponent() : nullptr;

			// 카드 UI는 보스 사망 '시작' 시점에 열리므로, 지금 열려 있지 않다면 이번 스테이지에 카드 보상이 없다는 뜻이다
			if (RewardCardComponent && RewardCardComponent->IsSelectionActive())
			{
				RewardCardComponent->OnCardSelectionFinishedDelegate.AddUniqueDynamic(this, &ThisClass::HandleCardSelectionFinished);
				return;
			}

			StartAutoAdvanceTimer();
			return;
		}
	}
	else if (ExitPolicy == EACStageExitPolicy::AutoNextStage)
	{
		// 자동 진행 스테이지에서는 선택지 자체를 주지 않으므로 로비 출구도 열지 않는다
		return;
	}

	SetActivated(true);

	// 플레이어가 이미 출구 범위 안에 서 있는 채로 보스가 죽은 경우에도 BeginOverlap이 발생하도록 강제 갱신
	InteractionSphere->UpdateOverlaps();

	BP_OnActivated();
}

void AACStageExitPoint::HandleCardSelectionFinished()
{
	if (const AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		if (UACRewardCardComponent* RewardCardComponent = PlayerCharacter->GetRewardCardComponent())
		{
			RewardCardComponent->OnCardSelectionFinishedDelegate.RemoveDynamic(this, &ThisClass::HandleCardSelectionFinished);
		}
	}

	StartAutoAdvanceTimer();
}

void AACStageExitPoint::StartAutoAdvanceTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);
	const UACDataAsset_StageDefinition* Stage = RunState ? RunState->GetCurrentStage() : nullptr;
	const float Delay = Stage ? Stage->AutoAdvanceDelay : 0.f;

	if (Delay <= 0.f)
	{
		// 같은 프레임에 바로 트래블하면 이 브로드캐스트를 아직 처리하지 못한 형제 액터가 생기므로 다음 틱으로 미룬다
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleAutoAdvance);
		return;
	}

	World->GetTimerManager().SetTimer(AutoAdvanceTimerHandle, this, &ThisClass::HandleAutoAdvance, Delay, false);
}

void AACStageExitPoint::HandleAutoAdvance()
{
	// 대기 중에 사망·런 종료·정책 변경이 있었을 수 있으므로 진행 직전에 전부 재확인한다
	const UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);
	if (!RunState || !RunState->IsRunActive() || !RunState->IsCurrentStageCleared() || !RunState->HasNextStage()
		|| RunState->GetEffectiveExitPolicy() != EACStageExitPolicy::AutoNextStage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACStageExitPoint] 자동 진행 조건이 더 이상 성립하지 않아 취소했습니다."));
		return;
	}

	ExecuteExit(nullptr);
}

void AACStageExitPoint::CancelAutoAdvance()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);
	}

	if (const AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		if (UACRewardCardComponent* RewardCardComponent = PlayerCharacter->GetRewardCardComponent())
		{
			RewardCardComponent->OnCardSelectionFinishedDelegate.RemoveDynamic(this, &ThisClass::HandleCardSelectionFinished);
		}
	}
}

void AACStageExitPoint::Deactivate(APawn* InstigatorPawn)
{
	SetActivated(false);

	// 이동이 확정된 뒤에 타이머가 늦게 발화하지 않도록 자동 진행 예약을 정리한다
	CancelAutoAdvance();

	// 콜리전을 끄면 EndOverlap이 오지만 순서를 보장할 수 없어, 플레이어 쪽 참조와 프롬프트를 명시적으로 정리한다.
	if (AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(InstigatorPawn))
	{
		PlayerCharacter->ClearCurrentInteractable(this);
	}
}

void AACStageExitPoint::SetActivated(bool bInActivated)
{
	bActivated = bInActivated;

	SetActorHiddenInGame(!bInActivated);
	SetActorEnableCollision(bInActivated);
}

void AACStageExitPoint::OnInteractionSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
	)
{
	if (!bActivated)
	{
		return;
	}

	if (AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(OtherActor))
	{
		PlayerCharacter->SetCurrentInteractable(this);
	}
}

void AACStageExitPoint::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(OtherActor))
	{
		PlayerCharacter->ClearCurrentInteractable(this);
	}
}
