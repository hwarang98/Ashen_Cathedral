// 에디터 전용 모듈. 비활성화해도 런타임 게임 코드에 영향이 없다.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FPostProcessPresetImporterEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** 임포터 탭 ID. */
	static const FName ImporterTabName;

private:
	void RegisterMenus();
	TSharedRef<class SDockTab> SpawnImporterTab(const class FSpawnTabArgs& Args);
};
