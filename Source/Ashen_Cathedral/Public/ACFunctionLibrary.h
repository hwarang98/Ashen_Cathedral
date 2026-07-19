// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enums/ACEnums.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ACFunctionLibrary.generated.h"

class UPawnCombatComponent;
class UACAbilitySystemComponent;
class UNiagaraSystem;
struct FGameplayTag;
/**
 * 
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Actor에서 AbilitySystemComponent를 반환
	 *
	 * @param InActor 검색 대상 Actor
	 * @return Actor에 연결된 UACAbilitySystemComponent가 존재하면 해당 인스턴스를 반환하고, 그렇지 않으면 nullptr을 반환
	 */
	static UACAbilitySystemComponent* NativeAbilitySystemComponentFromActor(AActor* InActor);

	/**
	 * Actor가 주어진 GameplayTag를 가지고 있지 않은 경우, 해당 태그를 추가
	 *
	 * @param InActor 태그를 추가할 Actor
	 * @param TagToAdd 추가하려는 GameplayTag
	 */
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|FunctionLibrary")
	static void AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd);

	/**
	 * Actor에서 특정 GameplayTag를 제거
	 *
	 * @param InActor 태그를 제거할 Actor
	 * @param TagToRemove 제거하려는 GameplayTag
	 *
	 * @details Actor에 연결된 AbilitySystemComponent를 이용하여 주어진 태그가 존재하면 해당 태그를 제거한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|FunctionLibrary")
	static void RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove);

	/**
	 * Actor의 AbilitySystemComponent에 GameplayTagContainer 전체를 Loose Tag로 추가
	 *
	 * @param InActor 태그를 추가할 Actor
	 * @param TagsToAdd 추가하려는 GameplayTagContainer
	 */
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|FunctionLibrary")
	static void AddGameplayTagsToActor(AActor* InActor, const FGameplayTagContainer& TagsToAdd);

	/**
	 * Actor의 AbilitySystemComponent에서 GameplayTagContainer 전체를 Loose Tag로 제거
	 *
	 * @param InActor 태그를 제거할 Actor
	 * @param TagsToRemove 제거하려는 GameplayTagContainer
	 */
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|FunctionLibrary")
	static void RemoveGameplayTagsFromActor(AActor* InActor, const FGameplayTagContainer& TagsToRemove);

	/**
	 * 주어진 Actor가 특정 GameplayTag를 가지고 있는지 확인
	 *
	 * @param InActor 검사 대상 Actor
	 * @param TagToCheck Actor의 태그 목록에서 확인할 GameplayTag
	 * @return Actor가 지정된 태그를 가지고 있다면 true를 반환하고, 그렇지 않으면 false를 반환
	 */
	static bool NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck);

	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|FunctionLibrary", meta = (DisplayName = "Does Actor Have Tag", ExpandEnumAsExecs = "OutConfirmType"))
	static void BP_DoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck, EACConfirmType& OutConfirmType);

	/**
	 * QueryPawn과 TargetPawn 간의 적대 관계를 확인
	 *
	 * @param QueryPawn 적대 관계를 확인하는 주체 Pawn
	 * @param TargetPawn 적대 관계를 확인할 대상 Pawn
	 * @return QueryPawn과 TargetPawn이 적대 관계라면 true를 반환, 그렇지 않으면 false를 반환
	 *
	 * @details 이 함수는 두 Pawn의 Controller가 IGenericTeamAgentInterface를 구현하고 있는지 확인하며, 팀 ID가 다를 경우 적대 관계로 간주한다.
	 * 팀 인식이 없는 경우에는 적대 관계가 아니라고 판단한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathdral|FunctionLibrary")
	static bool IsTargetPawnHostile(const APawn* QueryPawn, const APawn* TargetPawn);

	/* 액터에서 PawnCombatComponent를 가져옴 */
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|FunctionLibrary", meta = (DisplayName = "Get Pawn Combat Component From Actor", ExpandEnumAsExecs = "OutValidType"))
	static UPawnCombatComponent* BP_GetPawnCombatComponentFromActor(AActor* InActor, EACValidType& OutValidType);

	/* 내부적으로 PawnCombatComponent를 직접 검색 */
	static UPawnCombatComponent* NativeGetPawnCombatComponentFromActor(AActor* InActor);

	/**
	 * 무기 Trail 노티파이/노티파이 스테이트가 재생할 Niagara 이펙트를 결정한다.
	 * 소유 액터의 현재 장착 무기에 SocketName과 일치하는 TrailEffectOverride가 있으면 그것을, 없으면 InDefaultNiagaraSystem을 반환한다.
	 */
	static UNiagaraSystem* ResolveWeaponTrailEffect(AActor* InOwner, FName SocketName, UNiagaraSystem* InDefaultNiagaraSystem);

	/* 캐릭터가 맞은 위치 별 태그 반환 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathdral|FunctionLibrary")
	static FGameplayTag ComputeHitReactDirectionTag(const AActor* InAttacker, const AActor* InVictim, float& OutAngleDifference);

	/**
	 * 방어자가 공격자를 향해 블록 가능한 각도 범위 내에 있는지 확인합니다.
	 * @param InAttacker 공격자
	 * @param InDefender 방어자
	 * @param AngleThreshold 블록 허용 각도 (도 단위, 기본값 60도)
	 * @return 유효한 블록이면 true, 아니면 false
	 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathdral|FunctionLibrary")
	static bool IsValidBlock(const AActor* InAttacker, const AActor* InDefender, const float AngleThreshold = 60.0f);

	/**
	 * @brief Actor가 Player.Status.Blocking 또는 Enemy.Status.Blocking 중 하나라도 가지고 있는지 확인한다.
	 * Player 전용 Blocking 태그를 리네임하지 않고 Boss(Enemy) Blocking 태그와 함께 검사할 수 있도록
	 * TryTriggerSuccessfulBlockEvent/ACAbility_Attack/ACCalculation_DamageTaken이 공유하는 판정 진입점이다.
	 *
	 * @param InActor 검사 대상 Actor
	 * @return 둘 중 하나라도 있으면 true
	 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathdral|FunctionLibrary")
	static bool IsActorBlocking(const AActor* InActor);

	/** 공격 태그에 Shared.Attack.Blockable이 있고 Shared.Attack.Unblockable이 없으면 true */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathdral|FunctionLibrary")
	static bool IsAttackBlockable(const FGameplayTagContainer& AttackTags);

	/** 공격 태그에 Shared.Attack.Parryable이 있고 Shared.Attack.Unparryable이 없으면 true */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathdral|FunctionLibrary")
	static bool IsAttackParryable(const FGameplayTagContainer& AttackTags);

	/**
	 * @brief 실제 Block 성공 여부를 판정하는 공통 진입점.
	 * 데미지 감쇄(ACCalculation_DamageTaken), HitReact 억제(ACAttributeSet), SuccessfulBlock 이벤트 발송
	 * (TryTriggerSuccessfulBlockEvent)이 전부 이 판정을 공유해 서로 어긋나지 않게 한다.
	 *
	 * 성공 조건: Attacker/Defender 유효 && 공격이 Blockable(Unblockable 아님) && Defender가 Blocking 상태
	 * && Attacker가 Defender의 정면 방어 각도(IsValidBlock) 안에 있음.
	 *
	 * @param Attacker   공격자
	 * @param Defender   방어자
	 * @param AttackTags 현재 공격의 방어 가능 속성(Shared.Attack.*)
	 * @return 위 조건을 모두 만족하면 true
	 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathdral|FunctionLibrary")
	static bool IsSuccessfulBlock(const AActor* Attacker, const AActor* Defender, const FGameplayTagContainer& AttackTags);

	/**
	 * @brief 실제 Parry 성공 여부를 판정하는 공통 진입점 (IsSuccessfulBlock의 Parry 버전).
	 * 데미지 무효화/역공(ACCalculation_DamageTaken), SuccessfulBlock 이벤트 발송(TryTriggerSuccessfulBlockEvent),
	 * 일반 Hit GameplayCue 억제(UACAbility_Attack)가 전부 이 판정을 공유해 서로 어긋나지 않게 한다.
	 *
	 * 성공 조건: Attacker/Defender 유효 && 공격이 Parryable(Unparryable 아님) && Defender가 Shared.Status.Parry 상태
	 * && Attacker가 Defender의 정면 방어 각도(IsValidBlock) 안에 있음.
	 *
	 * @param Attacker   공격자
	 * @param Defender   방어자
	 * @param AttackTags 현재 공격의 방어 가능 속성(Shared.Attack.*)
	 * @return 위 조건을 모두 만족하면 true
	 */
	UFUNCTION(BlueprintPure, Category = "Ashen Cathdral|FunctionLibrary")
	static bool IsSuccessfulParry(const AActor* Attacker, const AActor* Defender, const FGameplayTagContainer& AttackTags);

	/**
	 * @brief HitActor가 유효한 각도에서 Parry/Block 중이고, 이 공격이 그걸 허용하는 태그를 가지고 있으면
	 * Player.Event.SuccessfulBlock 이벤트를 HitActor에게 보낸다.
	 * GA_Block이 이 이벤트를 받아 Block/Parry GameplayCue 재생, 넉백, 카운터어택 윈도우 부여를 처리한다.
	 * 무기 콜리전 기반 근접 공격(PawnCombatComponent)과 AOE 판정이 공통으로 사용한다.
	 *
	 * @param Attacker 공격자
	 * @param HitActor 피격된 대상 액터
	 * @param AttackDefenseTags 현재 공격의 방어 가능 속성(Shared.Attack.Parryable/Blockable/Unparryable/Unblockable).
	 * 비어있으면 Parry/Block 둘 다 성공하지 않는다.
	 * @return 이벤트를 보냈으면(=유효한 Parry 또는 Block이었으면) true
	 */
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|FunctionLibrary")
	static bool TryTriggerSuccessfulBlockEvent(const AActor* Attacker, AActor* HitActor, const FGameplayTagContainer& AttackDefenseTags);

	/**
	 * @brief 콤보를 유지해야 하는 공격 몽타주 조기 캔슬(이동 등) 직전에 호출한다.
	 * InActor에서 현재 애니메이팅 중인 어빌리티를 찾아 UACAbility_Attack이면 콤보 즉시 리셋을 막도록 표시한다.
	 * ANS_EarlyBlend가 Montage_Stop 호출 직전에 사용한다.
	 *
	 * @param InActor 몽타주를 재생 중인 액터
	 */
	UFUNCTION(BlueprintCallable, Category = "Ashen Cathdral|FunctionLibrary")
	static void RequestAttackMontageSoftCancel(AActor* InActor);

private:
	/* 주어진 각도 차이를 바탕으로 히트 반응 태그를 결정 */
	static FGameplayTag DetermineHitReactionTag(const float& OutAngleDifference);
};
