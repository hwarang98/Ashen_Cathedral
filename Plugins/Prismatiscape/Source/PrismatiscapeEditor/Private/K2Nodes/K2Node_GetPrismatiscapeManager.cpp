// © 2024, PrismaticaDev. All rights reserved.


#include "K2Nodes/K2Node_GetPrismatiscapeManager.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "KismetCompiler.h"
#include "EdGraphSchema_K2.h"
#include "PrismatiscapeWorldSubsystem.h"
#include "Manager/PrismatiscapeManager.h"


#define LOCTEXT_NAMESPACE "K2_GetPrismatiscapeManager"

namespace PinNames
{
	const FName WorldContextObject = "WorldContextObject";
	const FName ReturnValue = "ReturnValue";
}

UEdGraphPin* K2_PrismatiscapeHelpers::FNodeWrapperHelper::GetExecPin() const
{
	return Node->GetExecPin();
}
UEdGraphPin* K2_PrismatiscapeHelpers::FNodeWrapperHelper::GetThenPin() const
{
	return Node->GetThenPin();
}

UEdGraphPin* K2_PrismatiscapeHelpers::FCallFunctionNodeWrapperHelper::GetReturnPin() const
{
	return CallFunctionNode->GetReturnValuePin();
}

UEdGraphPin* K2_PrismatiscapeHelpers::FCallFunctionNodeWrapperHelper::GetTargetPin() const
{
	return CallFunctionNode->FindPinChecked(UEdGraphSchema_K2::PN_Self);
}

UEdGraphPin* K2_PrismatiscapeHelpers::FCallFunctionNodeWrapperHelper::GetPinByName(const FName PinName) const
{
	return CallFunctionNode->FindPinChecked(PinName);
}

K2_PrismatiscapeHelpers::FCallFunctionNodeWrapperHelper K2_PrismatiscapeHelpers::FCallFunctionNodeWrapperHelper::Create(UK2Node* SourceNode, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, const FName FunctionName)
{
	FCallFunctionNodeWrapperHelper Node;
	Node.FunctionName = FunctionName;
	Node.CreateNode(SourceNode, CompilerContext, SourceGraph);
	return Node;
}

void K2_PrismatiscapeHelpers::FCallFunctionNodeWrapperHelper::CreateNode(UK2Node* SourceNode, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Node = CallFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(SourceNode, SourceGraph);
	CallFunctionNode->FunctionReference.SetExternalMember(FunctionName, UPrismatiscapeWorldSubsystem::StaticClass());
	CallFunctionNode->AllocateDefaultPins();
}

/*******************************************************************************************************************************************************************************/

void UK2Node_GetPrismatiscapeManager::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, APrismatiscapeManager::StaticClass(), UEdGraphSchema_K2::PN_ReturnValue);
}

void UK2Node_GetPrismatiscapeManager::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static const FName FunctionName = GET_FUNCTION_NAME_CHECKED(UPrismatiscapeWorldSubsystem, GetPrismatiscapeManager);

	const K2_PrismatiscapeHelpers::FCallFunctionNodeWrapperHelper Node = K2_PrismatiscapeHelpers::FCallFunctionNodeWrapperHelper::Create(this, CompilerContext, SourceGraph, FunctionName);
	
	CompilerContext.MovePinLinksToIntermediate(*GetExecPin(), *Node.GetExecPin());
	CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *Node.GetThenPin());

	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue), *Node.GetReturnPin());
}

void UK2Node_GetPrismatiscapeManager::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);

	const UClass* Action = GetClass();
	
	if (ActionRegistrar.IsOpenForRegistration(Action))
	{
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
		check(Spawner != nullptr);

		ActionRegistrar.AddBlueprintAction(Action, Spawner);
	}
}

FText UK2Node_GetPrismatiscapeManager::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("UK2Node_GetPrismatiscapeManager_Title", "Get Prismatiscape Manager");
}

FText UK2Node_GetPrismatiscapeManager::GetMenuCategory() const
{
	return LOCTEXT("UK2Node_GetPrismatiscapeManager_Category", "Prismatiscape");
}

FSlateIcon UK2Node_GetPrismatiscapeManager::GetIconAndTint(FLinearColor& OutColor) const
{
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.FoliageType_Actor");
}

FLinearColor UK2Node_GetPrismatiscapeManager::GetNodeTitleColor() const
{
	return FLinearColor(0,7.5,10,1);
}

#undef LOCTEXT_NAMESPACE