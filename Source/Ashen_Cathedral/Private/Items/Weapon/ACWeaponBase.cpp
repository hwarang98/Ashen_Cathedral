// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapon/ACWeaponBase.h"
#include "NiagaraComponent.h"
#include "ACFunctionLibrary.h"
#include "Components/BoxComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

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
	WeaponCollisionBox->SetHiddenInGame(true);
	WeaponCollisionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnCollisionBoxBeginOverlap);
	WeaponCollisionBox->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnCollisionBoxEndOverlap);
}

UNiagaraSystem* AACWeaponBase::GetTrailEffectOverride(FName SocketName) const
{
	for (const FACPhase2NiagaraAttachment& Override : TrailEffectOverrides)
	{
		if (Override.SocketName == SocketName)
		{
			return Override.NiagaraSystem;
		}
	}

	return nullptr;
}

UMeshComponent* AACWeaponBase::GetWeaponMeshComponent() const
{
	if (USkeletalMeshComponent* SkelMesh = FindComponentByClass<USkeletalMeshComponent>())
	{
		return SkelMesh;
	}
	return WeaponMesh;
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
	if (UMeshComponent* Mesh = GetWeaponMeshComponent())
	{
		Mesh->SetVisibility(false);
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
	if (UMeshComponent* Mesh = GetWeaponMeshComponent())
	{
		Mesh->SetVisibility(true);
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

		// 실제 충돌 위치/법선/피격 컴포넌트/BoneName을 GameplayCue까지 전달하기 위해 여기서 한 번만 확정한다.
		const FHitResult ResolvedHitResult = ResolveWeaponHitResult(OtherActor, OtherComp, bFromSweep, SweepResult);

		// 아군 공격 무시 옵션이 활성화된 경우 적대적인 대상만 공격
		if (bIgnoreFriendly)
		{
			if (UACFunctionLibrary::IsTargetPawnHostile(WeaponOwningPawn, HitPawn))
			{
				OnWeaponHitTarget.ExecuteIfBound(OtherActor, ResolvedHitResult);
			}
		}
		else
		{
			// 아군 공격 무시 비활성화 시 모든 대상 공격
			OnWeaponHitTarget.ExecuteIfBound(OtherActor, ResolvedHitResult);
		}
	}
}

FHitResult AACWeaponBase::ResolveWeaponHitResult(AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep, const FHitResult& SweepResult) const
{
	// 스윕에서 발생한 오버랩이면 엔진이 채워준 실제 충돌 정보를 그대로 쓴다.
	if (bFromSweep
		&& SweepResult.GetActor() == OtherActor
		&& SweepResult.GetComponent() != nullptr
		&& !SweepResult.ImpactPoint.ContainsNaN()
		&& !SweepResult.ImpactPoint.IsNearlyZero())
	{
		return SweepResult;
	}

	FHitResult Result;
	Result.HitObjectHandle = FActorInstanceHandle(OtherActor);
	Result.Component = OtherComp;

	const FVector WeaponLocation = WeaponCollisionBox ? WeaponCollisionBox->GetComponentLocation() : GetActorLocation();

	// 무기 콜리전 박스 중심에서 가장 가까운 대상 표면 지점을 접촉점으로 삼는다.
	// GetClosestPointOnCollision은 실패 시 -1을 반환하며 OutPoint를 채우지 않는다.
	FVector ClosestPoint = FVector::ZeroVector;
	const float DistanceToSurface = OtherComp ? OtherComp->GetClosestPointOnCollision(WeaponLocation, ClosestPoint) : -1.f;

	if (DistanceToSurface >= 0.f && !ClosestPoint.ContainsNaN())
	{
		Result.ImpactPoint = ClosestPoint;
	}
	else if (OtherActor)
	{
		// 표면 위치를 구할 수 없을 때만 액터 위치를 최종 폴백으로 사용한다.
		Result.ImpactPoint = OtherActor->GetActorLocation();
	}
	else
	{
		Result.ImpactPoint = WeaponLocation;
	}
	Result.Location = Result.ImpactPoint;

	// GetSafeNormal은 길이가 0에 가까우면 ZeroVector를 반환하므로 NaN이 전달되지 않는다.
	FVector ImpactNormal = (WeaponLocation - Result.ImpactPoint).GetSafeNormal();
	if (ImpactNormal.IsNearlyZero() && OtherActor)
	{
		ImpactNormal = (GetActorLocation() - OtherActor->GetActorLocation()).GetSafeNormal();
	}
	if (ImpactNormal.IsNearlyZero())
	{
		ImpactNormal = FVector::UpVector;
	}
	Result.ImpactNormal = ImpactNormal;
	Result.Normal = ImpactNormal;

	// 스켈레탈 메시에 직접 맞은 경우에만 BoneName을 채운다 (캡슐 오버랩이면 채울 수 없다).
	if (const USkinnedMeshComponent* SkinnedMesh = Cast<USkinnedMeshComponent>(OtherComp))
	{
		Result.BoneName = SkinnedMesh->FindClosestBone(WeaponLocation);
	}

	return Result;
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