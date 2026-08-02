#pragma once

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/SkinnedAsset.h"
#include "GameFramework/Actor.h"

namespace Debug
{
	static void Print(const FString& Msg, const FColor& Color = FColor::MakeRandomColor(), int32 InKey = -1)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(InKey, 7.f, Color, Msg);

			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		}
	}

	static void Print(const FString& FloatTitle, float FloatValueToPrint, int32 InKey = -1, const FColor& Color = FColor::MakeRandomColor())
	{
		if (GEngine)
		{
			const FString FinalMsg = FloatTitle + TEXT(": ") + FString::SanitizeFloat(FloatValueToPrint);

			GEngine->AddOnScreenDebugMessage(InKey, 7.f, Color, FinalMsg);

			UE_LOG(LogTemp, Warning, TEXT("%s"), *FinalMsg);
		}
	}

	/**
	 * @brief 무기 부착에 쓰인 소켓이 실제로 어떤 본으로 해석되는지 로그로 남긴다.
	 *
	 * @param Context 호출 지점을 구분하는 접두어
	 * @param Mesh 부착 대상 스켈레탈 메시 컴포넌트
	 * @param SocketName 부착에 사용한 소켓 이름
	 * @param AttachedActor 부착된 무기 액터. nullptr이면 액터 상태 로그를 생략한다
	 * @note 소켓을 찾아도 부모 본이 메시에 없으면 본 인덱스가 -1이 되고 부착 위치가 메시 원점으로 떨어진다
	 */
	static void LogWeaponAttach(const FString& Context, const USkeletalMeshComponent* Mesh, FName SocketName, const AActor* AttachedActor)
	{
		if (!Mesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WeaponAttach][%s] Mesh가 null이라 확인 불가"), *Context);
			return;
		}

		FTransform SocketLocalTransform;
		int32 SocketBoneIndex = INDEX_NONE;
		const USkeletalMeshSocket* Socket = Mesh->GetSocketInfoByName(SocketName, SocketLocalTransform, SocketBoneIndex);

		UE_LOG(LogTemp, Warning, TEXT("[WeaponAttach][%s] 요청소켓=%s | Mesh=%s | Asset=%s | 소켓찾음=%s | 소켓부모본=%s | 부모본인덱스=%d | 본이름폴백인덱스=%d"),
			*Context,
			*SocketName.ToString(),
			*Mesh->GetName(),
			*GetNameSafe(Mesh->GetSkinnedAsset()),
			Socket ? TEXT("O") : TEXT("X"),
			Socket ? *Socket->BoneName.ToString() : TEXT("-"),
			SocketBoneIndex,
			Mesh->GetBoneIndex(SocketName));

		const FVector MeshOrigin = Mesh->GetComponentLocation();
		const FVector SocketWorld = Mesh->GetSocketLocation(SocketName);

		UE_LOG(LogTemp, Warning, TEXT("[WeaponAttach][%s] 메시원점=%s | 소켓월드=%s | 원점으로떨어짐=%s"),
			*Context,
			*MeshOrigin.ToCompactString(),
			*SocketWorld.ToCompactString(),
			SocketWorld.Equals(MeshOrigin, 0.01f) ? TEXT("예") : TEXT("아니오"));

		if (AttachedActor)
		{
			const USceneComponent* WeaponRoot = AttachedActor->GetRootComponent();

			UE_LOG(LogTemp, Warning, TEXT("[WeaponAttach][%s] 무기=%s | 무기월드=%s | AttachParent=%s | AttachSocket=%s"),
				*Context,
				*AttachedActor->GetName(),
				*AttachedActor->GetActorLocation().ToCompactString(),
				*GetNameSafe(WeaponRoot ? WeaponRoot->GetAttachParent() : nullptr),
				WeaponRoot ? *WeaponRoot->GetAttachSocketName().ToString() : TEXT("-"));
		}
	}
}