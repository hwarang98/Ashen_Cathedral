// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/ACPlayerController.h"

AACPlayerController::AACPlayerController()
{
	PlayerTeamID = FGenericTeamId(0);
}

FGenericTeamId AACPlayerController::GetGenericTeamId() const
{
	return PlayerTeamID;
}