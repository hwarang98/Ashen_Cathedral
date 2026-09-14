// Copyright 2024, PrismaticaDev. All rights reserved.

#include "Components/PrismatiscapeDeformComponentCharacter.h"
#include "Engine/World.h"

UPrismatiscapeDeformComponentCharacter::UPrismatiscapeDeformComponentCharacter()
: BoneChains({
		{"Head", FPrismatiscapeBoneChainData({"head", "hairroot"}, {10.f, 15.f})},
		{"Body", FPrismatiscapeBoneChainData({"pelvis", "neck_01"}, {15, 25})},
		{"LeftArm", FPrismatiscapeBoneChainData({"upperarm_l", "lowerarm_l", "hand_l"}, {10, 8, 7})},
		{"RightArm", FPrismatiscapeBoneChainData({"upperarm_r", "lowerarm_r", "hand_r"}, {10, 8, 7})},
		{"LeftLeg", FPrismatiscapeBoneChainData({"thigh_l", "calf_l", "foot_l"}, {12, 10, 8})},
		{"RightLeg", FPrismatiscapeBoneChainData({"thigh_r", "calf_r", "foot_r"}, {12, 10, 8})},
		{"LeftFoot", FPrismatiscapeBoneChainData({"foot_l", "ball_l"}, {8, 6})},
		{"RightFoot", FPrismatiscapeBoneChainData({"foot_r", "ball_r"}, {8, 6})}}),
BoneProfiles({
		{"FullBody", FPrismatiscapeBoneChainNameArray({"Head", "Body", "LeftArm", "RightArm", "LeftLeg", "RightLeg", "LeftFoot", "RightFoot"}, 1.f)},
		{"OnlyFeet", FPrismatiscapeBoneChainNameArray({"LeftFoot", "RightFoot"}, 3.f)}
	})
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPrismatiscapeDeformComponentCharacter::BeginPlay()
{
	Super::BeginPlay();

	SetSkeletalMesh();
	SetActiveProfile(ActiveProfile);
}

void UPrismatiscapeDeformComponentCharacter::SetSkeletalMesh()
{
	if(!GetOwner()) return;

	SkeletalMesh = GetOwner()->FindComponentByClass<UMeshComponent>();
}

void UPrismatiscapeDeformComponentCharacter::GetDeformCapsules(TArray<FVector4>& StartLocationAndRadius, TArray<FVector4>& EndLocationAndRadius, TArray<FVector4>& StartVelocityAndStrength, TArray<FVector4>& EndVelocityAndStrength)
{
	Super::GetDeformCapsules(StartLocationAndRadius, EndLocationAndRadius, StartVelocityAndStrength, EndVelocityAndStrength);

	TRACE_CPUPROFILER_EVENT_SCOPE(UPrismatiscapeDeformComponentCharacter::GetDeformCapsules);

	if(!SkeletalMesh) return;
	
	FPrismatiscapeBoneChainNameArray CurrentProfile = GetActiveBoneProfile();

	int32 CurrentBoneInt = 0;
	float Delta = GetWorld()->DeltaTimeSeconds;

	for (FName BoneChainName : CurrentProfile.BoneChainNames)
	{
		FPrismatiscapeBoneChainData* BoneChain = BoneChains.Find(BoneChainName);
		if(!BoneChain) continue;
		if(BoneChain->BoneNamesRootToTip.IsEmpty()) continue;
		if(BoneChain->RadiiRootToTip.IsEmpty())	continue;

		const FName RootBoneName = BoneChain->BoneNamesRootToTip[0];
		const float RootBoneRadius = BoneChain->RadiiRootToTip[0];
		
		FVector PrevLocation = SkeletalMesh->GetSocketLocation(RootBoneName);
		float PrevRadius = RootBoneRadius * ActorScale;

	
		FVector PreVelocity = (PrevLocation - PrevFramePositions[CurrentBoneInt]) / Delta;
		FVector CurrentVelocity = PreVelocity;

		PrevFramePositions[CurrentBoneInt] = PrevLocation;

		CurrentBoneInt++;

		for(int i = 1; i < BoneChain->BoneNamesRootToTip.Num(); ++i)
		{
			const FName BoneName = BoneChain->BoneNamesRootToTip[i];
			const float CurrentRadius = BoneChain->RadiiRootToTip[i] * ActorScale;

			FVector CurrentPos = SkeletalMesh->GetSocketLocation(BoneName);

			FVector BoneVel = (CurrentPos - PrevFramePositions[CurrentBoneInt]) / Delta;
			if(!BoneVel.IsNearlyZero()) CurrentVelocity = BoneVel;

			PrevFramePositions[CurrentBoneInt] = CurrentPos;

			StartLocationAndRadius.Add(FVector4(PrevLocation, PrevRadius));
			EndLocationAndRadius.Add(FVector4(CurrentPos, CurrentRadius));

			const float VelW = DrawStrength * CurrentProfile.StrengthMultiplier;
			StartVelocityAndStrength.Add(FVector4(PreVelocity, VelW));
			EndVelocityAndStrength.Add(FVector4(CurrentVelocity, VelW));

			PreVelocity = CurrentVelocity;
			PrevLocation = CurrentPos;
			PrevRadius = CurrentRadius;

			CurrentBoneInt++;
		}
	}
}

FPrismatiscapeBoneChainNameArray UPrismatiscapeDeformComponentCharacter::GetActiveBoneProfile()
{
	const FPrismatiscapeBoneChainNameArray* Profile = BoneProfiles.Find(ActiveProfile);
	if(Profile) return *Profile;
	
	return FPrismatiscapeBoneChainNameArray();
}

void UPrismatiscapeDeformComponentCharacter::SetActiveProfile(FName NewProfileName)
{
	const FPrismatiscapeBoneChainNameArray* Profile = BoneProfiles.Find(NewProfileName);
	if (!Profile) return;

	PrevFramePositions.Empty();

	for (FName CurrentBoneChainName : Profile->BoneChainNames)
	{
		const FPrismatiscapeBoneChainData* CurrentBoneNames = BoneChains.Find(CurrentBoneChainName);

		for (FName CurrentBoneName : CurrentBoneNames->BoneNamesRootToTip)
		{
			FVector SocketLocation = SkeletalMesh->GetSocketLocation(CurrentBoneName);

			PrevFramePositions.Add(SocketLocation);
		}
	}
}