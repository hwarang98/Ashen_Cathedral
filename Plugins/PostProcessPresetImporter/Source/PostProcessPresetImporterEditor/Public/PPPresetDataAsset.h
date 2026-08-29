// 임포트한 프리셋을 프로젝트 에셋으로 보존한다.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Scene.h"
#include "PPPresetDataAsset.generated.h"

/**
 * @brief 임포트된 포스트 프로세스 프리셋 한 건.
 * @note 에디터 모듈에 정의되어 있으므로 쿡되는 빌드에서는 로드되지 않는다. 저작 시점 자산이다.
 */
UCLASS(BlueprintType)
class POSTPROCESSPRESETIMPORTEREDITOR_API UPPPresetDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** JSON에 적힌 원본 프리셋 이름. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preset")
	FString SourcePresetName;

	/** JSON에 적힌 원본 패키지 경로. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preset")
	FString SourcePackagePath;

	/** 매핑된 설정 전체. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preset", meta = (ShowOnlyInnerProperties))
	FPostProcessSettings Settings;

	/** 이 프리셋이 켠 bOverride_* 플래그 이름 목록. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preset")
	TArray<FString> AppliedOverrides;

	/** 프로젝트에 없어서 연결하지 못한 에셋 경로. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preset")
	TArray<FString> MissingAssetPaths;

	/** 현재 엔진의 FPostProcessSettings에 없어서 건너뛴 프로퍼티. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preset")
	TArray<FString> UnknownProperties;
};
