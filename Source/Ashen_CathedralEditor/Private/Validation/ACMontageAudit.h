// Enemy 공격 몽타주에 필요한 AnimNotify가 배치돼 있는지 검사하는 에디터 전용 감사기

#pragma once

#include "CoreMinimal.h"
#include "Validation/ACAssetIssue.h"

namespace ACMontageAudit
{
	/**
	 * @brief /Game/Enemy 아래 공격 몽타주를 찾아 필수 노티파이 배치를 검사한다.
	 *
	 * @return 몽타주 이름 순으로 정렬된 검사 결과. 문제가 없는 몽타주도 포함된다.
	 * @note 공격 몽타주 판별은 에셋 이름과 경로 규칙(Attack / Combo / Counter / "/Attack/")을 따른다.
	 *       규칙에 걸리지 않는 몽타주는 로드조차 하지 않는다.
	 */
	TArray<FACAssetReport> AuditAll();
}
