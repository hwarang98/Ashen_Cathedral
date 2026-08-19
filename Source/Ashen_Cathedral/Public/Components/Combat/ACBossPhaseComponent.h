// 보스의 페이즈 전환을 관리하는 컴포넌트 — 체력 0을 사망 대신 다음 페이즈 진입으로 바꾼다

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Components/PawnExtensionComponentBase.h"
#include "Engine/EngineTypes.h"
#include "Interfaces/ACZeroHealthHandlerInterface.h"
#include "ACBossPhaseComponent.generated.h"

class AAIController;
class ALevelSequenceActor;
class UAbilitySystemComponent;
class UBrainComponent;
class UGameplayAbility;
class ULevelSequence;
class ULevelSequencePlayer;
struct FAbilityEndedData;
struct FOnAttributeChangeData;

/** 페이즈 전환을 발동시키는 조건. 새 조건은 여기에 항목을 추가하고 컴포넌트에서 판정을 붙이면 된다 */
UENUM(BlueprintType)
enum class EACPhaseTransitionTrigger : uint8
{
	HealthZero       UMETA(DisplayName = "체력 0 도달"),
	HealthPercentage UMETA(DisplayName = "체력 비율 최초 통과"),
	Manual           UMETA(DisplayName = "외부 수동 요청"),
};

/**
 * 페이즈 전환 한 단계의 설정. 보스 Blueprint의 BossPhaseComponent에서 배열로 편집한다.
 * 배열 인덱스 N은 "페이즈 N+1 → 페이즈 N+2" 전환을 뜻하며, 각 항목은 정확히 한 번만 실행된다.
 */
USTRUCT(BlueprintType)
struct FACBossPhaseTransition
{
	GENERATED_BODY()

	/** 이 전환을 발동시킬 조건 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossPhase")
	EACPhaseTransitionTrigger TriggerType = EACPhaseTransitionTrigger::HealthZero;

	/** TriggerType이 HealthPercentage일 때만 사용. 최대 체력 대비 이 비율 이하로 처음 떨어지는 순간 전환한다 (0.5 = 50%) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossPhase", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "TriggerType == EACPhaseTransitionTrigger::HealthPercentage", EditConditionHides))
	float HealthPercentage = 0.5f;

	/**
	 * 전환 연출을 실행할 어빌리티(예: GA_Phase2_Ordan). 시퀀스 재생이 끝난 뒤 활성화된다.
	 * 이미 ASC에 부여된 어빌리티면 그 Spec을 재사용하고, 없으면 이 전환 시점에 부여한다.
	 * None이면 어빌리티 단계를 건너뛰고 곧바로 완료 처리로 넘어간다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossPhase")
	TSubclassOf<UGameplayAbility> TransitionAbility;

	/** 전환 시작 시 재생할 컷신. None이거나 재생에 실패하면 곧바로 어빌리티 단계로 넘어간다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossPhase")
	TObjectPtr<ULevelSequence> TransitionSequence;

	/**
	 * 전환 완료 시 채워 넣을 체력 비율(최대 체력 기준). 1.0이면 완전 회복.
	 * 0이면 이 컴포넌트는 체력을 건드리지 않으며, 회복 책임은 TransitionAbility 쪽 GameplayEffect가 갖는다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossPhase", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RestoreHealthPercent = 0.f;

	/**
	 * 전환이 완료되면 부여할 영구 페이즈 상태 태그 (예: 첫 전환은 Enemy.State.Phase.2, 두 번째 전환은 Enemy.State.Phase.3).
	 *
	 * 한 번 부여하면 제거하지 않고 누적한다 — 3페이즈 보스는 Phase.2와 Phase.3을 함께 보유한다.
	 * GameplayTag 매칭은 형제 간 대소 비교가 되지 않으므로(Phase.3은 Phase.2에 매칭되지 않는다),
	 * 누적해야 "2페이즈 이상"을 HasTag(Phase.2) 하나로 물을 수 있다.
	 * 이렇게 태그로 유지하면 StateTree 조건뿐 아니라 GAS의 ActivationBlockedTags·GE Tag Requirement도 같은 값을 그대로 본다.
	 *
	 * None이면 이 컴포넌트는 태그를 부여하지 않으며, 부여 책임은 TransitionAbility가 갖는다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossPhase", meta = (Categories = "Enemy.State.Phase"))
	FGameplayTag PhaseStateTag;
};

/** 페이즈 전환 시작/완료를 외부(UI, 사운드, 카메라 등)에 알리는 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossPhaseSignature, int32, Phase);

/**
 * 보스의 페이즈 전환을 관리하는 컴포넌트.
 *
 * [책임 범위]
 * - 페이즈 판정(체력 0 / 체력 비율 / 수동)과 중복 전환 차단
 * - 전환 중 무적·AI 정지 등 "전환 상태" 소유
 * - 컷신(LevelSequence) 재생과 TransitionAbility 호출 순서 보장
 * - RestoreHealthPercent에 따른 체력 회복
 * - PhaseStateTag(Enemy.State.Phase.N) 누적 부여 — 페이즈 상태의 단일 진실 출처
 *
 * [책임 밖]
 * - 보스별 스탯 강화·외형·이펙트·몽타주는 TransitionAbility(예: GA_Phase2_Ordan)가 갖는다.
 *
 * @note 판정과 상태 변경은 모두 서버 권한에서만 일어나며, CurrentPhase와 전환 진행 여부만 복제된다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ASHEN_CATHEDRAL_API UACBossPhaseComponent : public UPawnExtensionComponentBase, public IACZeroHealthHandlerInterface
{
	GENERATED_BODY()

public:
	UACBossPhaseComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * @brief 체력이 0이 된 순간 UACAttributeSet이 호출한다. 남은 HealthZero 전환이 있으면 사망 대신 전환을 시작한다.
	 *
	 * @param DamageInstigator 마지막 피해를 입힌 액터. 없을 수 있다
	 * @return true면 호출자가 사망 태그·사망 이벤트를 모두 생략한다
	 * @note 이미 전환이 진행 중이면 컷신 도중 들어온 추가 피해이므로 항상 true를 반환해 사망을 막는다.
	 */
	virtual bool TryHandleZeroHealth(AActor* DamageInstigator) override;

	/**
	 * @brief TriggerType이 Manual인 다음 전환을 외부에서 발동시킨다.
	 *
	 * @return 전환을 시작했으면 true. 남은 전환이 없거나 조건이 Manual이 아니거나 이미 전환 중이면 false
	 */
	UFUNCTION(BlueprintCallable, Category = "BossPhase")
	bool RequestPhaseTransition();

	/** 현재 페이즈. 1부터 시작한다 */
	UFUNCTION(BlueprintPure, Category = "BossPhase")
	int32 GetCurrentPhase() const { return CurrentPhase; }

	/** 최대 페이즈. 별도 입력 없이 전환 개수로부터 계산된다 */
	UFUNCTION(BlueprintPure, Category = "BossPhase")
	int32 GetMaxPhase() const { return PhaseTransitions.Num() + 1; }

	/** 지금 페이즈 전환 연출이 진행 중인지 여부 */
	UFUNCTION(BlueprintPure, Category = "BossPhase")
	bool IsPhaseTransitionInProgress() const { return bPhaseTransitionInProgress; }

	/** 남은 전환이 없어 다음 체력 0이 최종 사망이 되는 상태인지 여부 */
	UFUNCTION(BlueprintPure, Category = "BossPhase")
	bool IsFinalPhase() const { return CurrentPhase >= GetMaxPhase(); }

	/** 에디터 검증 도구에서 페이즈 설정을 읽기 위한 읽기 전용 접근자. */
	const TArray<FACBossPhaseTransition>& GetPhaseTransitions() const { return PhaseTransitions; }

	/** 에디터 검증 도구에서 안전 제한 시간을 표시하기 위한 읽기 전용 접근자. */
	float GetMaxTransitionDuration() const { return MaxTransitionDuration; }

#if !UE_BUILD_SHIPPING
	/** 개발 도구 전용: TriggerType과 관계없이 다음 페이즈 전환을 시작한다. */
	bool DebugForceNextPhase();
#endif

	/**
	 * @brief 진행 중인 전환이 이 컴포넌트에서 체력을 회복시킬 예정인지 여부.
	 * TransitionAbility가 자신의 회복 GameplayEffect를 건너뛸지 판단하는 데 사용한다.
	 */
	UFUNCTION(BlueprintPure, Category = "BossPhase")
	bool WillRestoreHealthOnPendingTransition() const;

	/**
	 * @brief 진행 중인 전환이 이 컴포넌트에서 페이즈 상태 태그를 부여할 예정인지 여부.
	 * TransitionAbility가 자신의 페이즈 태그 부여를 건너뛸지 판단하는 데 사용한다.
	 */
	UFUNCTION(BlueprintPure, Category = "BossPhase")
	bool WillGrantPhaseStateTagOnPendingTransition() const;

	/** 액터에 붙은 BossPhaseComponent를 찾는다. 일반 적처럼 없는 경우 nullptr */
	static UACBossPhaseComponent* FindBossPhaseComponent(const AActor* InActor);

	/** 페이즈 전환 연출이 시작된 시점에 발송된다. 인자는 전환 전 페이즈 번호 */
	UPROPERTY(BlueprintAssignable, Category = "BossPhase")
	FOnBossPhaseSignature OnPhaseTransitionStarted;

	/** 페이즈 전환이 완료되어 전투가 재개된 시점에 발송된다. 인자는 새 페이즈 번호 */
	UPROPERTY(BlueprintAssignable, Category = "BossPhase")
	FOnBossPhaseSignature OnPhaseTransitionCompleted;

protected:
	/**
	 * 페이즈 전환 목록. 항목 수 + 1이 최대 페이즈가 된다.
	 * 배열이 비어 있으면 이 보스는 단일 페이즈이며 체력 0에서 기존 사망 처리를 그대로 탄다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossPhase")
	TArray<FACBossPhaseTransition> PhaseTransitions;

	/**
	 * 전환이 이 시간(초) 안에 끝나지 않으면 강제로 완료 처리한다.
	 * 시퀀스나 어빌리티가 콜백을 주지 않고 멈춰버려도 보스가 무적인 채 굳지 않게 하는 안전망이다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossPhase", meta = (ClampMin = "1.0"))
	float MaxTransitionDuration = 30.f;

	/**
	 * 전환 시작 시 어빌리티를 정리할 때 건드리지 않을 AssetTag 목록.
	 * 비워두면 진행 중인 모든 어빌리티를 취소한다.
	 * 무기 스폰처럼 한 번 켜지면 계속 살아 있어야 하는 패시브 어빌리티가 있다면 그 AssetTag를 여기에 넣는다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BossPhase")
	FGameplayTagContainer AbilityTagsToKeepDuringTransition;

private:
	/** 전환 조건 판정 → 실제 전환 시작. 중복 진입은 호출 측에서 이미 걸러진 상태여야 한다 */
	void BeginPhaseTransition(int32 TransitionIndex);

	/** 컷신을 재생한다. 재생을 시작했으면 true, 시퀀스가 없거나 재생에 실패했으면 false */
	bool PlayTransitionSequence(const FACBossPhaseTransition& Transition);

	/** 컷신 종료(정상 종료·스킵·강제 중단 공통) 시 호출. 어빌리티 단계로 넘어간다 */
	UFUNCTION()
	void OnTransitionSequenceEnded();

	/** TransitionAbility를 찾아 활성화한다. 어빌리티가 없거나 활성화에 실패하면 즉시 완료 처리한다 */
	void StartTransitionAbilityStep();

	/** 전환 어빌리티가 끝난 시점을 감지한다 */
	void OnTransitionAbilityEnded(const FAbilityEndedData& EndedData);

	/**
	 * @brief 완료 처리를 다음 틱으로 미뤄 예약한다.
	 *
	 * @note 어빌리티의 활성화·종료 콜스택 안에서 완료 처리를 하면 CleanupTransitionAbility의 ClearAbility가
	 *       아직 실행 중인 어빌리티의 Spec을 그 자리에서 걷어내게 되어, 이후 어빌리티 활성화가 조용히 실패한다.
	 *       전환 중이 아니면 아무것도 예약하지 않는다.
	 */
	void RequestCompletePhaseTransition();

	/** 전환 완료 처리. 중복 호출되어도 실제 처리는 정확히 한 번만 일어난다 */
	void CompletePhaseTransition();

	/** MaxTransitionDuration을 넘겼을 때 강제로 전환을 끝낸다 */
	void OnTransitionFailsafeElapsed();

	/** HealthPercentage 조건을 판정하기 위해 체력 변화를 구독한다 */
	void OnHealthAttributeChanged(const FOnAttributeChangeData& ChangeData);

	/** RestoreHealthPercent에 따라 체력을 채우고 체력 위젯을 갱신한다 */
	void RestoreHealth(float RestorePercent);

	/** 재생 중인 컷신을 정지하고 임시로 생성한 LevelSequenceActor를 정리한다 */
	void CleanupTransitionSequence();

	/**
	 * @brief 전환 컷신 동안 플레이어 HUD와 보스 UI를 숨기거나 되돌린다.
	 *
	 * @param bVisible false면 숨기고, true면 컷신 전 상태로 되돌린다
	 * @note 레벨 시퀀스의 Hide HUD 옵션은 레거시 AHUD만 끄므로 UMG 위젯은 이렇게 직접 걷어내야 한다.
	 *       실제로 숨긴 경우에만 복원하므로 여러 번 호출해도 안전하다.
	 */
	void SetTransitionUIVisible(bool bVisible);

	/** 전환 어빌리티 종료 구독을 해제하고, 이 전환에서 임시로 부여한 어빌리티면 회수한다 */
	void CleanupTransitionAbility();

	/**
	 * @brief 전환 연출 동안 AI 로직과 이동을 완전히 멈춘다. BT 보스와 StateTree 보스를 모두 지원한다.
	 *
	 * @note PauseLogic이 아니라 StopLogic을 쓴다. PauseLogic은 틱만 끌 뿐 트리를 살려 두기 때문에
	 *       컷신 도중에도 상태가 진입하는 일이 있었고, 재개 시점에는 멈춰 있던 구간의 이벤트가
	 *       처리되지 못한 채 사라져 대기 상태에 눌러앉았다.
	 */
	void StopAILogicForTransition();

	/**
	 * @brief 멈춰 둔 AI 로직을 다시 시작하고 Combat 진입에 필요한 타겟 이벤트를 다시 알린다.
	 *
	 * @note 조우 컷신이 StartEncounter에서 StartLogic + TargetAcquired 재발송으로 전투를 여는 것과 같은 절차다.
	 *       StateTree의 Combat 진입은 이 이벤트가 필수 조건(Enter Event)이라 재시작만으로는 대기 상태에 머무른다.
	 */
	void RestartAILogicAfterTransition();

	/** 소유 Pawn의 AIController. 플레이어이거나 컨트롤러가 없으면 nullptr */
	AAIController* GetOwningAIController() const;

	/**
	 * AIController가 물고 있는 BrainComponent.
	 * StateTree 보스는 UStateTreeComponent가 AAIController::BrainComponent 슬롯에 등록되지 않으므로
	 * 슬롯이 비어 있으면 컨트롤러의 컴포넌트에서 직접 찾는다.
	 */
	UBrainComponent* GetOwningBrainComponent() const;

	/** 소유 액터가 서버 권한을 가지고 있는지 여부 */
	bool HasAuthority() const;

	/** 다음에 실행될 전환의 인덱스. 남은 전환이 없으면 INDEX_NONE */
	int32 GetNextTransitionIndex() const;

	/** 현재 페이즈. 1부터 시작하며 서버에서만 증가한다 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "BossPhase", meta = (AllowPrivateAccess = "true"))
	int32 CurrentPhase = 1;

	/** 전환 연출 진행 여부. 중복 전환과 추가 피해로 인한 사망을 막는 게이트이자 완료 처리의 1회성 보장 플래그 */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "BossPhase", meta = (AllowPrivateAccess = "true"))
	bool bPhaseTransitionInProgress = false;

	/** 최종 사망이 시작되었는지 여부. 한 번 켜지면 이후 체력 0 질의는 항상 기존 사망 로직으로 넘긴다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BossPhase", meta = (AllowPrivateAccess = "true"))
	bool bFinalDeathStarted = false;

	/** 현재 진행 중인 전환의 인덱스. 전환 중이 아니면 INDEX_NONE */
	int32 PendingTransitionIndex = INDEX_NONE;

	/** 컷신 종료 처리가 이미 한 번 일어났는지 여부. 스킵과 정상 종료가 동시에 들어와도 한 번만 진행시킨다 */
	bool bTransitionSequenceEndHandled = false;

	/** 이 전환에서 이동을 껐는지 여부. 실제로 껐을 때만 되돌려 다른 곳이 설정한 이동 모드를 건드리지 않는다 */
	bool bDisabledMovementForTransition = false;

	/** 이동을 끄기 직전의 이동 모드. 전환이 끝나면 이 값으로 되돌린다 */
	TEnumAsByte<EMovementMode> PreTransitionMovementMode = MOVE_Walking;

	/** 이 전환에서 UI를 숨겼는지 여부. 숨긴 경우에만 되돌려 다른 연출이 걸어 둔 숨김 상태를 건드리지 않는다 */
	bool bTransitionUIHidden = false;

	/** 이 전환에서 TransitionAbility를 임시로 부여했는지 여부. true면 전환 종료 시 회수한다 */
	bool bGrantedTransitionAbility = false;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	TWeakObjectPtr<ULevelSequencePlayer> ActiveSequencePlayer;

	TWeakObjectPtr<ALevelSequenceActor> ActiveSequenceActor;

	FGameplayAbilitySpecHandle TransitionAbilityHandle;

	FDelegateHandle HealthChangedHandle;

	FDelegateHandle AbilityEndedHandle;

	FTimerHandle TransitionFailsafeTimerHandle;
};
