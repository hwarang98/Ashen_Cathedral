// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AnimNotify/ACAnimNotify_PlayWeaponTrail.h"
#include "ACFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"

void UACAnimNotify_PlayWeaponTrail::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	UNiagaraSystem* EffectToPlay = UACFunctionLibrary::ResolveWeaponTrailEffect(MeshComp->GetOwner(), SocketName, DefaultNiagaraSystem.Get());
	if (!EffectToPlay)
	{
		return;
	}

	// 무기 메시가 아닌 캐릭터 스켈레탈 메시(MeshComp)의 소켓 기준으로 부착 — 기존 AnimNotify_PlayNiagaraEffect와 동일한 부착 대상
	UNiagaraFunctionLibrary::SpawnSystemAttached(
		EffectToPlay,
		MeshComp,
		SocketName,
		LocationOffset,
		RotationOffset,
		Scale,
		EAttachLocation::KeepRelativeOffset,
		bAutoDestroy,
		ENCPoolMethod::None
		);
}

FString UACAnimNotify_PlayWeaponTrail::GetNotifyName_Implementation() const
{
	return TEXT("Play Weapon Trail");
}
