// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveGame/ACSaveGame_MetaProgression.h"

bool UACSaveGame_MetaProgression::MigrateIfNeeded()
{
	if (SaveVersion >= CurrentSaveVersion)
	{
		// 미래 버전 파일을 이 빌드로 열면 모르는 필드는 조용히 사라진다. 덮어쓰기 전에 알 수 있도록 남긴다
		if (SaveVersion > CurrentSaveVersion)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ACSaveGame_MetaProgression] 세이브 버전 %d가 이 빌드가 아는 버전 %d보다 높습니다. 저장하면 알 수 없는 데이터가 유실될 수 있습니다."), SaveVersion, CurrentSaveVersion);
		}
		return false;
	}

	// 버전 1 -> 2: 인벤토리 필드가 추가되었을 뿐이라 옮겨올 데이터가 없다.
	// CurrencyAmounts와 ClearedBossTags는 그대로 두고 버전만 올린다.
	UE_LOG(LogTemp, Log, TEXT("[ACSaveGame_MetaProgression] 세이브 버전 %d -> %d 마이그레이션. 재화 %d종, 보스 기록 %d개를 유지합니다."), SaveVersion, CurrentSaveVersion, CurrencyAmounts.Num(), ClearedBossTags.Num());

	SaveVersion = CurrentSaveVersion;
	return true;
}
