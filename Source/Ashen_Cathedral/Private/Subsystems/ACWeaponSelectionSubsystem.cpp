// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/ACWeaponSelectionSubsystem.h"
#include "DataAssets/Items/Weapon/ACDataAsset_WeaponData.h"

void UACWeaponSelectionSubsystem::SetSelectedWeaponData(UACDataAsset_WeaponData* InWeaponData)
{
	if (SelectedWeaponData == InWeaponData)
	{
		return;
	}

	UACDataAsset_WeaponData* OldWeaponData = SelectedWeaponData;
	SelectedWeaponData = InWeaponData;

	OnSelectedWeaponChangedDelegate.Broadcast(OldWeaponData, SelectedWeaponData);
}

void UACWeaponSelectionSubsystem::ClearSelectedWeaponData()
{
	SetSelectedWeaponData(nullptr);
}

void UACWeaponSelectionSubsystem::SetWeaponChangeInProgress(bool bInProgress)
{
	bWeaponChangeInProgress = bInProgress;
}

bool UACWeaponSelectionSubsystem::CanChangeWeaponSelection() const
{
	return !bWeaponChangeInProgress;
}
