// 보스 아레나에 배치해 클리어 후 상호작용하면 다음 스테이지로 진행시키는 액터


#include "GameModes/ACStageExitPoint.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "GameModes/ACGameMode.h"

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

	// GameMode를 호출하기 전에 아레나의 모든 출구를 닫는다 — 다음 보스가 같은 맵에 스폰되므로,
	// 자신만 닫으면 같은 브로드캐스트로 함께 열린 형제 출구(예: 로비 복귀)가 남아
	// 다음 전투 중에 보스를 건너뛰고 나갈 수 있다.
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

	// 최종 보스 뒤에는 이어질 스테이지가 없으므로 진행용 출구는 열지 않는다 — 로비 출구만 남는다
	if (bInIsFinalBoss && Destination == EACStageExitDestination::NextStage)
	{
		return;
	}

	SetActivated(true);

	// 플레이어가 이미 출구 범위 안에 서 있는 채로 보스가 죽은 경우에도 BeginOverlap이 발생하도록 강제 갱신
	InteractionSphere->UpdateOverlaps();

	BP_OnActivated();
}

void AACStageExitPoint::Deactivate(APawn* InstigatorPawn)
{
	SetActivated(false);

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
