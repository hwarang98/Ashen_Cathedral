// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/ACEnemyCharacter.h"
#include "Character/Player/ACPlayerCharacter.h"
#include "Components/RewardCard/ACRewardCardComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/UI/EnemyUIComponent.h"
#include "DataAssets/Startup/ACDataAsset_EnemyStartupData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameModes/ACGameMode.h"
#include "Subsystems/ACRunStateSubsystem.h"
#include "GameplayAbilitySystem/ACAbilitySystemComponent.h"
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
		// 보스 식별 태그는 스폰 직후 부여해 어빌리티/StateTree/연출이 어느 보스인지 태그로 물을 수 있게 한다
		if (ACAbilitySystemComponent && BossIdentityTag.IsValid())
		{
			ACAbilitySystemComponent->AddLooseGameplayTag(BossIdentityTag);
		}

		AACGameMode* ACGameMode = GetWorld()->GetAuthGameMode<AACGameMode>();
		if (ACGameMode)
		{
			ACGameMode->RegisterBossCharacter(this);
		}

		// 최종 스테이지의 보스는 능력(보상 카드) 선택을 생략하므로 등록하지 않는다.
		// 이 시점에는 플레이어 폰만 존재하고 그 BeginPlay는 아직 돌지 않았으므로,
		// RegisterBossCharacter가 델리게이트 바인딩 이상의 일을 하게 만들면 안 된다
		const UACRunStateSubsystem* RunState = UACRunStateSubsystem::Get(this);
		if (RunState && !RunState->IsCurrentStageFinal())
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
