// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstance/Player/ACPlayerLinkedAnimLayer.h"
#include "AnimInstance/Player/ACPlayerAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

UACPlayerAnimInstance* UACPlayerLinkedAnimLayer::GetPlayerAnimInstance() const
{
	return Cast<UACPlayerAnimInstance>(GetOwningComponent()->GetAnimInstance());
}
