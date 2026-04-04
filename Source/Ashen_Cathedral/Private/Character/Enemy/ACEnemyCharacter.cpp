// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/ACEnemyCharacter.h"
#include "Controllers/ACEnemyController.h"
#include "DataAssets/Startup/ACDataAsset_EnemyStartupData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/AssetManager.h"

AACEnemyCharacter::AACEnemyCharacter()
{
	EnemyCombatComponent = CreateDefaultSubobject<UEnemyCombatComponent>(TEXT("Enemy Combat Component"));

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AACEnemyController::StaticClass();

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

UPawnCombatComponent* AACEnemyCharacter::GetPawnCombatComponent() const
{
	return EnemyCombatComponent;
}

void AACEnemyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (CharacterStartUpData.IsNull())
	{
		return;
	}

	int32 AbilityApplyLevel = 1;

	UAssetManager::GetStreamableManager().RequestAsyncLoad(
		CharacterStartUpData.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda(
			[this,AbilityApplyLevel]() {
				if (UACDataAsset_EnemyStartupData* LoadedData = Cast<UACDataAsset_EnemyStartupData>(CharacterStartUpData.Get()))
				{
					LoadedData->GiveToAbilitySystemComponent(ACAbilitySystemComponent, AbilityApplyLevel);
				}
			}
			)
		);
}