#include "Validation/ACStateTreeValidator.h"

#include "AI/StateTree/ACSTCondition_HasGameplayTag.h"
#include "AI/StateTree/ACSTTask_ActivateAbilityByTag.h"
#include "AI/StateTree/ACSTTask_SendGameplayEvent.h"
#include "Abilities/GameplayAbility.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "GameplayAbilitySystem/Abilities/ACGameplayAbility.h"
#include "StateTree.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

namespace ACStateTreeValidatorInternal
{
	/** GameplayAbility CDO 하나에서 뽑아낸 태그 요약. StateTree가 참조하는 태그의 실제 수신자를 찾는 데 쓴다 */
	struct FACAbilityInfo
	{
		FString Name;
		FGameplayTagContainer AssetTags;
		FGameplayTagContainer TriggerTags;
		FGameplayTagContainer ActivationOwnedTags;
		FGameplayTagContainer CooldownTags;
	};

	const FName EventTagPropertyName(TEXT("EventTag"));
	const FName WaitOwnedTagPropertyName(TEXT("WaitOwnedTag"));
	const FName AbilityTagToActivatePropertyName(TEXT("AbilityTagToActivate"));
	const FName TagPropertyName(TEXT("Tag"));
	const FName InvertPropertyName(TEXT("bInvert"));
	const FName AbilityTriggersPropertyName(TEXT("AbilityTriggers"));
	const FName ActivationOwnedTagsPropertyName(TEXT("ActivationOwnedTags"));

	/** private UPROPERTY도 읽어야 하므로 멤버 접근 대신 리플렉션으로 값을 꺼낸다 */
	FGameplayTag ReadTag(const UStruct* Struct, const void* Container, const FName PropertyName)
	{
		if (const FStructProperty* Property = FindFProperty<FStructProperty>(Struct, PropertyName))
		{
			if (Property->Struct == FGameplayTag::StaticStruct())
			{
				return *Property->ContainerPtrToValuePtr<FGameplayTag>(Container);
			}
		}
		return FGameplayTag();
	}

	FGameplayTagContainer ReadTagContainer(const UStruct* Struct, const void* Container, const FName PropertyName)
	{
		if (const FStructProperty* Property = FindFProperty<FStructProperty>(Struct, PropertyName))
		{
			if (Property->Struct == FGameplayTagContainer::StaticStruct())
			{
				return *Property->ContainerPtrToValuePtr<FGameplayTagContainer>(Container);
			}
		}
		return FGameplayTagContainer();
	}

	bool ReadBool(const UStruct* Struct, const void* Container, const FName PropertyName)
	{
		if (const FBoolProperty* Property = FindFProperty<FBoolProperty>(Struct, PropertyName))
		{
			return Property->GetPropertyValue_InContainer(Container);
		}
		return false;
	}

	/** AbilityTriggers는 protected라 배열 프로퍼티를 리플렉션으로 순회한다 */
	void ReadTriggerTags(UGameplayAbility* Ability, FGameplayTagContainer& OutTags)
	{
		const FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(UGameplayAbility::StaticClass(), AbilityTriggersPropertyName);
		if (!ArrayProperty)
		{
			return;
		}

		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Ability));
		for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
		{
			const FAbilityTriggerData* TriggerData = reinterpret_cast<const FAbilityTriggerData*>(ArrayHelper.GetRawPtr(Index));
			if (TriggerData && TriggerData->TriggerTag.IsValid())
			{
				OutTags.AddTag(TriggerData->TriggerTag);
			}
		}
	}

	FACAbilityInfo MakeAbilityInfo(UClass* AbilityClass)
	{
		FACAbilityInfo Info;
		Info.Name = AbilityClass->GetName();

		UGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UGameplayAbility>();
		if (!AbilityCDO)
		{
			return Info;
		}

		Info.AssetTags = AbilityCDO->GetAssetTags();
		ReadTriggerTags(AbilityCDO, Info.TriggerTags);
		Info.ActivationOwnedTags = ReadTagContainer(UGameplayAbility::StaticClass(), AbilityCDO, ActivationOwnedTagsPropertyName);

		if (const FGameplayTagContainer* CooldownTags = AbilityCDO->GetCooldownTags())
		{
			Info.CooldownTags = *CooldownTags;
		}

		return Info;
	}

	/** 네이티브 + 블루프린트 어빌리티를 모아 태그 색인을 만든다. BP는 네이티브 부모 태그로 먼저 걸러 필요한 것만 로드한다 */
	TArray<FACAbilityInfo> BuildAbilityInfos()
	{
		TArray<UClass*> AbilityClasses;

		for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
		{
			UClass* Class = *ClassIterator;
			if (!Class->IsChildOf(UACGameplayAbility::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_NewerVersionExists | CLASS_Deprecated))
			{
				continue;
			}

			// 블루프린트 컴파일 부산물은 원본과 같은 태그를 갖고 있어 색인에 중복으로 들어간다
			const FString ClassName = Class->GetName();
			if (ClassName.StartsWith(TEXT("SKEL_")) || ClassName.StartsWith(TEXT("REINST_")))
			{
				continue;
			}

			AbilityClasses.AddUnique(Class);
		}

		FARFilter Filter;
		Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
		Filter.PackagePaths.Add(TEXT("/Game"));
		Filter.bRecursiveClasses = true;
		Filter.bRecursivePaths = true;

		TArray<FAssetData> BlueprintAssets;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().GetAssets(Filter, BlueprintAssets);

		for (const FAssetData& AssetData : BlueprintAssets)
		{
			const UClass* NativeParent = UBlueprint::GetBlueprintParentClassFromAssetTags(AssetData);
			if (!NativeParent || !NativeParent->IsChildOf(UACGameplayAbility::StaticClass()))
			{
				continue;
			}

			if (const UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
			{
				if (Blueprint->GeneratedClass)
				{
					AbilityClasses.AddUnique(Blueprint->GeneratedClass);
				}
			}
		}

		TArray<FACAbilityInfo> Infos;
		Infos.Reserve(AbilityClasses.Num());
		for (UClass* AbilityClass : AbilityClasses)
		{
			Infos.Add(MakeAbilityInfo(AbilityClass));
		}
		return Infos;
	}

	/** AbilityTagToActivate를 전부 가진 어빌리티를 찾는다. FACSTTask_ActivateAbilityByTag::FindAbilityHandle과 같은 규칙이다 */
	const FACAbilityInfo* FindAbilityByAssetTags(const TArray<FACAbilityInfo>& Infos, const FGameplayTagContainer& RequiredTags)
	{
		if (RequiredTags.IsEmpty())
		{
			return nullptr;
		}

		for (const FACAbilityInfo& Info : Infos)
		{
			if (Info.AssetTags.HasAll(RequiredTags))
			{
				return &Info;
			}
		}
		return nullptr;
	}

	const FACAbilityInfo* FindAbilityByTriggerTag(const TArray<FACAbilityInfo>& Infos, const FGameplayTag& TriggerTag)
	{
		for (const FACAbilityInfo& Info : Infos)
		{
			if (Info.TriggerTags.HasTagExact(TriggerTag))
			{
				return &Info;
			}
		}
		return nullptr;
	}

	bool IsCooldownTagGranted(const TArray<FACAbilityInfo>& Infos, const FGameplayTag& CooldownTag)
	{
		for (const FACAbilityInfo& Info : Infos)
		{
			if (Info.CooldownTags.HasTagExact(CooldownTag))
			{
				return true;
			}
		}
		return false;
	}

	bool IsUtilityWeightedSelection(const EStateTreeStateSelectionBehavior Behavior)
	{
		return Behavior == EStateTreeStateSelectionBehavior::TrySelectChildrenAtRandomWeightedByUtility;
	}

	bool IsUtilityHighestSelection(const EStateTreeStateSelectionBehavior Behavior)
	{
		return Behavior == EStateTreeStateSelectionBehavior::TrySelectChildrenWithHighestUtility;
	}

	void AddIssue(FACStateTreeReport& Report, const EACStateTreeIssueSeverity Severity, const FString& StatePath, const FString& Message)
	{
		FACStateTreeIssue& Issue = Report.Issues.AddDefaulted_GetRef();
		Issue.Severity = Severity;
		Issue.StatePath = StatePath;
		Issue.Message = Message;
	}

	/** 상태의 태스크 목록(Tasks + SingleTask)에서 유효한 노드만 모은다 */
	void CollectTaskNodes(const UStateTreeState& State, TArray<const FStateTreeEditorNode*>& OutNodes)
	{
		for (const FStateTreeEditorNode& Node : State.Tasks)
		{
			if (Node.Node.IsValid())
			{
				OutNodes.Add(&Node);
			}
		}

		if (State.SingleTask.Node.IsValid())
		{
			OutNodes.Add(&State.SingleTask);
		}
	}

	/** 조건 노드에서 Enemy.Cooldown.* 태그를 검사한다. 부여하는 어빌리티가 없으면 조건이 상수로 굳는다 */
	void ValidateConditionNode(const FStateTreeEditorNode& Node, const TArray<FACAbilityInfo>& Infos, const FString& StatePath, const FString& Context, FACStateTreeReport& Report)
	{
		if (Node.Node.GetScriptStruct() != FACSTCondition_HasGameplayTag::StaticStruct())
		{
			return;
		}

		const UScriptStruct* NodeStruct = Node.Node.GetScriptStruct();
		const void* NodeMemory = Node.Node.GetMemory();

		const FGameplayTag Tag = ReadTag(NodeStruct, NodeMemory, TagPropertyName);
		if (!Tag.IsValid() || !Tag.ToString().StartsWith(TEXT("Enemy.Cooldown.")))
		{
			return;
		}

		if (IsCooldownTagGranted(Infos, Tag))
		{
			return;
		}

		const bool bInvert = ReadBool(NodeStruct, NodeMemory, InvertPropertyName);
		AddIssue(
			Report,
			EACStateTreeIssueSeverity::Warning,
			StatePath,
			FString::Printf(
				TEXT("%s: 쿨다운 태그 %s를 CooldownIdentifierTags로 부여하는 어빌리티가 없습니다. 조건이 항상 %s로 고정됩니다."),
				*Context,
				*Tag.ToString(),
				bInvert ? TEXT("통과") : TEXT("차단")));
	}

	void ValidateTaskNode(const FStateTreeEditorNode& Node, const TArray<FACAbilityInfo>& Infos, const FString& StatePath, FACStateTreeReport& Report)
	{
		const UScriptStruct* NodeStruct = Node.Node.GetScriptStruct();
		const void* NodeMemory = Node.Node.GetMemory();

		if (NodeStruct == FACSTTask_SendGameplayEvent::StaticStruct())
		{
			const FGameplayTag EventTag = ReadTag(NodeStruct, NodeMemory, EventTagPropertyName);
			if (!EventTag.IsValid())
			{
				AddIssue(Report, EACStateTreeIssueSeverity::Error, StatePath, TEXT("Send Gameplay Event: EventTag가 비어 있어 태스크가 즉시 Failed로 끝납니다."));
				return;
			}

			const FACAbilityInfo* Receiver = FindAbilityByTriggerTag(Infos, EventTag);
			if (!Receiver)
			{
				AddIssue(
					Report,
					EACStateTreeIssueSeverity::Warning,
					StatePath,
					FString::Printf(TEXT("Send Gameplay Event: %s를 AbilityTriggers로 받는 어빌리티가 없습니다. 어빌리티가 아닌 곳에서 받는다면 정상입니다."), *EventTag.ToString()));
				return;
			}

			const FGameplayTag WaitOwnedTag = ReadTag(NodeStruct, NodeMemory, WaitOwnedTagPropertyName);
			if (!WaitOwnedTag.IsValid())
			{
				AddIssue(Report, EACStateTreeIssueSeverity::Warning, StatePath, TEXT("Send Gameplay Event: WaitOwnedTag가 없어 이벤트 발송 직후 태스크가 완료됩니다."));
			}
			else if (!Receiver->ActivationOwnedTags.HasTagExact(WaitOwnedTag))
			{
				AddIssue(
					Report,
					EACStateTreeIssueSeverity::Info,
					StatePath,
					FString::Printf(
						TEXT("Send Gameplay Event: WaitOwnedTag %s가 %s의 ActivationOwnedTags에 없습니다. GE나 AnimNotify로 부여한다면 정상입니다."),
						*WaitOwnedTag.ToString(),
						*Receiver->Name));
			}
			return;
		}

		if (NodeStruct == FACSTTask_ActivateAbilityByTag::StaticStruct())
		{
			const FGameplayTagContainer AbilityTags = ReadTagContainer(NodeStruct, NodeMemory, AbilityTagToActivatePropertyName);
			if (AbilityTags.IsEmpty())
			{
				AddIssue(Report, EACStateTreeIssueSeverity::Error, StatePath, TEXT("Activate Ability By Tag: AbilityTagToActivate가 비어 있어 어떤 어빌리티도 찾지 못합니다."));
				return;
			}

			const FACAbilityInfo* Target = FindAbilityByAssetTags(Infos, AbilityTags);
			if (!Target)
			{
				AddIssue(
					Report,
					EACStateTreeIssueSeverity::Error,
					StatePath,
					FString::Printf(TEXT("Activate Ability By Tag: AssetTags에 %s를 모두 가진 어빌리티가 없습니다. 태스크가 어빌리티를 찾지 못합니다."), *AbilityTags.ToStringSimple()));
				return;
			}

			const FGameplayTag WaitOwnedTag = ReadTag(NodeStruct, NodeMemory, WaitOwnedTagPropertyName);
			if (WaitOwnedTag.IsValid() && !Target->ActivationOwnedTags.HasTagExact(WaitOwnedTag))
			{
				AddIssue(
					Report,
					EACStateTreeIssueSeverity::Info,
					StatePath,
					FString::Printf(
						TEXT("Activate Ability By Tag: WaitOwnedTag %s가 %s의 ActivationOwnedTags에 없습니다. GE나 AnimNotify로 부여한다면 정상입니다."),
						*WaitOwnedTag.ToString(),
						*Target->Name));
			}
		}
	}

	/** 트리 전체에서 전이가 직접 겨냥하는 상태 ID를 모은다. 부모 선택 순서로는 도달 못해도 전이로 들어오면 정상이기 때문이다 */
	void CollectTransitionTargets(const UStateTreeState& State, TSet<FGuid>& OutTargets)
	{
		for (const FStateTreeTransition& Transition : State.Transitions)
		{
			if (Transition.State.LinkType == EStateTreeTransitionType::GotoState && Transition.State.ID.IsValid())
			{
				OutTargets.Add(Transition.State.ID);
			}
		}

		for (const UStateTreeState* Child : State.Children)
		{
			if (Child)
			{
				CollectTransitionTargets(*Child, OutTargets);
			}
		}
	}

	void ValidateState(const UStateTreeState& State, const FString& ParentPath, const TArray<FACAbilityInfo>& Infos, const TSet<FGuid>& TransitionTargets, FACStateTreeReport& Report)
	{
		if (!State.bEnabled)
		{
			return;
		}

		const FString StatePath = ParentPath.IsEmpty() ? State.Name.ToString() : FString::Printf(TEXT("%s > %s"), *ParentPath, *State.Name.ToString());
		++Report.StateCount;

		TArray<const FStateTreeEditorNode*> TaskNodes;
		CollectTaskNodes(State, TaskNodes);

		for (const FStateTreeEditorNode* TaskNode : TaskNodes)
		{
			ValidateTaskNode(*TaskNode, Infos, StatePath, Report);
		}

		for (const FStateTreeEditorNode& ConditionNode : State.EnterConditions)
		{
			if (ConditionNode.Node.IsValid())
			{
				ValidateConditionNode(ConditionNode, Infos, StatePath, TEXT("Enter Condition"), Report);
			}
		}

		for (const FStateTreeTransition& Transition : State.Transitions)
		{
			for (const FStateTreeEditorNode& ConditionNode : Transition.Conditions)
			{
				if (ConditionNode.Node.IsValid())
				{
					ValidateConditionNode(ConditionNode, Infos, StatePath, TEXT("Transition Condition"), Report);
				}
			}
		}

		const bool bIsLeaf = State.Children.IsEmpty();
		if (bIsLeaf)
		{
			if (TaskNodes.IsEmpty() && State.Transitions.IsEmpty() && State.Type == EStateTreeStateType::State)
			{
				AddIssue(Report, EACStateTreeIssueSeverity::Warning, StatePath, TEXT("태스크도 전이도 없는 말단 상태입니다. 진입하면 아무것도 실행하지 않고 빠져나갈 길도 없습니다."));
			}
			return;
		}

		// 자식 선택 규칙 검사 — 부모의 SelectionBehavior에 따라 자식이 실제로 선택될 수 있는지 본다
		int32 UnconditionalChildIndex = INDEX_NONE;
		for (int32 Index = 0; Index < State.Children.Num(); ++Index)
		{
			const UStateTreeState* Child = State.Children[Index];
			if (!Child || !Child->bEnabled)
			{
				continue;
			}

			const FString ChildPath = FString::Printf(TEXT("%s > %s"), *StatePath, *Child->Name.ToString());

			if (IsUtilityWeightedSelection(State.SelectionBehavior) && Child->Considerations.IsEmpty())
			{
				AddIssue(
					Report,
					EACStateTreeIssueSeverity::Error,
					ChildPath,
					FString::Printf(
						TEXT("부모가 Try Select Children At Random Weighted By Utility인데 Considerations가 없어 점수가 0이 됩니다. Weight %.1f는 무시되고 이 상태는 절대 선택되지 않습니다."),
						Child->Weight));
			}
			else if (IsUtilityHighestSelection(State.SelectionBehavior) && Child->Considerations.IsEmpty())
			{
				AddIssue(
					Report,
					EACStateTreeIssueSeverity::Warning,
					ChildPath,
					TEXT("부모가 Try Select Children With Highest Utility인데 Considerations가 없어 점수가 0입니다. 동점 처리로 항상 앞선 형제가 선택됩니다."));
			}

			if (State.SelectionBehavior == EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder
				&& UnconditionalChildIndex == INDEX_NONE
				&& Child->EnterConditions.IsEmpty()
				&& !Child->bHasRequiredEventToEnter
				&& Child->SelectionBehavior != EStateTreeStateSelectionBehavior::None)
			{
				UnconditionalChildIndex = Index;
			}
			else if (UnconditionalChildIndex != INDEX_NONE && !TransitionTargets.Contains(Child->ID))
			{
				AddIssue(
					Report,
					EACStateTreeIssueSeverity::Warning,
					ChildPath,
					FString::Printf(
						TEXT("순서 선택에서 앞선 형제 %s가 조건 없이 항상 통과하고, 이 상태를 겨냥하는 전이도 없어 도달할 수 없습니다."),
						*State.Children[UnconditionalChildIndex]->Name.ToString()));
			}
		}

		if (IsUtilityWeightedSelection(State.SelectionBehavior))
		{
			bool bAnyChildScorable = false;
			for (const UStateTreeState* Child : State.Children)
			{
				if (Child && Child->bEnabled && !Child->Considerations.IsEmpty())
				{
					bAnyChildScorable = true;
					break;
				}
			}

			if (!bAnyChildScorable)
			{
				AddIssue(
					Report,
					EACStateTreeIssueSeverity::Error,
					StatePath,
					TEXT("자식이 모두 Considerations가 없어 후보가 하나도 남지 않습니다. 이 상태로의 전이가 항상 실패합니다."));
			}
		}

		for (const UStateTreeState* Child : State.Children)
		{
			if (Child)
			{
				ValidateState(*Child, StatePath, Infos, TransitionTargets, Report);
			}
		}
	}
}

int32 FACStateTreeReport::CountBySeverity(EACStateTreeIssueSeverity Severity) const
{
	int32 Count = 0;
	for (const FACStateTreeIssue& Issue : Issues)
	{
		Count += Issue.Severity == Severity ? 1 : 0;
	}
	return Count;
}

TArray<FACStateTreeReport> ACStateTreeValidator::ValidateAllStateTrees()
{
	using namespace ACStateTreeValidatorInternal;

	FARFilter Filter;
	Filter.ClassPaths.Add(UStateTree::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	TArray<FAssetData> StateTreeAssets;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().GetAssets(Filter, StateTreeAssets);

	if (StateTreeAssets.IsEmpty())
	{
		return TArray<FACStateTreeReport>();
	}

	StateTreeAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.AssetName.LexicalLess(Right.AssetName);
	});

	const TArray<FACAbilityInfo> AbilityInfos = BuildAbilityInfos();

	TArray<FACStateTreeReport> Reports;
	Reports.Reserve(StateTreeAssets.Num());

	for (const FAssetData& AssetData : StateTreeAssets)
	{
		const UStateTree* StateTree = Cast<UStateTree>(AssetData.GetAsset());
		if (!StateTree)
		{
			continue;
		}

		FACStateTreeReport& Report = Reports.AddDefaulted_GetRef();
		Report.AssetPath = AssetData.GetSoftObjectPath();
		Report.AssetName = AssetData.AssetName.ToString();

		const UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
		if (!EditorData)
		{
			AddIssue(Report, EACStateTreeIssueSeverity::Error, FString(), TEXT("EditorData를 읽을 수 없습니다. 쿠킹된 에셋이거나 손상됐을 수 있습니다."));
			continue;
		}

		TSet<FGuid> TransitionTargets;
		for (const UStateTreeState* SubTree : EditorData->SubTrees)
		{
			if (SubTree)
			{
				CollectTransitionTargets(*SubTree, TransitionTargets);
			}
		}

		for (const UStateTreeState* SubTree : EditorData->SubTrees)
		{
			if (SubTree)
			{
				ValidateState(*SubTree, FString(), AbilityInfos, TransitionTargets, Report);
			}
		}
	}

	return Reports;
}
