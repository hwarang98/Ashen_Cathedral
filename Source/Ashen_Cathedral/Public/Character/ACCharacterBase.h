// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Interfaces/PawnCombatInterface.h"
#include "Interfaces/PawnUIInterface.h"
#include "ACCharacterBase.generated.h"

// AACCharacterBase를 델리게이트 인자로 사용하기 위한 전방 선언
class AACCharacterBase;

/**
 * 캐릭터 사망 시 외부 시스템(UI, GameMode, 퀘스트 등)에 알리기 위한 델리게이트.
 * 사망한 캐릭터를 인자로 넘겨주므로 수신자가 누가 죽었는지 파악할 수 있음.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeath, AACCharacterBase*, DeadCharacter);

class UACDataAsset_StartupDataBase;
class UACAttributeSet;
class UACAbilitySystemComponent;
class UMotionWarpingComponent;

UCLASS(Abstract)
class ASHEN_CATHEDRAL_API AACCharacterBase : public ACharacter, public IAbilitySystemInterface, public IPawnCombatInterface, public IPawnUIInterface
{
	GENERATED_BODY()

public:
	AACCharacterBase();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UPawnCombatComponent* GetPawnCombatComponent() const override;
	virtual UPawnUIComponent* GetPawnUIComponent() const override;

	FORCEINLINE UACAbilitySystemComponent* GetACAbilitySystemComponent() const { return ACAbilitySystemComponent; }
	FORCEINLINE UACAttributeSet* GetACAttributeSet() const { return ACAttributeSet; }
	FORCEINLINE TSoftObjectPtr<UACDataAsset_StartupDataBase> GetCharacterStartUpData() const { return CharacterStartUpData; }
	FORCEINLINE UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }

	/**
	 * 캐릭터 사망 브로드캐스트 델리게이트.
	 * 사망 어빌리티에서 Broadcast() 호출 시 바인딩된 모든 리스너에게 전달됨.
	 * 외부 시스템(UI, GameMode, 퀘스트 등)에서 OnDeathDelegate.AddDynamic()으로 구독할 것.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Death")
	FOnCharacterDeath OnDeathDelegate;

	/**
	 * 사망 애니메이션 재생이 끝났을 때 브로드캐스트되는 델리게이트.
	 * OnDeathDelegate(HP 0 판정 즉시)와 달리 사망 몽타주가 끝까지 재생된 시점에 호출됨.
	 * 사망 연출 완료를 기준으로 동작해야 하는 시스템(보스 클리어 UI 등)에서 구독할 것.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Death")
	FOnCharacterDeath OnDeathAnimationCompletedDelegate;

protected:
	#pragma region GAS
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UACAbilitySystemComponent> ACAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	TObjectPtr<UACAttributeSet> ACAttributeSet;
	#pragma endregion

	#pragma region DataAssets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterData | DataAsset")
	TSoftObjectPtr<UACDataAsset_StartupDataBase> CharacterStartUpData;
	#pragma endregion

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MotionWarping")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;
};