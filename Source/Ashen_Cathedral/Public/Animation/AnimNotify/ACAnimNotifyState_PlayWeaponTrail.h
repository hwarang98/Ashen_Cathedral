// 공격 몽타주 구간 동안 무기 Trail 이펙트를 재생하되, 무기의 런타임 오버라이드가 있으면 그것을 우선 재생하는 AnimNotifyState

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ACAnimNotifyState_PlayWeaponTrail.generated.h"

class UNiagaraSystem;

/**
 * @brief 공격 몽타주에 배치해 구간 동안 캐릭터 메시 소켓에 Trail Niagara 이펙트를 재생하는 AnimNotifyState.
 *
 * NotifyBegin에서 소유 Pawn의 현재 장착 무기(AACWeaponBase)에 TrailEffectOverride가 설정되어 있으면
 * 그 이펙트를, 없으면 DefaultNiagaraSystem을 루핑 재생하고, NotifyEnd에서 재생을 중단(Deactivate)한다.
 * ANS_PlayWeaponTrail이 필요한 단발성 효과라면 UACAnimNotify_PlayWeaponTrail을 사용한다.
 */
UCLASS(meta = (DisplayName = "ANS_PlayWeaponTrail"))
class ASHEN_CATHEDRAL_API UACAnimNotifyState_PlayWeaponTrail : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	// 무기의 TrailEffectOverride가 없을 때 재생할 기본 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	TObjectPtr<UNiagaraSystem> DefaultNiagaraSystem;

	// 이펙트를 부착할 캐릭터 메시 소켓
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (AnimNotifyBoneName = "true"))
	FName SocketName;

	// 소켓 기준 위치 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	FVector LocationOffset = FVector::ZeroVector;

	// 소켓 기준 회전 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	FRotator RotationOffset = FRotator::ZeroRotator;

	// 이펙트 스케일
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	FVector Scale = FVector(1.f);

	// true면 NotifyEnd에서 잔여 파티클 없이 즉시 파괴, false면 남은 파티클을 재생하며 자연스럽게 소멸
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	bool bDestroyImmediatelyOnEnd = false;
};
