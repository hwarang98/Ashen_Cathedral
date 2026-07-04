// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "ACAnimNotifyState_AddGameplayTag.generated.h"

/**
 * @brief 몽타주 구간 동안 지정된 GameplayTag를 대상 액터에 부여/제거하는 범용 AnimNotifyState.
 *
 * NotifyBegin에서 IACAnimNotifyTagReceiverInterface를 통해 GameplayTags를 대상에 전달하고,
 * bRemoveOnEnd가 true면 NotifyEnd에서 동일하게 제거를 요청한다.
 */
UCLASS(meta = (DisplayName = "ANS_AddGameplayTag"))
class ASHEN_CATHEDRAL_API UACAnimNotifyState_AddGameplayTag : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayTag")
	FGameplayTagContainer GameplayTags;

	// NotifyEnd 시 GameplayTags를 제거할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayTag")
	bool bRemoveOnEnd = true;
};