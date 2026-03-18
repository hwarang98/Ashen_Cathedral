// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "CADataAsset_InputConfig.generated.h"


struct FCAInputActionConfig;
class UInputAction;
class UInputMappingContext;

/**
 * @class UCADataAsset_InputConfig
 * @brief 입력 설정을 구성하는 DataAsset을 정의하는 클래스. 이 클래스는 입력 매핑 컨텍스트 및
 *        네이티브/능력 입력 액션 목록을 포함하며, 특정 입력 태그에 대한 입력 액션을 검색하는 기능을 제공.
 *
 * UDataAsset을 상속받으며, 다양한 입력 데이터를 Blueprint에서 정의하고 사용할 수 있도록 설계
 */
UCLASS()
class ASHEN_CATHEDRAL_API UCADataAsset_InputConfig : public UDataAsset
{
	GENERATED_BODY()

public: 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> UIMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputTag"))
	TArray<FCAInputActionConfig> NativeInputActions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputTag"))
	TArray<FCAInputActionConfig> AbilityInputActions;

	UInputAction* FindNativeInputActionByTag(const FGameplayTag& InputTag) const;
	UInputAction* FindAbilityInputActionByTag(const FGameplayTag& InputTag) const;
};
