// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/ACBTTask_ToggleStrafingState.h"
#include "ACFunctionLibrary.h"
#include "ACGameplayTags.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Enemy/ACEnemyCharacter.h"
#include "Controllers/ACEnemyController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"

UACBTTask_ToggleStrafingState::UACBTTask_ToggleStrafingState()
{
	NodeName = TEXT("BTTask_ToggleStrafing");
}

void UACBTTask_ToggleStrafingState::OnEnemyExecuteTask()
{
	Super::OnEnemyExecuteTask();

	UCharacterMovementComponent* MovementComponent = OwningEnemyCharacter->GetCharacterMovement();

	// === Strafing 활성화 ===
	if (ShouldEnable)
	{
		// 이동 방향으로 캐릭터가 회전하도록 설정
		if (MovementComponent)
		{
			MovementComponent->bOrientRotationToMovement = true;
		}

		// 옵션에 따라 이동 속도 Attribute 변경
		// SetNumericAttributeBase를 통해 PostAttributeChange가 트리거되어 CharacterMovement와 동기화됨
		if (ShouldChangeMaxWalkSpeed)
		{
			if (UACAbilitySystemComponent* ASC = OwningEnemyCharacter->GetACAbilitySystemComponent())
			{
				ASC->SetNumericAttributeBase(UACAttributeSet::GetMoveSpeedAttribute(), MaxWalkSpeed);
			}
		}

		// Strafing 상태 태그 부여
		UACFunctionLibrary::AddGameplayTagToActorIfNone(OwningEnemyCharacter, ACGameplayTags::Enemy_Status_Strafing);
	}

	// === Strafing 비활성화 ===
	if (!ShouldEnable)
	{
		// 이동 방향으로 캐릭터가 회전하도록 복원
		if (MovementComponent)
		{
			MovementComponent->bOrientRotationToMovement = true;
		}

		UACAbilitySystemComponent* ASC = OwningEnemyCharacter->GetACAbilitySystemComponent();
		if (!ASC)
		{
			return;
		}

		// Blackboard에서 기본 이동 속도를 읽어와 Attribute 복원
		if (ShouldChangeMaxWalkSpeed)
		{
			if (const AACEnemyController* EnemyController = Cast<AACEnemyController>(OwningEnemyCharacter->GetController()))
			{
				if (const UBlackboardComponent* Blackboard = EnemyController->GetBlackboardComponent())
				{
					const float DefaultMaxWalkSpeed = Blackboard->GetValueAsFloat(InDefaultMaxWalkSpeedKey.SelectedKeyName);
					const float CurrentMoveSpeed = ASC->GetNumericAttribute(UACAttributeSet::GetMoveSpeedAttribute());
					const float SpeedToRestore = FMath::IsNearlyEqual(CurrentMoveSpeed, MovementComponent->MaxWalkSpeed) ? DefaultMaxWalkSpeed : CurrentMoveSpeed;
					ASC->SetNumericAttributeBase(UACAttributeSet::GetMoveSpeedAttribute(), SpeedToRestore);
				}
			}
		}

		// Strafing 상태 태그 제거
		UACFunctionLibrary::RemoveGameplayTagFromActorIfFound(OwningEnemyCharacter, ACGameplayTags::Enemy_Status_Strafing);
	}
}