// RunDefinition / StageDefinition 배선 검사 — 잘못 지정된 레벨이나 스테이지 순서를 런타임 이전에 잡는다

#pragma once

#include "CoreMinimal.h"
#include "Validation/ACAssetIssue.h"

namespace ACRunAssetValidator
{
	/**
	 * @brief 프로젝트의 모든 UACDataAsset_RunDefinition을 검사한다.
	 *
	 * @return RunDefinition 하나당 리포트 하나. 이름 순으로 정렬된다
	 * @note 레벨 에셋이 실제로 존재하는지까지 확인한다 — OpenLevelBySoftObjectPtr에는 null 가드가 없어
	 *       삭제되거나 이름이 바뀐 맵을 런타임에야 발견하게 되기 때문이다.
	 */
	TArray<FACAssetReport> ValidateAll();
}
