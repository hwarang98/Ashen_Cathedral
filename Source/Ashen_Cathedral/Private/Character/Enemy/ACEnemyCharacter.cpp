// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/ACEnemyCharacter.h"

#include "Components/WidgetComponent.h"
#include "Components/UI/EnemyUIComponent.h"
#include "Controllers/ACEnemyController.h"
#include "DataAssets/Startup/ACDataAsset_EnemyStartupData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/AssetManager.h"
#include "Widget/ACWidgetBase.h"

AACEnemyCharacter::AACEnemyCharacter()
{
	EnemyCombatComponent = CreateDefaultSubobject<UEnemyCombatComponent>(TEXT("Enemy Combat Component"));
	EnemyUIComponent = CreateDefaultSubobject<UEnemyUIComponent>(TEXT("EnemyUIComponent"));
	EnemyHealthWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("Enemy Health Widget Component"));

	EnemyHealthWidgetComponent->SetupAttachment(GetMesh());

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AACEnemyController::StaticClass();

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void AACEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UACWidgetBase* EnemyHealthWidget = Cast<UACWidgetBase>(EnemyHealthWidgetComponent->GetUserWidgetObject()))
	{
		EnemyHealthWidget->InitEnemyCreatedWidget(this);
	}

}

UPawnCombatComponent* AACEnemyCharacter::GetPawnCombatComponent() const
{
	return EnemyCombatComponent;
}

UPawnUIComponent* AACEnemyCharacter::GetPawnUIComponent() const
{
	return EnemyUIComponent;
}

UEnemyUIComponent* AACEnemyCharacter::GetEnemyUIComponent() const
{
	return EnemyUIComponent;
}

void AACEnemyCharacter::OnDeath()
{
	EnemyUIComponent->RemoveEnemyDrawnWidgetsIfAny();
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