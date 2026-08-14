// 아레나 입구에 배치해 플레이어가 밟으면 조우 컷신을 재생하고, 끝나면 보스 AI를 시작시키는 1회용 트리거


#include "GameModes/ACEncounterTrigger.h"
#include "Character/ACCharacterBase.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Controllers/ACStateTreeController.h"
#include "GameModes/ACGameState.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"

AACEncounterTrigger::AACEncounterTrigger()
{
	// 박스를 루트로 두면 액터 원점이 박스 중심이 되어, 바닥에 놓고 키울 때 절반이 바닥을 뚫고 내려간다.
	// 빈 씬 컴포넌트를 루트로 두고 박스를 절반 높이만큼 올려 붙이면 원점이 곧 아랫면이 된다.
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger Box"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetBoxExtent(FVector(200.f, 200.f, 100.f));
	TriggerBox->SetRelativeLocation(FVector(0.f, 0.f, 100.f));

	// 아레나에서 보스가 길찾기를 하므로 큰 트리거 볼륨이 내비메시 생성에 관여하지 않게 한다
	TriggerBox->SetCanEverAffectNavigation(false);

	TriggerBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnTriggerBeginOverlap);
}

void AACEncounterTrigger::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 스케일이 아니라 Box Extent를 직접 수정한 경우에도 아랫면이 원점에 남도록 오프셋을 다시 계산한다
	if (TriggerBox)
	{
		TriggerBox->SetRelativeLocation(FVector(0.f, 0.f, TriggerBox->GetUnscaledBoxExtent().Z));
	}
}

void AACEncounterTrigger::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
	)
{
	if (bTriggered || !Cast<AACPlayerCharacter>(OtherActor))
	{
		return;
	}

	bTriggered = true;
	SetActorEnableCollision(false);

	ULevelSequencePlayer* SequencePlayer = EncounterSequence ? EncounterSequence->GetSequencePlayer() : nullptr;
	if (!SequencePlayer)
	{
		// 컷신이 배선되지 않았어도 전투는 열려야 한다 — 보스가 영영 대기하는 것이 최악이다
		UE_LOG(LogTemp, Warning, TEXT("[AACEncounterTrigger] EncounterSequence가 지정되지 않아 컷신 없이 전투를 시작합니다."));
		StartBossEncounter();
		return;
	}

	SequencePlayer->OnFinished.AddUniqueDynamic(this, &ThisClass::HandleSequenceFinished);
	SequencePlayer->Play();
}

void AACEncounterTrigger::HandleSequenceFinished()
{
	if (ULevelSequencePlayer* SequencePlayer = EncounterSequence ? EncounterSequence->GetSequencePlayer() : nullptr)
	{
		SequencePlayer->OnFinished.RemoveDynamic(this, &ThisClass::HandleSequenceFinished);
	}

	StartBossEncounter();
}

void AACEncounterTrigger::StartBossEncounter()
{
	const AACGameState* ACGameState = GetWorld() ? GetWorld()->GetGameState<AACGameState>() : nullptr;
	const AACCharacterBase* BossCharacter = ACGameState ? ACGameState->GetBossCharacter() : nullptr;

	AACStateTreeController* BossController = BossCharacter ? Cast<AACStateTreeController>(BossCharacter->GetController()) : nullptr;
	if (!BossController)
	{
		// 보스가 등록되지 않았거나(배치 누락·RegisterBossCharacter 거부) StateTree 컨트롤러가 아니다
		UE_LOG(LogTemp, Warning, TEXT("[AACEncounterTrigger] 전투를 시작할 보스 컨트롤러를 찾지 못했습니다. 보스 배치와 등록 로그를 확인하세요."));
		return;
	}

	BossController->StartEncounter();
}
