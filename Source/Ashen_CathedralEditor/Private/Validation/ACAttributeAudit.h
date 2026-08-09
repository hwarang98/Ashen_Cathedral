// Startup DataAsset이 적용하는 초기화 GE를 역추적해 캐릭터별 어트리뷰트 초기값과 그 출처를 수집하는 에디터 전용 감사기

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "UObject/SoftObjectPath.h"

class UCurveTable;

/** 어트리뷰트 초기값이 어디서 왔는지 */
enum class EACAttributeSource : uint8
{
	/** 초기화 GE가 건드리지 않아 AttributeSet 생성자 기본값이 그대로 남았다 */
	NotSet,

	/** GE의 ScalableFloat 상수값 */
	Constant,

	/** CurveTable 행에서 평가된 값 */
	CurveTable,

	/** AttributeBased / SetByCaller / CustomCalculationClass — 적용 시점에 결정된다 */
	Dynamic
};

/** 캐릭터 하나의 어트리뷰트 하나에 대한 초기값 정보 */
struct FACAttributeEntry
{
	FGameplayAttribute Attribute;
	float Value = 0.f;
	EACAttributeSource Source = EACAttributeSource::NotSet;

	/** Source가 CurveTable일 때만 유효하다. 이 두 값이 있으면 인라인 편집이 가능하다 */
	TWeakObjectPtr<UCurveTable> CurveTable;
	FName CurveRowName;

	/** 이 값을 설정한 GameplayEffect */
	FSoftObjectPath SourceEffectPath;
	FString SourceEffectName;
};

/** Startup DataAsset 하나로 정의되는 캐릭터의 어트리뷰트 프로필 */
struct FACAttributeProfile
{
	FString CharacterName;
	FSoftObjectPath StartupDataPath;

	/** GetTrackedAttributes()와 같은 순서로 채워진다 */
	TArray<FACAttributeEntry> Entries;

	/** 이 캐릭터의 초기화 GE들이 참조하는 CurveTable 전체. 2개 이상이면 복제 잔재를 의심한다 */
	TArray<TWeakObjectPtr<UCurveTable>> ReferencedCurveTables;
};

namespace ACAttributeAudit
{
	/** 매트릭스에 표시할 스탯 어트리뷰트 목록. 메타 어트리뷰트와 현재값(Health 등)은 제외한다 */
	const TArray<FGameplayAttribute>& GetTrackedAttributes();

	/** PIE 중 직접 편집 대상이 되는 현재값 어트리뷰트 목록 */
	const TArray<FGameplayAttribute>& GetRuntimeGaugeAttributes();

	/** 피해 주입에 사용하는 메타 어트리뷰트 목록 */
	const TArray<FGameplayAttribute>& GetMetaAttributes();

	/**
	 * @brief /Game 아래 모든 Startup DataAsset을 찾아 캐릭터별 어트리뷰트 프로필을 만든다.
	 *
	 * @return 캐릭터 이름 순으로 정렬된 프로필. 초기화 GE가 없는 캐릭터도 포함된다.
	 * @note 초기화 GE의 CDO를 읽어야 하므로 호출 시 에셋 로드가 발생한다.
	 */
	TArray<FACAttributeProfile> CollectAllProfiles();

	/**
	 * @brief CurveTable 행의 지정 레벨 키를 새 값으로 바꾸고 에셋을 더티 처리한다.
	 *
	 * @param Level 커브의 시간축 값. 어트리뷰트 초기화는 레벨 1을 쓴다
	 * @return 행을 찾아 실제로 수정했으면 true
	 * @note CSV에서 임포트한 CurveTable이면 재임포트 시 덮어써진다.
	 */
	bool SetCurveValue(UCurveTable* Table, FName RowName, float Level, float NewValue);
}
