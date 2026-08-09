#include "Validation/ACDecayPairValidator.h"

#include "ACGameplayTags.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "DataAssets/Startup/ACDataAsset_StartupDataBase.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "UObject/UnrealType.h"

namespace ACDecayPairValidatorInternal
{
	const FName StartUpGameplayEffectsPropertyName(TEXT("StartUpGameplayEffects"));

	/** 초기화 GE는 레벨 1로 적용된다 */
	constexpr float InitializationLevel = 1.f;

	/** 한 게이지(Posture 또는 Guard)의 감소·유예 배선을 정의하는 값들 */
	struct FACDecayGaugeSpec
	{
		FString GaugeName;
		FGameplayAttribute GaugeAttribute;
		FGameplayTag BlockedTag;
		FName DelayEffectPropertyName;
	};

	void ReadEffectClasses(const UACDataAsset_StartupDataBase* StartupData, const FName PropertyName, TArray<UClass*>& OutClasses)
	{
		const FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(UACDataAsset_StartupDataBase::StaticClass(), PropertyName);
		if (!ArrayProperty)
		{
			return;
		}

		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(StartupData));
		for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
		{
			const TSubclassOf<UGameplayEffect>* EffectClass = reinterpret_cast<const TSubclassOf<UGameplayEffect>*>(ArrayHelper.GetRawPtr(Index));
			if (EffectClass && *EffectClass)
			{
				OutClasses.Add(*EffectClass);
			}
		}
	}

	/** *DecayDelayEffectClass는 protected라 리플렉션으로 읽는다 */
	UClass* ReadDelayEffectClass(const UACDataAsset_StartupDataBase* StartupData, const FName PropertyName)
	{
		if (const FClassProperty* ClassProperty = FindFProperty<FClassProperty>(UACDataAsset_StartupDataBase::StaticClass(), PropertyName))
		{
			return Cast<UClass>(ClassProperty->GetObjectPropertyValue_InContainer(StartupData));
		}
		return nullptr;
	}

	/**
	 * StackingType은 5.7에서 deprecated이고 GetStackingType()은 UE_API가 없어 모듈 밖에서 링크되지 않는다.
	 * UPROPERTY라 리플렉션으로는 안전하게 읽을 수 있고, 멤버가 private으로 바뀌어도 계속 동작한다.
	 */
	EGameplayEffectStackingType ReadStackingType(const UGameplayEffect* Effect)
	{
		static const FName StackingTypePropertyName(TEXT("StackingType"));

		if (const FEnumProperty* EnumProperty = FindFProperty<FEnumProperty>(UGameplayEffect::StaticClass(), StackingTypePropertyName))
		{
			const void* ValuePtr = EnumProperty->ContainerPtrToValuePtr<void>(Effect);
			return static_cast<EGameplayEffectStackingType>(EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr));
		}

		return EGameplayEffectStackingType::None;
	}

	bool HasModifierOnAttribute(const UGameplayEffect* Effect, const FGameplayAttribute& Attribute)
	{
		for (const FGameplayModifierInfo& Modifier : Effect->Modifiers)
		{
			if (Modifier.Attribute == Attribute)
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * 자연 감소 GE를 찾는다. 태그 배선이 잘못돼 있어도 찾아낼 수 있도록,
	 * 태그가 아니라 "무한 지속 + 주기형 + 해당 게이지 수정" 이라는 형태로 판별한다.
	 */
	const UGameplayEffect* FindDecayEffect(const TArray<UClass*>& EffectClasses, const FGameplayAttribute& GaugeAttribute)
	{
		for (UClass* EffectClass : EffectClasses)
		{
			const UGameplayEffect* EffectCDO = EffectClass->GetDefaultObject<UGameplayEffect>();
			if (!EffectCDO || EffectCDO->DurationPolicy != EGameplayEffectDurationType::Infinite)
			{
				continue;
			}

			if (EffectCDO->Period.GetValueAtLevel(InitializationLevel) <= 0.f)
			{
				continue;
			}

			if (HasModifierOnAttribute(EffectCDO, GaugeAttribute))
			{
				return EffectCDO;
			}
		}
		return nullptr;
	}

	bool DecayEffectIgnoresTag(const UGameplayEffect* DecayEffect, const FGameplayTag& BlockedTag)
	{
		const UTargetTagRequirementsGameplayEffectComponent* Requirements = DecayEffect->FindComponent<UTargetTagRequirementsGameplayEffectComponent>();
		if (!Requirements)
		{
			return false;
		}

		if (Requirements->OngoingTagRequirements.IgnoreTags.HasTagExact(BlockedTag))
		{
			return true;
		}

		// 태그 쿼리로 표현했을 수도 있다. 정확한 해석은 어려우므로 쿼리가 있으면 통과시킨다
		return !Requirements->OngoingTagRequirements.TagQuery.IsEmpty();
	}

	void AddIssue(TArray<FACDecayIssue>& OutIssues, const EACDecayIssueSeverity Severity, const FString& CharacterName, const FString& GaugeName, const FString& Message, const FSoftObjectPath& AssetPath)
	{
		FACDecayIssue& Issue = OutIssues.AddDefaulted_GetRef();
		Issue.Severity = Severity;
		Issue.CharacterName = CharacterName;
		Issue.GaugeName = GaugeName;
		Issue.Message = Message;
		Issue.AssetPath = AssetPath;
	}

	void ValidateGauge(
		const UACDataAsset_StartupDataBase* StartupData,
		const FString& CharacterName,
		const FSoftObjectPath& StartupDataPath,
		const TArray<UClass*>& EffectClasses,
		const FACDecayGaugeSpec& Spec,
		TArray<FACDecayIssue>& OutIssues)
	{
		UClass* DelayEffectClass = ReadDelayEffectClass(StartupData, Spec.DelayEffectPropertyName);
		const UGameplayEffect* DecayEffect = FindDecayEffect(EffectClasses, Spec.GaugeAttribute);

		// 이 캐릭터는 해당 게이지 시스템을 아예 쓰지 않는다
		if (!DelayEffectClass && !DecayEffect)
		{
			return;
		}

		if (!DecayEffect)
		{
			AddIssue(
				OutIssues,
				EACDecayIssueSeverity::Error,
				CharacterName,
				Spec.GaugeName,
				FString::Printf(TEXT("유예 GE(%s)는 지정됐는데 StartUpGameplayEffects에 자연 감소 GE가 없습니다. 게이지가 한 번 차면 줄어들지 않습니다."), *DelayEffectClass->GetName()),
				StartupDataPath);
			return;
		}

		const FSoftObjectPath DecayEffectPath(DecayEffect->GetClass());

		if (!DelayEffectClass)
		{
			AddIssue(
				OutIssues,
				EACDecayIssueSeverity::Error,
				CharacterName,
				Spec.GaugeName,
				FString::Printf(TEXT("자연 감소 GE(%s)는 있는데 %s가 비어 있습니다. 맞는 도중에도 게이지가 계속 줄어듭니다."), *DecayEffect->GetClass()->GetName(), *Spec.DelayEffectPropertyName.ToString()),
				StartupDataPath);
			return;
		}

		if (!DecayEffectIgnoresTag(DecayEffect, Spec.BlockedTag))
		{
			AddIssue(
				OutIssues,
				EACDecayIssueSeverity::Error,
				CharacterName,
				Spec.GaugeName,
				FString::Printf(TEXT("자연 감소 GE(%s)의 Ongoing Tag Requirements에 %s가 없습니다. 유예 GE를 적용해도 감소가 멈추지 않습니다."), *DecayEffect->GetClass()->GetName(), *Spec.BlockedTag.ToString()),
				DecayEffectPath);
		}

		const UGameplayEffect* DelayEffect = DelayEffectClass->GetDefaultObject<UGameplayEffect>();
		if (!DelayEffect)
		{
			return;
		}

		const FSoftObjectPath DelayEffectPath(DelayEffectClass);

		if (!DelayEffect->GetGrantedTags().HasTagExact(Spec.BlockedTag))
		{
			AddIssue(
				OutIssues,
				EACDecayIssueSeverity::Error,
				CharacterName,
				Spec.GaugeName,
				FString::Printf(TEXT("유예 GE(%s)가 %s를 부여하지 않습니다. 유예가 전혀 걸리지 않습니다."), *DelayEffectClass->GetName(), *Spec.BlockedTag.ToString()),
				DelayEffectPath);
		}

		if (DelayEffect->DurationPolicy != EGameplayEffectDurationType::HasDuration)
		{
			const bool bInfinite = DelayEffect->DurationPolicy == EGameplayEffectDurationType::Infinite;
			AddIssue(
				OutIssues,
				EACDecayIssueSeverity::Error,
				CharacterName,
				Spec.GaugeName,
				FString::Printf(
					TEXT("유예 GE(%s)의 Duration Policy가 Has Duration이 아닙니다. %s"),
					*DelayEffectClass->GetName(),
					bInfinite ? TEXT("Infinite라 한 번 걸리면 감소가 영영 재개되지 않습니다.") : TEXT("Instant라 태그가 즉시 사라져 유예가 없습니다.")),
				DelayEffectPath);
		}

		// 스택을 쓰지 않으면 재적용마다 새 인스턴스가 쌓여 결과적으로 유예가 연장되므로 문제가 없다
		if (ReadStackingType(DelayEffect) != EGameplayEffectStackingType::None
			&& DelayEffect->StackDurationRefreshPolicy != EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication)
		{
			AddIssue(
				OutIssues,
				EACDecayIssueSeverity::Warning,
				CharacterName,
				Spec.GaugeName,
				FString::Printf(TEXT("유예 GE(%s)의 Stack Duration Refresh Policy가 Refresh On Successful Application이 아닙니다. 연타해도 유예가 갱신되지 않고 첫 피격 기준으로만 동작합니다."), *DelayEffectClass->GetName()),
				DelayEffectPath);
		}
	}
}

TArray<FACDecayIssue> ACDecayPairValidator::ValidateAll()
{
	using namespace ACDecayPairValidatorInternal;

	static const TArray<FACDecayGaugeSpec> GaugeSpecs = {
		{ TEXT("Posture"), UACAttributeSet::GetPostureAttribute(), ACGameplayTags::Shared_Status_PostureDecayBlocked, FName(TEXT("PostureDecayDelayEffectClass")) },
		{ TEXT("Guard"), UACAttributeSet::GetGuardGaugeAttribute(), ACGameplayTags::Shared_Status_GuardDecayBlocked, FName(TEXT("GuardDecayDelayEffectClass")) }
	};

	FARFilter Filter;
	Filter.ClassPaths.Add(UACDataAsset_StartupDataBase::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	TArray<FAssetData> StartupAssets;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().GetAssets(Filter, StartupAssets);

	StartupAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.AssetName.LexicalLess(Right.AssetName);
	});

	TArray<FACDecayIssue> Issues;
	for (const FAssetData& AssetData : StartupAssets)
	{
		const UACDataAsset_StartupDataBase* StartupData = Cast<UACDataAsset_StartupDataBase>(AssetData.GetAsset());
		if (!StartupData)
		{
			continue;
		}

		TArray<UClass*> EffectClasses;
		ReadEffectClasses(StartupData, StartUpGameplayEffectsPropertyName, EffectClasses);

		for (const FACDecayGaugeSpec& Spec : GaugeSpecs)
		{
			ValidateGauge(StartupData, AssetData.AssetName.ToString(), AssetData.GetSoftObjectPath(), EffectClasses, Spec, Issues);
		}
	}

	return Issues;
}
