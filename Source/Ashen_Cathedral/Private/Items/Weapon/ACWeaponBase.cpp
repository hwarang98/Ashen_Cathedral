// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapon/ACWeaponBase.h"
#include "NiagaraComponent.h"
#include "ACFunctionLibrary.h"
#include "Components/BoxComponent.h"

// Sets default values
AACWeaponBase::AACWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetCollisionProfileName(TEXT("NoCollision"));
	SetRootComponent(WeaponMesh);

	WeaponCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Weapon Collision Box"));
	WeaponCollisionBox->SetupAttachment(GetRootComponent());
	WeaponCollisionBox->SetBoxExtent(FVector(20.f));
	WeaponCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponCollisionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnCollisionBoxBeginOverlap);
	WeaponCollisionBox->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnCollisionBoxEndOverlap);
}

void AACWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	if (bHideUntilEquipped)
	{
		HideWeapon();
	}
}

void AACWeaponBase::AddGrantedGameplayEffect(FActiveGameplayEffectHandle Handle)
{
	GrantedEffectHandles.Add(Handle);
}

TArray<FActiveGameplayEffectHandle> AACWeaponBase::RemoveGrantedGameplayEffects()
{
	// 복사본을 만들어서 반환
	TArray<FActiveGameplayEffectHandle> HandlesToRemove = GrantedEffectHandles;

	// 리스트 초기화
	GrantedEffectHandles.Empty();

	return HandlesToRemove;
}

void AACWeaponBase::HideWeapon() const
{
	if (WeaponMesh)
	{
		WeaponMesh->SetVisibility(false);
	}

	// 모든 Niagara 컴포넌트 비활성화
	TArray<UNiagaraComponent*> NiagaraComponents;
	GetComponents<UNiagaraComponent>(NiagaraComponents);

	for (UNiagaraComponent* NiagaraComp : NiagaraComponents)
	{
		if (NiagaraComp)
		{
			NiagaraComp->Deactivate();
		}
	}
}

void AACWeaponBase::ShowWeapon() const
{
	if (WeaponMesh)
	{
		WeaponMesh->SetVisibility(true);
	}

	// 모든 Niagara 컴포넌트 활성화
	TArray<UNiagaraComponent*> NiagaraComponents;
	GetComponents<UNiagaraComponent>(NiagaraComponents);

	for (UNiagaraComponent* NiagaraComp : NiagaraComponents)
	{
		if (NiagaraComp)
		{
			NiagaraComp->Activate();
		}
	}
}

void AACWeaponBase::OnCollisionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 무기를 소유한 캐릭터를 가져옴 (보통 무기를 들고 있는 플레이어)
	const APawn* WeaponOwningPawn = GetInstigator<APawn>();

	checkf(WeaponOwningPawn, TEXT("무기의 소유 폰을 instigator로 설정하는 걸 잊었습니다: %s"), *GetName());

	if (const APawn* HitPawn = Cast<APawn>(OtherActor))
	{
		if (HitPawn == WeaponOwningPawn)
		{
			return; // 자기 자신이므로 무시
		}

		// 아군 공격 무시 옵션이 활성화된 경우 적대적인 대상만 공격
		if (bIgnoreFriendly)
		{
			if (UACFunctionLibrary::IsTargetPawnHostile(WeaponOwningPawn, HitPawn))
			{
				OnWeaponHitTarget.ExecuteIfBound(OtherActor);
			}
		}
		else
		{
			// 아군 공격 무시 비활성화 시 모든 대상 공격
			OnWeaponHitTarget.ExecuteIfBound(OtherActor);
		}
	}
}

void AACWeaponBase::OnCollisionBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	const APawn* WeaponOwningPawn = GetInstigator<APawn>();

	checkf(WeaponOwningPawn, TEXT("무기의 소유 폰을 instigator로 설정하는 걸 잊었습니다: %s"), *GetName());

	if (const APawn* HitPawn = Cast<APawn>(OtherActor))
	{
		if (HitPawn == WeaponOwningPawn)
		{
			return; // 자기 자신이므로 무시
		}

		// 아군 공격 무시 옵션이 활성화된 경우 적대적인 대상만 처리
		if (bIgnoreFriendly)
		{
			if (UACFunctionLibrary::IsTargetPawnHostile(WeaponOwningPawn, HitPawn))
			{
				OnWeaponPulledFromTarget.ExecuteIfBound(OtherActor);
			}
		}
		else
		{
			// 아군 공격 무시 비활성화 시 모든 대상 처리
			OnWeaponPulledFromTarget.ExecuteIfBound(OtherActor);
		}
	}
}