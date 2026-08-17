// 아레나 입구에 배치해 플레이어가 밟으면 조우 컷신을 재생하고, 끝나면 보스 AI를 시작시키는 1회용 트리거

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "ACEncounterTrigger.generated.h"

class UBoxComponent;
class ALevelSequenceActor;
class UGameplayEffect;
class UACAbilitySystemComponent;
class AACPlayerCharacter;
class APlayerController;
class UAudioComponent;
class USoundBase;
class UACBossPhaseComponent;
class AACCharacterBase;

/**
 * @brief 보스 조우 연출의 시작점.
 *
 * 보스의 AIController(StateTreeAIComponent)에서 Start Logic Automatically를 꺼 두면 보스는
 * 이 트리거가 발동하기 전까지 완전히 대기한다. 플레이어가 트리거를 밟으면 지정된 컷신을 재생하고,
 * 컷신이 끝나는 시점에 GameState에 등록된 보스의 컨트롤러에 StartEncounter()를 호출해 전투를 연다.
 * 컷신을 지정하지 않으면 연출 없이 즉시 전투가 시작된다(안전 폴백).
 *
 * @note 컷신 중 플레이어 입력 잠금은 이 액터가 직접 건다. 레벨 시퀀스의 Playback 설정
 *       (Disable Movement Input / Disable Look Input)에만 맡기면 배치 인스턴스마다 체크해야 해 누락되기 쉽고,
 *       그 설정은 이동·시점만 막아 회피나 공격으로 연출에서 빠져나갈 수 있다.
 *       두 곳이 겹쳐 걸려도 각자 잠금/해제 짝이 맞아 안전하다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API AACEncounterTrigger : public AActor
{
	GENERATED_BODY()

public:
	AACEncounterTrigger();

	/**
	 * @brief 조우 전까지 보스 UI를 감춘다.
	 *
	 * @note 이 트리거가 레벨에 있다는 것은 곧 "보스 등장 연출이 있다"는 뜻이므로, 별도 플래그 없이
	 *       여기서 보스 체력바를 내려 둔다. 트리거가 없는 레벨의 보스는 평소대로 UI가 보인다.
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief 박스 크기가 바뀌어도 아랫면이 액터 원점에 붙어 있도록 위치를 다시 맞춘다.
	 *
	 * @param Transform 액터의 새 트랜스폼
	 * @note 바닥에 놓고 위로만 키우는 배치 방식을 위한 것이다. 박스는 중심 기준이라 그냥 두면
	 *       크기를 키울 때 바닥 아래로도 절반이 파고든다.
	 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/**
	 * @brief 컷신 도중 액터가 파괴되거나 레벨이 바뀌어도 플레이어에게 Infinite GE가 남지 않도록 정리한다.
	 *
	 * @param EndPlayReason 액터가 종료되는 이유
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * @brief 자동 이동 중일 때만 돌며, 플레이어를 목표 지점 방향으로 강제 이동 입력으로 민다.
	 *
	 * @param DeltaSeconds 프레임 델타
	 * @note 평소에는 꺼져 있고 StartCinematicAutoWalk가 켠다.
	 */
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * @brief 컷신 자동 이동을 시작한다 — 플레이어를 TargetActor 쪽으로 걸어가게 한다.
	 *
	 * @param TargetActor 걸어갈 목표(Target Point 등)
	 * @note 시퀀서 Director Blueprint에서 호출한다. 트리거를 밟아 컷신 상태에 들어간 뒤에만 동작하며,
	 *       그 전에 호출하면 대상 플레이어를 몰라 경고만 남기고 아무것도 하지 않는다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cinematic|Movement")
	void StartCinematicAutoWalk(AActor* TargetActor);

	/**
	 * @brief 컷신 자동 이동을 멈추고 남은 관성까지 끊는다.
	 *
	 * @note 목표 도달·컷신 종료·중단·액터 파괴 어느 경로로 들어와도 안전하도록 여러 번 호출해도 무해하다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cinematic|Movement")
	void StopCinematicAutoWalk();

	/**
	 * @brief 처형당한 시네마틱용 기사를 감추고, 같은 자리에 미리 배치해 둔 게임플레이용 시체를 드러낸다.
	 *
	 * @note 시퀀서 Event Track이 처형 애니메이션이 끝나는 정확한 프레임에서 호출한다. C++ 타이머나
	 *       시퀀스 길이로 시점을 계산하지 않는 이유는 애니메이션이 수정될 때마다 어긋나기 때문이다.
	 * @note Event Key가 누락돼도 컷신이 정상 종료되면 HandleSequenceFinished가 한 번 더 부른다.
	 *       여러 번 호출해도 상태는 한 번만 바뀐다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Encounter|Execution")
	void FinalizeExecutedVictim();

protected:
	// 액터 원점 = 박스의 아랫면. 바닥에 배치한 뒤 스케일을 올리면 위쪽으로만 자란다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter")
	TObjectPtr<UBoxComponent> TriggerBox;

	// 재생할 조우 컷신 — 같은 레벨에 배치된 LevelSequenceActor를 지정한다. 비워 두면 컷신 없이 즉시 전투가 시작된다
	UPROPERTY(EditInstanceOnly, Category = "Encounter")
	TObjectPtr<ALevelSequenceActor> EncounterSequence;

	// 1페이즈 BGM을 재생하는 컴포넌트. 시퀀스 수명과 분리하기 위해 이 액터가 직접 소유한다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Music")
	TObjectPtr<UAudioComponent> Phase1MusicComponent;

	// 2페이즈 BGM을 재생하는 컴포넌트. 1페이즈와 겹쳐 Crossfade 하기 위해 별도로 둔다
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Encounter|Music")
	TObjectPtr<UAudioComponent> Phase2MusicComponent;

	// 트리거를 밟는 순간 시작할 1페이즈 BGM. 루프 설정은 에셋(Sound Wave/Cue) 쪽에서 켜 둔다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Music")
	TObjectPtr<USoundBase> Phase1BossMusic;

	// 1→2페이즈 전환이 시작될 때 넘겨받을 2페이즈 BGM. 비워 두면 1페이즈 BGM이 그대로 유지된다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Music")
	TObjectPtr<USoundBase> Phase2BossMusic;

	// 1페이즈 BGM이 무음에서 올라오는 시간(초). 0이면 즉시 최대 볼륨으로 시작한다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Music", meta = (ClampMin = "0.0"))
	float Phase1FadeInDuration = 0.5f;

	// 1페이즈 BGM이 빠지고 2페이즈 BGM이 올라오는 데 걸리는 시간(초). 두 페이드가 같은 길이로 겹친다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Music", meta = (ClampMin = "0.0"))
	float PhaseCrossfadeDuration = 1.5f;

	// 보스가 죽은 뒤 현재 BGM이 무음까지 내려가는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Music", meta = (ClampMin = "0.0"))
	float BossDeathFadeOutDuration = 2.f;

	// 처형 애니메이션을 재생하는 시네마틱용 기사 Actor — 처형이 끝나는 프레임에 감춘다
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter|Execution")
	TObjectPtr<AActor> CinematicVictim;

	// 마지막 누운 자세 그대로 레벨에 미리 배치해 둔 게임플레이용 시체 Actor — 처형이 끝나는 프레임에 드러낸다
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Encounter|Execution")
	TObjectPtr<AActor> PersistentCorpse;

	// 컷신 동안 플레이어에게 걸어 둘 이동 속도 GE (GE_CinematicWalkSpeed). MoveSpeed를 Override 하므로 MaxWalkSpeed는 건드리지 않는다
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter|Cinematic")
	TSubclassOf<UGameplayEffect> CinematicWalkSpeedEffectClass;

	// 목표까지 이 거리 안으로 들어오면 도착으로 보고 자동 이동을 끝낸다. 너무 작으면 목표 위에서 좌우로 떤다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cinematic|Movement")
	float CinematicWalkAcceptanceRadius = 100.f;

	// AddMovementInput에 넘길 스케일. 실제 속도는 MoveSpeed 어트리뷰트가 정하므로 보통 1.0을 그대로 둔다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cinematic|Movement")
	float CinematicWalkInputScale = 1.f;

	/**
	 * 조우 연출이 끝나 전투가 시작되는 순간 호출된다. 보스 체력바처럼 전투 개시와 함께 나타나야 하는
	 * UI를 여기서 띄운다 — 컷신 이전에 미리 떠 있으면 연출을 가린다.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Encounter", meta = (DisplayName = "OnEncounterStarted"))
	void BP_OnEncounterStarted();

private:
	// 컷신 동안 게임플레이 HUD를 숨기거나 되돌린다 (보스 UI도 함께 처리)
	void SetPlayerHUDVisible(bool bVisible);

	// 보스 체력바 등 보스가 소유한 UI를 숨기거나 되돌린다
	void SetBossUIVisible(bool bVisible);

	// 보스 등록이 이 액터의 BeginPlay보다 늦을 수 있어 다음 틱에 한 번 더 숨김을 시도한다
	void HideBossUIOnStart();

	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);

	// 컷신 재생이 끝나면 호출되어 전투를 연다
	UFUNCTION()
	void HandleSequenceFinished();

	// Stop()으로 컷신이 중단된 경우의 안전장치 — OnFinished가 오지 않으므로 이동 상태만 되돌린다
	UFUNCTION()
	void HandleSequenceStopped();

	// GameState에 등록된 보스의 컨트롤러를 찾아 StartEncounter를 호출한다
	void StartBossEncounter();

	/**
	 * @brief 컷신용 이동 상태로 전환한다 — 플레이어 입력을 잠그고, Sprint를 취소한 뒤 걷기 속도 GE를 적용한다.
	 *
	 * @param PlayerCharacter 트리거를 밟은 플레이어
	 * @note 입력 차단은 컨트롤러가 아니라 폰에 걸어야 한다. 이 프로젝트의 바인딩은 폰의 InputComponent에
	 *       있고, BuildInputStack이 APawn::bInputEnabled로 그 컴포넌트를 통째로 걸러 내기 때문이다.
	 * @note 태그를 직접 제거하지 않고 CancelAbilities를 쓰는 이유는, Sprint 어빌리티가 살아 있으면
	 *       Shared.Status.Sprinting과 Sprint 속도 GE가 그대로 남아 컷신 속도를 덮어쓰기 때문이다.
	 */
	void BeginCinematicMovementState(AACPlayerCharacter* PlayerCharacter);

	/**
	 * @brief 컷신용 이동 상태를 되돌린다 — 자동 이동을 끊고, 입력 잠금을 풀고, 걷기 속도 GE를 제거한다.
	 *
	 * @note MoveSpeed는 GE 제거만으로 재계산되므로 MaxWalkSpeed를 따로 저장·복구하지 않는다.
	 *       여러 번 호출해도 안전하다(핸들이 무효면 아무것도 하지 않는다).
	 */
	void EndCinematicMovementState();

	/**
	 * @brief GameState에 등록된 보스를 찾아 페이즈 전환·사망 델리게이트를 구독한다.
	 *
	 * @note 여러 번 호출해도 중복으로 걸리지 않는다. 보스나 BossPhaseComponent가 없어도 경고만 남기고
	 *       조우 진행을 막지 않는다 — 음악은 연출이고 전투가 열리는 것이 우선이다.
	 */
	void BindBossMusicEvents();

	// BindBossMusicEvents로 걸어 둔 구독을 모두 해제한다. 여러 번 호출해도 안전하다
	void UnbindBossMusicEvents();

	/**
	 * @brief 1페이즈 BGM을 FadeIn으로 시작한다.
	 *
	 * @note 이미 재생 중이면 아무것도 하지 않는다 — 다시 FadeIn을 걸면 재생 위치가 0으로 되돌아가
	 *       조우 컷신 종료나 트리거 재진입 때 음악이 처음부터 다시 들린다.
	 */
	void StartPhase1BossMusic();

	/**
	 * @brief 1페이즈 BGM을 빼면서 2페이즈 BGM을 올린다. 정확히 한 번만 실행된다.
	 *
	 * @note Phase2BossMusic이 비어 있으면 1페이즈 BGM을 끊지 않고 그대로 둔다 — 무음보다 낫다.
	 */
	void CrossfadeToPhase2BossMusic();

	// 재생 중인 BGM을 BossDeathFadeOutDuration 동안 무음까지 내리고 정지시킨다
	void FadeOutBossMusic();

	// 보스의 페이즈 전환이 시작된 시점에 호출된다. 인자는 전환 전 페이즈 번호
	UFUNCTION()
	void HandleMusicPhaseTransitionStarted(int32 CurrentPhase);

	// 보스 HP가 0이 된 시점에 호출된다
	UFUNCTION()
	void HandleMusicBossDeath(AACCharacterBase* DeadCharacter);

	// 이미 발동했는지 여부 (1회용)
	bool bTriggered = false;

	// 컷신을 시작한 플레이어 — 컷신 도중 사망·리스폰으로 사라질 수 있어 약참조로 들고 있는다
	TWeakObjectPtr<AACPlayerCharacter> CinematicPlayerCharacter;

	// 입력 잠금·이동 정지 대상. 폰이 교체되어도 컨트롤러는 남으므로 캐릭터와 따로 보관한다
	TWeakObjectPtr<APlayerController> CinematicPlayerController;

	// 위 플레이어의 ASC. 복구 시점에 캐릭터가 유효하지 않아도 GE는 걷어낼 수 있도록 따로 보관한다
	TWeakObjectPtr<UACAbilitySystemComponent> CinematicPlayerASC;

	// 적용 중인 걷기 속도 GE의 핸들. 유효하면 이미 적용된 상태라는 뜻이라 중복 적용 방지에도 쓰인다
	FActiveGameplayEffectHandle CinematicWalkSpeedEffectHandle;

	// 입력을 잠근 상태인지 여부. SetIgnoreMoveInput이 누적 카운터라 잠금과 해제가 정확히 1:1이어야 한다
	bool bCinematicInputLocked = false;

	// 자동 이동 목표. 시퀀서가 지정한 Target Point가 도중에 사라져도 안전하도록 약참조로 들고 있는다
	TWeakObjectPtr<AActor> CinematicAutoWalkTarget;

	// 자동 이동 중인지 여부. Tick 활성화 상태와 짝을 이룬다
	bool bCinematicAutoWalking = false;

	// 시체 전환을 이미 처리했는지 여부. Event Track과 HandleSequenceFinished 양쪽에서 불려도 한 번만 적용된다
	bool bExecutionVictimFinalized = false;

	// 레벨에 배치된 시체의 원래 collision 설정. BeginPlay에서 끄기 전에 저장해 두고 표시할 때 그대로 되돌린다
	bool bPersistentCorpseCollisionWasEnabled = false;

	// BGM 이벤트를 구독한 보스. 사망으로 파괴될 수 있어 약참조로 들고 있으며, 중복 바인딩 판정에도 쓰인다
	TWeakObjectPtr<AACCharacterBase> BoundMusicBoss;

	// 위 보스의 BossPhaseComponent. 단일 페이즈 보스면 비어 있다
	TWeakObjectPtr<UACBossPhaseComponent> BoundMusicPhaseComponent;

	// 1페이즈 BGM을 이미 시작했는지 여부. 트리거 재진입이나 폴백 경로 중복 호출로 다시 재생되지 않게 한다
	bool bPhase1MusicStarted = false;

	// 2페이즈 BGM으로 이미 넘어갔는지 여부. Crossfade가 정확히 한 번만 일어나게 한다
	bool bPhase2MusicStarted = false;
};
