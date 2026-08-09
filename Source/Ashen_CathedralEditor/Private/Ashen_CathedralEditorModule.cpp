#include "Details/ACBossTuningDetailsCustomization.h"
#include "DataAssets/AI/ACDataAsset_BossTuning.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Workbench/SACBossCombatWorkbench.h"

#define LOCTEXT_NAMESPACE "AshenCathedralEditorModule"

namespace AshenCathedralEditor
{
	const FName BossCombatWorkbenchTabName(TEXT("AshenCathedral.BossCombatWorkbench"));
}

class FAshenCathedralEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyEditorModule =
			FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

		PropertyEditorModule.RegisterCustomClassLayout(
			UACDataAsset_BossTuning::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FACBossTuningDetailsCustomization::MakeInstance));
		PropertyEditorModule.NotifyCustomizationModuleChanged();

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			AshenCathedralEditor::BossCombatWorkbenchTabName,
			FOnSpawnTab::CreateRaw(this, &FAshenCathedralEditorModule::SpawnBossCombatWorkbenchTab))
			.SetDisplayName(LOCTEXT("BossCombatWorkbenchTabTitle", "Boss Combat Workbench"))
			.SetTooltipText(LOCTEXT("BossCombatWorkbenchTabTooltip", "보스 전투 테스트, 튜닝, 검증 도구"))
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("LevelEditor.Tabs.Details")));

		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAshenCathedralEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AshenCathedralEditor::BossCombatWorkbenchTabName);

		if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
		{
			return;
		}

		FPropertyEditorModule& PropertyEditorModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyEditorModule.UnregisterCustomClassLayout(UACDataAsset_BossTuning::StaticClass()->GetFName());
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}

private:
	TSharedRef<SDockTab> SpawnBossCombatWorkbenchTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SACBossCombatWorkbench)
			];
	}

	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);
		UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
		FToolMenuSection& Section = ToolsMenu->FindOrAddSection(TEXT("AshenCathedral"));
		Section.AddMenuEntry(
			TEXT("OpenBossCombatWorkbench"),
			LOCTEXT("OpenBossCombatWorkbench", "Boss Combat Workbench"),
			LOCTEXT("OpenBossCombatWorkbenchTooltip", "Ashen Cathedral 보스 제작 및 디버그 도구를 엽니다."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("LevelEditor.Tabs.Details")),
			FUIAction(FExecuteAction::CreateLambda([]
			{
				FGlobalTabmanager::Get()->TryInvokeTab(AshenCathedralEditor::BossCombatWorkbenchTabName);
			})));
	}
};

IMPLEMENT_MODULE(FAshenCathedralEditorModule, Ashen_CathedralEditor)

#undef LOCTEXT_NAMESPACE
