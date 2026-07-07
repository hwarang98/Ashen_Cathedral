// 공격 몽타주에서 무기 Trail 이펙트를 재생하되, 무기의 런타임 오버라이드가 있으면 그것을 우선 재생하는 AnimNotify

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ACAnimNotify_PlayWeaponTrail.generated.h"

class UNiagaraSystem;

/**
 * @brief 공격 몽타주에 배치해 무기 메시 소켓에 Trail Niagara 이펙트를 재생하는 AnimNotify.
 *
 * 재생 시점에 소유 Pawn의 현재 장착 무기(AACWeaponBase)에 TrailEffectOverride가 설정되어 있으면
 * 그 이펙트를, 없으면 DefaultNiagaraSystem을 재생한다. 보스 페이즈 전환처럼 런타임에 무기의
 * Trail 이펙트를 바꿔야 하는 경우 몽타주 수정 없이 무기 액터의 오버라이드만 바꾸면 된다.
 */
UCLASS(meta = (DisplayName = "ANS_PlayWeaponTrail"))
class ASHEN_CATHEDRAL_API UACAnimNotify_PlayWeaponTrail : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	// 무기의 TrailEffectOverride가 없을 때 재생할 기본 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	TObjectPtr<UNiagaraSystem> DefaultNiagaraSystem;

	// 이펙트를 부착할 무기 메시 소켓
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

	// true면 이펙트 재생이 끝나는 대로 스폰된 컴포넌트를 자동 파괴, false면 수동으로 관리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	bool bAutoDestroy = true;
};