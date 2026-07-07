// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AnimNotify/ACAnimNotifyState_PlayWeaponTrail.h"
#include "ACFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

static const FName WeaponTrailComponentTag(TEXT("ACWeaponTrail"));

void UACAnimNotifyState_PlayWeaponTrail::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	UNiagaraSystem* EffectToPlay = UACFunctionLibrary::ResolveWeaponTrailEffect(MeshComp->GetOwner(), SocketName, DefaultNiagaraSystem.Get());
	if (!EffectToPlay)
	{
		return;
	}

	// 무기 메시가 아닌 캐릭터 스켈레탈 메시(MeshComp)의 소켓 기준으로 부착 — UACAnimNotify_PlayWeaponTrail과 동일한 부착 대상
	UNiagaraComponent* SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		EffectToPlay,
		MeshComp,
		SocketName,
		LocationOffset,
		RotationOffset,
		Scale,
		EAttachLocation::KeepRelativeOffset,
		true, // bAutoDestroy — NotifyEnd에서 Deactivate하면 잔여 파티클 재생 후 자동 파괴됨
		ENCPoolMethod::None
		);

	if (SpawnedComponent)
	{
		// 몽타주 애셋의 NotifyState 인스턴스는 동시 재생되는 여러 캐릭터가 공유하므로,
		// 멤버 변수 대신 컴포넌트 태그로 표시해 NotifyEnd에서 MeshComp 기준으로 다시 찾는다.
		SpawnedComponent->ComponentTags.AddUnique(WeaponTrailComponentTag);
	}
}

void UACAnimNotifyState_PlayWeaponTrail::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	for (USceneComponent* Child : MeshComp->GetAttachChildren())
	{
		UNiagaraComponent* NiagaraComponent = Cast<UNiagaraComponent>(Child);
		if (NiagaraComponent && NiagaraComponent->ComponentHasTag(WeaponTrailComponentTag))
		{
			if (bDestroyImmediatelyOnEnd)
			{
				NiagaraComponent->DeactivateImmediate();
			}
			else
			{
				NiagaraComponent->Deactivate();
			}
		}
	}
}

FString UACAnimNotifyState_PlayWeaponTrail::GetNotifyName_Implementation() const
{
	return TEXT("Play Weapon Trail");
}
