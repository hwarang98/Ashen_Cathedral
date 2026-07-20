// 메타 성장 재화와 첫 클리어 보스 목록을 보관하는 SaveGame — 프로젝트 최초의 영속 저장 데이터

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "ACSaveGame_MetaProgression.generated.h"

UCLASS()
class ASHEN_CATHEDRAL_API UACSaveGame_MetaProgression : public USaveGame
{
	GENERATED_BODY()

public:
	// 재화 태그(MetaProgression.Currency.*)별 보유량. 항목이 없는 재화는 0으로 취급한다.
	UPROPERTY()
	TMap<FGameplayTag, int32> CurrencyAmounts;

	// 첫 클리어를 완료한 보스의 BossID 태그 모음 — 반복 클리어 보상 판정에 사용
	UPROPERTY()
	FGameplayTagContainer ClearedBossTags;

	// 세이브 데이터 구조 변경 시 마이그레이션 분기에 사용할 버전. 단일 재화 -> 3재화 전환으로 1이 되었다.
	UPROPERTY()
	int32 SaveVersion = 1;
};
