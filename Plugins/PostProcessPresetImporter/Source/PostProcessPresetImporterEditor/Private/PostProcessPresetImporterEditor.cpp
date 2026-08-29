#include "PostProcessPresetImporterEditor.h"

#include "SPPPresetImporterWindow.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FPostProcessPresetImporterEditorModule"

const FName FPostProcessPresetImporterEditorModule::ImporterTabName("PostProcessPresetImporter");

void FPostProcessPresetImporterEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		ImporterTabName,
		FOnSpawnTab::CreateRaw(this, &FPostProcessPresetImporterEditorModule::SpawnImporterTab))
		.SetDisplayName(LOCTEXT("TabTitle", "Post Process Preset Importer"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// ToolMenus는 아직 준비되지 않았을 수 있으므로 콜백으로 등록한다.
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPostProcessPresetImporterEditorModule::RegisterMenus));
}

void FPostProcessPresetImporterEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ImporterTabName);
}

void FPostProcessPresetImporterEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	if (!ToolsMenu)
	{
		return;
	}

	FToolMenuSection& Section = ToolsMenu->FindOrAddSection("PostProcessTools", LOCTEXT("SectionLabel", "Post Process"));
	Section.AddMenuEntry(
		"OpenPostProcessPresetImporter",
		LOCTEXT("MenuLabel", "Post Process Preset Importer"),
		LOCTEXT("MenuTooltip", "JSON 프리셋을 레벨의 PostProcessVolume에 적용합니다."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FPostProcessPresetImporterEditorModule::ImporterTabName);
		})));
}

TSharedRef<SDockTab> FPostProcessPresetImporterEditorModule::SpawnImporterTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SPPPresetImporterWindow)
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPostProcessPresetImporterEditorModule, PostProcessPresetImporterEditor)
