// 인벤토리 아이템 원형(불변 데이터)을 정의하는 PrimaryDataAsset

#include "DataAssets/Items/ACItemDefinition.h"

const FPrimaryAssetType UACItemDefinition::ItemDefinitionAssetType = TEXT("ACItemDefinition");

FPrimaryAssetId UACItemDefinition::GetPrimaryAssetId() const
{
	// CDO와 아키타입은 에셋이 아니다. 여기서 ID를 만들어주면 세이브/검증에 "Default__ACItemDefinition" 유령 항목이 섞인다
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return FPrimaryAssetId();
	}

	return FPrimaryAssetId(ItemDefinitionAssetType, GetFName());
}

int32 UACItemDefinition::ClampStackCount(int32 DesiredCount) const
{
	return FMath::Clamp(DesiredCount, 0, GetEffectiveMaxStackCount());
}
