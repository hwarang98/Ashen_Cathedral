// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Structs/ACStructTypes.h"
#include "ACDataAsset_StartupDataBase.h"
#include "ACDataAsset_PlayerStartupData.generated.h"

/**
 * @class UACDataAsset_PlayerStartupData
 * @brief 플레이어의 시작 데이터를 정의하는 데이터 에셋 클래스
 *
 * UCMDataAsset_PlayerStartupData 클래스는 UCMDataAsset_StartupDataBase를 상속하며, 플레이어가 시작할 때 필요한 데이터(예: 기본 능력, 시작 효과 등)를 정의
 * 이 데이터는 플레이어 캐릭터가 생성되면서 자동으로 할당되며, 게임플레이 중 필요한 초기 설정을 제공.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACDataAsset_PlayerStartupData : public UACDataAsset_StartupDataBase
{
	GENERATED_BODY()

public:
	virtual void GiveToAbilitySystemComponent(UACAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData", meta = (TitleProperty = "InputTag"))
	TArray<FACPlayerAbilitySet> PlayerStartUpAbilitySets;
};