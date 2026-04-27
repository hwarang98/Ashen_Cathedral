// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayTagContainer.h"
#include "InputAction.h"
#include "NiagaraSystem.h"
#include "Enums/ACEnums.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "ACStructTypes.generated.h"

class UTexture2D;
class AACWeaponBase;
class UACGameplayAbility;


/**
 * @struct FCAInputActionConfig
 * @brief 입력 액션과 관련된 구성을 정의하는 데이터 구조체.
 *
 * 이 구조체는 게임플레이 태그와 해당 태그에 매핑된 입력 액션을 설정하는 데 사용한다.
 * 주로 입력 설정 데이터 에셋이나 입력 행위를 바인딩할 때 활용한다.
 */
USTRUCT(BlueprintType)
struct FCAInputActionConfig
{
	GENERATED_BODY()


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputAction> InputAction;

	bool IsValid() const;
};

/**
 * @struct FACPlayerAbilitySet
 * @brief 플레이어의 어빌리티 설정을 정의하는 데이터 구조체.
 *
 * 이 구조체는 특정 입력 태그와 UI 슬롯 태그에 따라 플레이어가 사용할 수 있는 어빌리티를 정의한다.
 * 어빌리티 부여, UI 아이콘 참조, 쿨다운 태그 설정 등에 사용한다.
 *
 * @details
 * - InputTag: 입력 시스템과 연관된 태그.
 * - SlotTag: UI 슬롯과 매핑에 사용되는 태그.
 * - AbilityToGrant: 설정된 어빌리티 클래스.
 * - SoftAbilityIconMaterial: 어빌리티 UI 아이콘을 위한 에셋.
 * - CooldownTag: 어빌리티의 쿨다운을 트래킹하는데 사용되는 태그.
 */
USTRUCT(BlueprintType)
struct FACPlayerAbilitySet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	// UI 슬롯 태그 (예: InputTag_Skill_Q) - 캐릭터 클래스와 무관하게 UI 슬롯 매핑용
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Player.Skill"))
	// FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UACGameplayAbility> AbilityToGrant;

	// UPROPERTY(EditAnywhere, BlueprintReadOnly)
	// TSoftObjectPtr<UTexture2D> SoftAbilityIconMaterial;

	// 쿨다운 태그 (GE에서 사용하는 CooldownTag)
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Cooldown"))
	// FGameplayTag CooldownTag;

	bool IsValid() const;
};

USTRUCT()
struct FWeaponEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag WeaponTag;

	UPROPERTY()
	TObjectPtr<AACWeaponBase> WeaponActor;
};

struct FCADamageCapture
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower)
	DECLARE_ATTRIBUTE_CAPTUREDEF(DefensePower)
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageTaken)
	DECLARE_ATTRIBUTE_CAPTUREDEF(GroggyDamageTaken)
	DECLARE_ATTRIBUTE_CAPTUREDEF(BurnAccumulation)

	FCADamageCapture()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UACAttributeSet, AttackPower, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UACAttributeSet, DefensePower, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UACAttributeSet, DamageTaken, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UACAttributeSet, GroggyDamageTaken, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UACAttributeSet, BurnAccumulation, Target, false);
	}
};

struct FRotateToFaceTargetTaskMemory
{
	TWeakObjectPtr<APawn> OwningPawn;
	TWeakObjectPtr<AActor> TargetActor;

	bool IsValid() const
	{
		return OwningPawn.IsValid() && TargetActor.IsValid();
	}

	void Reset()
	{
		OwningPawn.Reset();
		TargetActor.Reset();
	}
};

/**
 * 보상 카드 한 장의 정의 데이터. FTableRowBase를 상속하므로 UDataTable에서 사용 가능.
 * RowName이 카드 고유 ID 역할을 하며, CardID는 컴포넌트가 런타임에 RowName으로 채운다.
 * CSV 임포트 대상 컬럼: CardName, Description, Category, Rarity, MaxStack, Weight, bIsMVP
 * 에디터에서 수동 설정 컬럼: GameplayEffectClass, GrantedAbilityClass, Icon
 */
USTRUCT(BlueprintType)
struct FACRewardCardData : public FTableRowBase
{
	GENERATED_BODY()

	// 런타임에 DataTable RowName으로 자동 채워짐 — DataTable 에디터에서 비워도 무방
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName CardID = NAME_None;

	// UI에 표시할 카드 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText CardName;

	// 카드 효과 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	// 카드 계열
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EACCardCategory Category = EACCardCategory::Attack;

	// 카드 희귀도
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EACCardRarity Rarity = EACCardRarity::Common;

	// Run 내 최대 중첩 가능 횟수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1))
	int32 MaxStack = 1;

	// 추첨 가중치 (높을수록 더 자주 등장)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.01f))
	float Weight = 1.0f;

	// 선택 시 플레이어 ASC에 적용할 Infinite GameplayEffect
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	// 선택 시 플레이어 ASC에 부여할 Ability (Passive GA 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UACGameplayAbility> GrantedAbilityClass;

	// UI에 표시할 카드 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	// MVP 구현 대상 카드 여부 (우선 구현 표시용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsMVP = false;

	bool IsValid() const { return CardID != NAME_None; }
};

/** 위젯에 전달하는 카드 표시 정보 — 카드 데이터와 현재 중첩 수를 함께 전달 */
USTRUCT(BlueprintType)
struct FACRewardCardDisplayInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FACRewardCardData CardData;

	// 현재 Run에서 이미 획득한 중첩 수
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentStack = 0;
};

/** 소켓 이름과 Niagara 시스템을 쌍으로 묶어 복수 소켓에 이펙트를 부착할 때 사용합니다. */
USTRUCT(BlueprintType)
struct FACPhase2NiagaraAttachment
{
	GENERATED_BODY()

	/** 이펙트를 부착할 소켓 또는 본 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName SocketName;

	/** 부착할 Niagara 시스템 에셋 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> NiagaraSystem;
};