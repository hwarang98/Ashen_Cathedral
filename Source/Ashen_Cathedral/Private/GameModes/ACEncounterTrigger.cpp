// 아레나 입구에 배치해 플레이어가 밟으면 조우 컷신을 재생하고, 끝나면 보스 AI를 시작시키는 1회용 트리거


#include "GameModes/ACEncounterTrigger.h"
#include "ACGameplayTags.h"
#include "Character/ACCharacterBase.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/Combat/ACBossPhaseComponent.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Components/UI/EnemyUIComponent.h"
#include "Components/UI/PlayerUIComponent.h"
#include "Components/WidgetComponent.h"
#include "Controllers/ACStateTreeController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/ACGameState.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

AACEncounterTrigger::AACEncounterTrigger()
{
	// 컷신 자동 이동 중에만 틱이 필요하다. 평소에 도는 트리거가 레벨마다 늘어나면 그냥 낭비다
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

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

	// 시퀀서 Audio Track에 BGM을 얹으면 음악 수명이 시퀀스에 묶여서 컷신이 끝나는 순간 함께 잘리고,
	// 재생을 다시 걸면 처음부터 들린다. 페이즈 단위로 이어지려면 트리거가 오디오 컴포넌트를 직접 소유해야 한다
	Phase1MusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Phase1 Music"));
	Phase1MusicComponent->SetupAttachment(SceneRoot);

	// 트리거를 밟는 시점에 코드가 FadeIn으로 시작한다 — 레벨 로드와 함께 울리면 안 된다
	Phase1MusicComponent->bAutoActivate = false;

	// BGM은 아레나 어디에 서 있어도 같은 크기로 들려야 하므로 트리거 위치에 따른 감쇠를 끈다
	Phase1MusicComponent->bAllowSpatialization = false;

	Phase2MusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Phase2 Music"));
	Phase2MusicComponent->SetupAttachment(SceneRoot);
	Phase2MusicComponent->bAutoActivate = false;
	Phase2MusicComponent->bAllowSpatialization = false;
}

void AACEncounterTrigger::BeginPlay()
{
	Super::BeginPlay();

	// 보스를 조우하기 전에는 체력바를 노출하지 않는다
	SetBossUIVisible(false);

	// 시체는 처형이 끝나는 프레임까지 감춰 둔다. 레벨에서 collision을 꺼 둔 배치라면 표시 후에도 꺼진 채로 남도록
	// 원래 설정을 먼저 저장한다. CinematicVictim은 에디터에 잡아 둔 표시 상태를 그대로 존중한다
	if (IsValid(PersistentCorpse))
	{
		bPersistentCorpseCollisionWasEnabled = PersistentCorpse->GetActorEnableCollision();
		PersistentCorpse->SetActorHiddenInGame(true);
		PersistentCorpse->SetActorEnableCollision(false);
	}
	else
	{
		// 시체 배선이 빠져도 조우 자체는 진행되어야 한다 — 보스가 영영 대기하는 것이 최악이다
		UE_LOG(LogTemp, Warning, TEXT("[AACEncounterTrigger] PersistentCorpse가 지정되지 않아 처형 후 시체를 표시할 수 없습니다."));
	}

	// 보스의 RegisterBossCharacter가 이 액터의 BeginPlay보다 늦게 도는 경우를 대비해 한 번 더 시도한다
	GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::HideBossUIOnStart);
}

void AACEncounterTrigger::HideBossUIOnStart()
{
	// 첫 틱 전에 이미 트리거를 밟았다면 그쪽 흐름을 존중한다
	if (!bTriggered)
	{
		SetBossUIVisible(false);
	}
}

void AACEncounterTrigger::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 컷신 중 레벨이 바뀌거나 트리거가 파괴되면 자동 이동이 그대로 돌고 Infinite GE도 플레이어에게 남는다.
	// EndCinematicMovementState가 StopCinematicAutoWalk까지 처리한다
	EndCinematicMovementState();

	// 보스는 이 트리거보다 오래 살 수 있어, 구독을 남겨 두면 파괴된 액터로 콜백이 들어간다
	UnbindBossMusicEvents();

	// 레벨을 벗어나거나 트리거가 파괴되면 BGM도 함께 끝나야 한다
	if (Phase1MusicComponent)
	{
		Phase1MusicComponent->Stop();
	}

	if (Phase2MusicComponent)
	{
		Phase2MusicComponent->Stop();
	}

	Super::EndPlay(EndPlayReason);
}

void AACEncounterTrigger::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bCinematicAutoWalking)
	{
		return;
	}

	AACPlayerCharacter* PlayerCharacter = CinematicPlayerCharacter.Get();
	const AActor* TargetActor = CinematicAutoWalkTarget.Get();

	// 컷신 중 플레이어가 사라지거나 시퀀서가 지정한 목표가 파괴돼도 틱이 계속 돌지 않게 한다
	if (!PlayerCharacter || !TargetActor)
	{
		StopCinematicAutoWalk();
		return;
	}

	// 목표가 플레이어보다 높거나 낮게 놓여 있어도 수평으로만 걷게 한다 — Z를 남기면 벽을 향해 밀린다
	FVector ToTarget = TargetActor->GetActorLocation() - PlayerCharacter->GetActorLocation();
	ToTarget.Z = 0.f;

	if (ToTarget.SizeSquared() <= FMath::Square(CinematicWalkAcceptanceRadius))
	{
		StopCinematicAutoWalk();
		return;
	}

	// 컷신 동안 SetIgnoreMoveInput(true)가 걸려 있어, bForce=false면 이 입력까지 같이 버려진다.
	// 플레이어 키 입력은 그대로 막힌 채 시스템이 넣는 이동만 통과시키려면 세 번째 인자가 반드시 true여야 한다
	PlayerCharacter->AddMovementInput(ToTarget.GetSafeNormal(), CinematicWalkInputScale, true);
}

void AACEncounterTrigger::StartCinematicAutoWalk(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACEncounterTrigger] StartCinematicAutoWalk: TargetActor가 유효하지 않아 자동 이동을 시작하지 못했습니다."));
		return;
	}

	// 트리거를 밟아야 CinematicPlayerCharacter가 채워진다. 컷신 밖에서 부르면 움직일 대상을 알 수 없다
	if (!CinematicPlayerCharacter.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACEncounterTrigger] StartCinematicAutoWalk: 컷신을 시작한 플레이어가 없습니다. 트리거 진입 이후에 호출되는지 확인하세요."));
		return;
	}

	CinematicAutoWalkTarget = TargetActor;
	bCinematicAutoWalking = true;
	SetActorTickEnabled(true);
}

void AACEncounterTrigger::StopCinematicAutoWalk()
{
	bCinematicAutoWalking = false;
	CinematicAutoWalkTarget.Reset();
	SetActorTickEnabled(false);

	// 마지막 프레임에 넣은 이동 입력의 관성이 남으면 도착 지점을 지나쳐 미끄러진다
	if (const AACPlayerCharacter* PlayerCharacter = CinematicPlayerCharacter.Get())
	{
		if (UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}
	}

	if (APlayerController* PlayerController = CinematicPlayerController.Get())
	{
		PlayerController->StopMovement();
	}
}

void AACEncounterTrigger::FinalizeExecutedVictim()
{
	// 시퀀서 Event Track과 컷신 종료 양쪽에서 불려도 상태는 한 번만 바꾼다
	if (bExecutionVictimFinalized)
	{
		return;
	}

	// 아직 성공하지 않았으므로 플래그를 세우지 않는다 — 컷신이 정상 종료되면 한 번 더 시도한다
	if (!IsValid(PersistentCorpse))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACEncounterTrigger] FinalizeExecutedVictim: PersistentCorpse가 유효하지 않아 시체를 표시하지 못했습니다."));
		return;
	}

	bExecutionVictimFinalized = true;

	// 처형 애니메이션을 재생하던 기사는 같은 프레임에 사라져야 시체와 겹쳐 보이지 않는다
	if (IsValid(CinematicVictim))
	{
		CinematicVictim->SetActorHiddenInGame(true);
		CinematicVictim->SetActorEnableCollision(false);
	}

	PersistentCorpse->SetActorHiddenInGame(false);

	// 레벨에서 collision을 꺼 둔 배치라면 표시 후에도 꺼진 상태를 유지한다
	PersistentCorpse->SetActorEnableCollision(bPersistentCorpseCollisionWasEnabled);
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
	AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(OtherActor);
	if (bTriggered || !PlayerCharacter)
	{
		return;
	}

	bTriggered = true;
	SetActorEnableCollision(false);

	// 페이즈 전환과 사망은 컷신이 끝난 뒤에 일어나므로, 트리거가 발동한 이 시점에 미리 구독해 둔다
	BindBossMusicEvents();

	ULevelSequencePlayer* SequencePlayer = EncounterSequence ? EncounterSequence->GetSequencePlayer() : nullptr;
	if (!SequencePlayer)
	{
		// 컷신이 배선되지 않았어도 전투는 열려야 한다 — 보스가 영영 대기하는 것이 최악이다
		UE_LOG(LogTemp, Warning, TEXT("[AACEncounterTrigger] EncounterSequence가 지정되지 않아 컷신 없이 전투를 시작합니다."));

		// 컷신이 없으면 걷기 속도를 걸 이유도 없다. 어떤 경로로든 이미 걸려 있었다면 여기서 걷어낸다
		EndCinematicMovementState();

		// 컷신을 건너뛰는 경로에서도 전투 BGM은 전투가 열리기 전에 시작되어야 한다
		StartPhase1BossMusic();

		StartBossEncounter();
		return;
	}

	// 레벨 시퀀스의 Hide HUD 옵션은 레거시 AHUD만 끄므로 UMG 위젯은 직접 걷어내야 한다
	SetPlayerHUDVisible(false);

	// 시퀀서 Event Track이 StartCinematicAutoWalk를 부르기 전에 속도를 걷기로 고정해 둔다
	BeginCinematicMovementState(PlayerCharacter);

	SequencePlayer->OnFinished.AddUniqueDynamic(this, &ThisClass::HandleSequenceFinished);

	// Stop()으로 끝나는 경로는 OnFinished를 브로드캐스트하지 않는다(MovieSceneSequencePlayer::Stop).
	// 그 경우에도 입력이 잠긴 채 남지 않도록 복구만 따로 받아 둔다
	SequencePlayer->OnStop.AddUniqueDynamic(this, &ThisClass::HandleSequenceStopped);

	// 조우 컷신과 함께 1페이즈 BGM을 시작한다. 시퀀스가 아니라 이 액터가 소유하므로 컷신이 끝나도 이어진다
	StartPhase1BossMusic();

	SequencePlayer->Play();
}

void AACEncounterTrigger::BeginCinematicMovementState(AACPlayerCharacter* PlayerCharacter)
{
	UACAbilitySystemComponent* PlayerASC = PlayerCharacter ? PlayerCharacter->GetACAbilitySystemComponent() : nullptr;
	if (!PlayerASC)
	{
		return;
	}

	CinematicPlayerCharacter = PlayerCharacter;
	CinematicPlayerASC = PlayerASC;
	CinematicPlayerController = Cast<APlayerController>(PlayerCharacter->GetController());

	// 레벨 시퀀스의 Disable Movement Input은 배치 인스턴스마다 켜 줘야 하는 체크박스라 누락되기 쉽다.
	// 컷신 중에는 이동뿐 아니라 회피·공격·점프·상호작용까지 전부 잠근다
	if (APlayerController* PlayerController = CinematicPlayerController.Get())
	{
		if (!bCinematicInputLocked)
		{
			// 이 프로젝트의 입력 바인딩(이동·시점·어빌리티·상호작용)은 SetupPlayerInputComponent가 만든
			// '폰의' InputComponent에 걸려 있다. PlayerController::DisableInput은 컨트롤러 자신의 것만 막으므로
			// 폰 쪽을 꺼야 BuildInputStack이 이 컴포넌트를 스택에서 통째로 빼 준다
			PlayerCharacter->DisableInput(PlayerController);
			PlayerController->DisableInput(PlayerController);

			// 다른 액터가 입력 컴포넌트를 직접 스택에 밀어 넣는 경우까지 대비한 이중 안전장치
			PlayerController->SetIgnoreMoveInput(true);
			PlayerController->SetIgnoreLookInput(true);
			bCinematicInputLocked = true;
		}
	}

	// Shared.Status.Sprinting은 Sprint 어빌리티의 ActivationOwnedTags라 태그만 지우면 어빌리티가 살아남아
	// Sprint 속도 GE가 그대로 걷기 속도를 덮어쓴다. 어빌리티를 취소해야 태그와 GE가 함께 정리된다
	FGameplayTagContainer SprintTag;
	SprintTag.AddTag(ACGameplayTags::Player_Ability_Sprint);
	PlayerASC->CancelAbilities(&SprintTag);

	if (!CinematicWalkSpeedEffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACEncounterTrigger] CinematicWalkSpeedEffectClass가 비어 있어 컷신 이동 속도를 고정하지 못했습니다."));
		return;
	}

	// 이미 걸려 있으면 중복 적용하지 않는다 — 두 번째 핸들만 남아 첫 GE가 영구히 떠 있게 된다
	if (CinematicWalkSpeedEffectHandle.IsValid())
	{
		return;
	}

	// MoveSpeed가 속도의 source of truth이고 PostAttributeChange가 MaxWalkSpeed를 따라오게 하므로
	// CharacterMovement를 직접 만지지 않고 어트리뷰트만 Override 한다
	const UGameplayEffect* WalkSpeedEffect = CinematicWalkSpeedEffectClass->GetDefaultObject<UGameplayEffect>();
	CinematicWalkSpeedEffectHandle = PlayerASC->ApplyGameplayEffectToSelf(WalkSpeedEffect, 1.f, PlayerASC->MakeEffectContext());
}

void AACEncounterTrigger::EndCinematicMovementState()
{
	// 자동 이동이 남아 있으면 컷신이 끝난 뒤에도 플레이어가 목표를 향해 계속 걸어간다.
	// 참조를 비우기 전에 먼저 끊어야 StopCinematicAutoWalk가 플레이어를 제대로 정지시킬 수 있다
	StopCinematicAutoWalk();

	if (APlayerController* PlayerController = CinematicPlayerController.Get())
	{
		// 잠금 카운터를 정확히 되돌린다 — 시퀀스의 Cinematic Mode가 같이 걸려 있어도 각자 1씩 짝을 맞춘다
		if (bCinematicInputLocked)
		{
			PlayerController->EnableInput(PlayerController);
			PlayerController->SetIgnoreMoveInput(false);
			PlayerController->SetIgnoreLookInput(false);
		}
	}

	// 폰은 컨트롤러와 수명이 달라 따로 되돌린다. 컷신 중 리스폰됐다면 새 폰은 이미 입력이 열려 있어 안전하다
	if (bCinematicInputLocked)
	{
		if (AACPlayerCharacter* PlayerCharacter = CinematicPlayerCharacter.Get())
		{
			PlayerCharacter->EnableInput(PlayerCharacter->GetController<APlayerController>());
		}
	}

	bCinematicInputLocked = false;

	// GE를 걷어내면 MoveSpeed가 원래 값으로 재계산되고 MaxWalkSpeed도 따라 복구된다
	if (UACAbilitySystemComponent* PlayerASC = CinematicPlayerASC.Get())
	{
		if (CinematicWalkSpeedEffectHandle.IsValid())
		{
			PlayerASC->RemoveActiveGameplayEffect(CinematicWalkSpeedEffectHandle);
		}
	}

	CinematicWalkSpeedEffectHandle.Invalidate();
	CinematicPlayerASC.Reset();
	CinematicPlayerController.Reset();
	CinematicPlayerCharacter.Reset();
}

void AACEncounterTrigger::SetPlayerHUDVisible(bool bVisible)
{
	const AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (UPlayerUIComponent* PlayerUIComponent = PlayerCharacter ? Cast<UPlayerUIComponent>(PlayerCharacter->GetPawnUIComponent()) : nullptr)
	{
		PlayerUIComponent->SetHUDVisible(bVisible);
	}

	SetBossUIVisible(bVisible);
}

void AACEncounterTrigger::SetBossUIVisible(bool bVisible)
{
	const AACGameState* ACGameState = GetWorld() ? GetWorld()->GetGameState<AACGameState>() : nullptr;
	AACCharacterBase* BossCharacter = ACGameState ? ACGameState->GetBossCharacter() : nullptr;
	if (!BossCharacter)
	{
		return;
	}

	// 보스 체력바는 보스 캐릭터에 붙은 WidgetComponent가 그리므로 컴포넌트 자체를 토글해야 한다
	TArray<UWidgetComponent*> WidgetComponents;
	BossCharacter->GetComponents<UWidgetComponent>(WidgetComponents);
	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		WidgetComponent->SetVisibility(bVisible);
	}

	// BP가 별도로 등록해 둔 적 UI가 있으면 함께 처리한다
	if (UEnemyUIComponent* EnemyUIComponent = BossCharacter->GetEnemyUIComponent())
	{
		EnemyUIComponent->SetEnemyWidgetsVisible(bVisible);
	}
}

void AACEncounterTrigger::HandleSequenceStopped()
{
	// Stop 경로에서도 입력·속도만 되돌린다. 전투 개시는 OnFinished 쪽에 그대로 맡긴다
	EndCinematicMovementState();
}

void AACEncounterTrigger::HandleSequenceFinished()
{
	if (ULevelSequencePlayer* SequencePlayer = EncounterSequence ? EncounterSequence->GetSequencePlayer() : nullptr)
	{
		SequencePlayer->OnFinished.RemoveDynamic(this, &ThisClass::HandleSequenceFinished);
		SequencePlayer->OnStop.RemoveDynamic(this, &ThisClass::HandleSequenceStopped);
	}

	// 시퀀서 Event Key가 빠져 있어도 게임플레이가 시작되기 전에 시체는 반드시 자리에 있어야 한다.
	// Event Track에서 이미 처리했다면 여기서는 아무 일도 하지 않는다
	FinalizeExecutedVictim();

	// 보스 AI를 깨우기 전에 이동 상태부터 되돌린다 — 전투가 열린 뒤에도 걷기 속도면 회피가 되지 않는다
	EndCinematicMovementState();

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
	}
	else
	{
		BossController->StartEncounter();
	}

	// 보스 AI를 못 깨웠더라도 HUD는 반드시 되돌린다 — 컷신 뒤 HUD 없이 갇히는 것이 더 나쁘다
	SetPlayerHUDVisible(true);

	BP_OnEncounterStarted();
}

void AACEncounterTrigger::BindBossMusicEvents()
{
	// 이미 붙어 있다면 다시 걸지 않는다. AddUniqueDynamic이 중복을 막지만 보스 탐색까지 반복할 이유가 없다
	if (BoundMusicBoss.IsValid())
	{
		return;
	}

	const AACGameState* ACGameState = GetWorld() ? GetWorld()->GetGameState<AACGameState>() : nullptr;
	AACCharacterBase* BossCharacter = ACGameState ? ACGameState->GetBossCharacter() : nullptr;
	if (!BossCharacter)
	{
		// 음악은 연출이므로 보스를 못 찾아도 조우와 전투는 그대로 진행시킨다
		UE_LOG(LogTemp, Warning, TEXT("[AACEncounterTrigger] 보스를 찾지 못해 BGM 이벤트를 연결하지 못했습니다."));
		return;
	}

	BossCharacter->OnDeathDelegate.AddUniqueDynamic(this, &ThisClass::HandleMusicBossDeath);
	BoundMusicBoss = BossCharacter;

	UACBossPhaseComponent* PhaseComponent = UACBossPhaseComponent::FindBossPhaseComponent(BossCharacter);
	if (!PhaseComponent)
	{
		// 단일 페이즈 보스라면 정상이다 — 2페이즈 BGM 전환만 일어나지 않는다
		UE_LOG(LogTemp, Warning, TEXT("[AACEncounterTrigger] BossPhaseComponent가 없어 2페이즈 BGM 전환은 사용하지 않습니다."));
		return;
	}

	PhaseComponent->OnPhaseTransitionStarted.AddUniqueDynamic(this, &ThisClass::HandleMusicPhaseTransitionStarted);
	BoundMusicPhaseComponent = PhaseComponent;
}

void AACEncounterTrigger::UnbindBossMusicEvents()
{
	if (AACCharacterBase* BossCharacter = BoundMusicBoss.Get())
	{
		BossCharacter->OnDeathDelegate.RemoveDynamic(this, &ThisClass::HandleMusicBossDeath);
	}

	if (UACBossPhaseComponent* PhaseComponent = BoundMusicPhaseComponent.Get())
	{
		PhaseComponent->OnPhaseTransitionStarted.RemoveDynamic(this, &ThisClass::HandleMusicPhaseTransitionStarted);
	}

	BoundMusicBoss.Reset();
	BoundMusicPhaseComponent.Reset();
}

void AACEncounterTrigger::StartPhase1BossMusic()
{
	if (bPhase1MusicStarted)
	{
		return;
	}

	if (!Phase1BossMusic)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AACEncounterTrigger] Phase1BossMusic이 지정되지 않아 1페이즈 BGM을 재생하지 못했습니다."));
		return;
	}

	if (!Phase1MusicComponent)
	{
		return;
	}

	// 이미 울리고 있으면 FadeIn을 다시 걸지 않는다 — 재생 위치가 0으로 돌아가 음악이 처음부터 들린다
	if (Phase1MusicComponent->IsPlaying())
	{
		bPhase1MusicStarted = true;
		return;
	}

	Phase1MusicComponent->SetSound(Phase1BossMusic);
	Phase1MusicComponent->FadeIn(Phase1FadeInDuration, 1.f, 0.f);

	bPhase1MusicStarted = true;
}

void AACEncounterTrigger::CrossfadeToPhase2BossMusic()
{
	// 전환 이벤트가 중복으로 들어와도 2페이즈 BGM이 처음부터 다시 시작되지 않게 한다
	if (bPhase2MusicStarted)
	{
		return;
	}

	if (!Phase2BossMusic)
	{
		// 2페이즈 음악이 없다면 1페이즈 음악을 끊지 않는다 — 무음으로 싸우는 것보다 낫다
		UE_LOG(LogTemp, Warning, TEXT("[AACEncounterTrigger] Phase2BossMusic이 지정되지 않아 1페이즈 BGM을 그대로 유지합니다."));
		return;
	}

	if (!Phase2MusicComponent)
	{
		return;
	}

	bPhase2MusicStarted = true;

	if (Phase1MusicComponent && Phase1MusicComponent->IsPlaying())
	{
		Phase1MusicComponent->FadeOut(PhaseCrossfadeDuration, 0.f);
	}

	Phase2MusicComponent->SetSound(Phase2BossMusic);
	Phase2MusicComponent->FadeIn(PhaseCrossfadeDuration, 1.f, 0.f);
}

void AACEncounterTrigger::FadeOutBossMusic()
{
	// FadeOut의 목표 볼륨이 0이면 페이드가 끝나는 시점에 재생도 함께 멈춘다
	if (Phase1MusicComponent && Phase1MusicComponent->IsPlaying())
	{
		Phase1MusicComponent->FadeOut(BossDeathFadeOutDuration, 0.f);
	}

	if (Phase2MusicComponent && Phase2MusicComponent->IsPlaying())
	{
		Phase2MusicComponent->FadeOut(BossDeathFadeOutDuration, 0.f);
	}
}

void AACEncounterTrigger::HandleMusicPhaseTransitionStarted(int32 CurrentPhase)
{
	// OnPhaseTransitionStarted는 전환 '전' 페이즈를 넘긴다. 3페이즈 이상 보스의 2→3 전환에는 반응하지 않는다
	if (CurrentPhase != 1)
	{
		return;
	}

	CrossfadeToPhase2BossMusic();
}

void AACEncounterTrigger::HandleMusicBossDeath(AACCharacterBase* DeadCharacter)
{
	// 구독한 보스의 사망에만 반응한다 — 다른 캐릭터의 델리게이트가 섞여 들어와도 BGM을 끊지 않는다
	if (BoundMusicBoss.IsValid() && DeadCharacter != BoundMusicBoss.Get())
	{
		return;
	}

	FadeOutBossMusic();
}
