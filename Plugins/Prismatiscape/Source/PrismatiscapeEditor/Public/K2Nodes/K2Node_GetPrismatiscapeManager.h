// © 2024, PrismaticaDev. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_CallFunction.h"
#include "K2Node_GetPrismatiscapeManager.generated.h"


namespace K2_PrismatiscapeHelpers
{
	struct FNodeWrapperHelper
	{
		virtual ~FNodeWrapperHelper() = default;
		FNodeWrapperHelper(){};

	protected:
		UK2Node* Node = nullptr;
	
	public:
		template<typename T = FNodeWrapperHelper>
		static T Create(UK2Node* SourceNode, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
		{
			T Node;
			Node.CreateNode(SourceNode, CompilerContext, SourceGraph);
			return Node;
		}

		virtual UEdGraphPin* GetExecPin() const;
		virtual UEdGraphPin* GetThenPin() const;
	
	protected:
		virtual void CreateNode(UK2Node* SourceNode, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph){}
	};
	
	struct FCallFunctionNodeWrapperHelper : FNodeWrapperHelper
	{
	public:
		FName FunctionName;
		UK2Node_CallFunction* CallFunctionNode = nullptr;

		UEdGraphPin* GetReturnPin() const;

		UEdGraphPin* GetTargetPin() const;

		UEdGraphPin* GetPinByName(const FName PinName) const;

		static FCallFunctionNodeWrapperHelper Create(UK2Node* SourceNode, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, const FName FunctionName);

		virtual void CreateNode(UK2Node* SourceNode, FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	};
}

/**
 * 
 */
UCLASS()
class PRISMATISCAPEEDITOR_API UK2Node_GetPrismatiscapeManager : public UK2Node
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;

	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetMenuCategory() const override;

	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
};