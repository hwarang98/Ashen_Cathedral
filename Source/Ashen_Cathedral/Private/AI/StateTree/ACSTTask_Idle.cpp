// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/StateTree/ACSTTask_Idle.h"

FACSTTask_Idle::FACSTTask_Idle()
{
	// 대기만 하므로 틱과 프로퍼티 복사를 모두 끈다. 상태 유지는 태스크 존재 자체로 충분하다
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
}