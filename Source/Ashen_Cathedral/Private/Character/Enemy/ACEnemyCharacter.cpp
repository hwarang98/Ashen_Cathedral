// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/ACEnemyCharacter.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/RewardCard/ACRewardCardComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/UI/EnemyUIComponent.h"
#include "Controllers/ACEnemyController.h"
#include "DataAssets/Startup/ACDataAsset_EnemyStartupData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameModes/ACGameMode.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
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

	if (bIsBoss)
	{
		AACGameMode* ACGameMode = GetWorld()->GetAuthGameMode<AACGameMode>();
		if (ACGameMode)
		{
			ACGameMode->RegisterBossCharacter(this);
		}

		// 최종 보스는 능력(보상 카드) 선택을 생략하므로 등록하지 않는다
		if (ACGameMode && !ACGameMode->IsFinalBossPending())
		{
			if (const AACPlayerCharacter* PlayerCharacter = Cast<AACPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
			{
				if (UACRewardCardComponent* RewardCardComponent = PlayerCharacter->GetRewardCardComponent())
				{
					RewardCardComponent->RegisterBossCharacter(this);
				}
			}
		}
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

	TWeakObjectPtr<AACEnemyCharacter> WeakThis(this);
	UAssetManager::GetStreamableManager().RequestAsyncLoad(
		CharacterStartUpData.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda(
			[WeakThis,AbilityApplyLevel]() {
				AACEnemyCharacter* StrongThis = WeakThis.Get();
				if (!StrongThis || !StrongThis->ACAbilitySystemComponent)
				{
					return;
				}

				if (UACDataAsset_EnemyStartupData* LoadedData = Cast<UACDataAsset_EnemyStartupData>(StrongThis->CharacterStartUpData.Get()))
				{
					LoadedData->GiveToAbilitySystemComponent(StrongThis->ACAbilitySystemComponent, AbilityApplyLevel);
				}
			}
			)
		);
}