#include "Validation/ACAttributeAudit.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Curves/RealCurve.h"
#include "DataAssets/Startup/ACDataAsset_StartupDataBase.h"
#include "Engine/CurveTable.h"
#include "GameplayAbilitySystem/ACAttributeSet.h"
#include "GameplayEffect.h"
#include "ScopedTransaction.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "ACAttributeAudit"

namespace ACAttributeAuditInternal
{
	const FName StartUpGameplayEffectsPropertyName(TEXT("StartUpGameplayEffects"));
	const FName MagnitudeCalculationTypePropertyName(TEXT("MagnitudeCalculationType"));
	const FName ScalableFloatMagnitudePropertyName(TEXT("ScalableFloatMagnitude"));

	/** 초기화 GE는 레벨 1로 적용된다 (GiveToAbilitySystemComponent의 ApplyLevel 기본값) */
	constexpr float InitializationLevel = 1.f;

	/** StartUpGameplayEffects는 protected라 리플렉션으로 읽는다 */
	void ReadStartupEffectClasses(const UACDataAsset_StartupDataBase* StartupData, TArray<UClass*>& OutClasses)
	{
		const FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(UACDataAsset_StartupDataBase::StaticClass(), StartUpGameplayEffectsPropertyName);
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

	/** MagnitudeCalculationType과 ScalableFloatMagnitude는 protected라 리플렉션으로 읽는다 */
	EGameplayEffectMagnitudeCalculation ReadCalculationType(const FGameplayEffectModifierMagnitude& Magnitude)
	{
		if (const FEnumProperty* EnumProperty = FindFProperty<FEnumProperty>(FGameplayEffectModifierMagnitude::StaticStruct(), MagnitudeCalculationTypePropertyName))
		{
			const void* ValuePtr = EnumProperty->ContainerPtrToValuePtr<void>(&Magnitude);
			const int64 RawValue = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
			return static_cast<EGameplayEffectMagnitudeCalculation>(RawValue);
		}
		return EGameplayEffectMagnitudeCalculation::ScalableFloat;
	}

	const FScalableFloat* ReadScalableFloat(const FGameplayEffectModifierMagnitude& Magnitude)
	{
		if (const FStructProperty* StructProperty = FindFProperty<FStructProperty>(FGameplayEffectModifierMagnitude::StaticStruct(), ScalableFloatMagnitudePropertyName))
		{
			return StructProperty->ContainerPtrToValuePtr<FScalableFloat>(&Magnitude);
		}
		return nullptr;
	}

	/** 하나의 Modifier를 읽어 Entry를 채운다. 추적 대상 어트리뷰트가 아니면 아무것도 하지 않는다 */
	void ApplyModifierToEntry(const FGameplayModifierInfo& Modifier, const UGameplayEffect* Effect, FACAttributeEntry& OutEntry)
	{
		OutEntry.SourceEffectPath = FSoftObjectPath(Effect->GetClass());
		OutEntry.SourceEffectName = Effect->GetClass()->GetName();
		OutEntry.CurveTable = nullptr;
		OutEntry.CurveRowName = NAME_None;

		const EGameplayEffectMagnitudeCalculation CalculationType = ReadCalculationType(Modifier.ModifierMagnitude);
		if (CalculationType != EGameplayEffectMagnitudeCalculation::ScalableFloat)
		{
			OutEntry.Source = EACAttributeSource::Dynamic;
			OutEntry.Value = 0.f;
			return;
		}

		float StaticMagnitude = 0.f;
		const FString ContextString(TEXT("ACAttributeAudit"));
		if (Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(InitializationLevel, StaticMagnitude, &ContextString))
		{
			OutEntry.Value = StaticMagnitude;
		}

		if (const FScalableFloat* ScalableFloat = ReadScalableFloat(Modifier.ModifierMagnitude))
		{
			if (ScalableFloat->Curve.CurveTable && ScalableFloat->Curve.RowName != NAME_None)
			{
				// FCurveTableRowHandle은 읽기 전용으로 들고 있지만, 이 도구는 에셋을 고치는 것이 목적이라 const를 벗긴다
				OutEntry.Source = EACAttributeSource::CurveTable;
				OutEntry.CurveTable = const_cast<UCurveTable*>(ScalableFloat->Curve.CurveTable.Get());
				OutEntry.CurveRowName = ScalableFloat->Curve.RowName;
				return;
			}
		}

		OutEntry.Source = EACAttributeSource::Constant;
	}

	int32 FindEntryIndex(const TArray<FACAttributeEntry>& Entries, const FGameplayAttribute& Attribute)
	{
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			if (Entries[Index].Attribute == Attribute)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
}

const TArray<FGameplayAttribute>& ACAttributeAudit::GetTrackedAttributes()
{
	static const TArray<FGameplayAttribute> TrackedAttributes = {
		UACAttributeSet::GetMaxHealthAttribute(),
		UACAttributeSet::GetMaxStaminaAttribute(),
		UACAttributeSet::GetStaminaRegenRateAttribute(),
		UACAttributeSet::GetMaxPostureAttribute(),
		UACAttributeSet::GetPostureResistanceAttribute(),
		UACAttributeSet::GetMaxGuardGaugeAttribute(),
		UACAttributeSet::GetGuardBreakResistanceAttribute(),
		UACAttributeSet::GetGuardGaugeRegenRateAttribute(),
		UACAttributeSet::GetMaxBurnGaugeAttribute(),
		UACAttributeSet::GetAttackPowerAttribute(),
		UACAttributeSet::GetDefensePowerAttribute(),
		UACAttributeSet::GetAttackSpeedAttribute(),
		UACAttributeSet::GetMoveSpeedAttribute()
	};
	return TrackedAttributes;
}

const TArray<FGameplayAttribute>& ACAttributeAudit::GetRuntimeGaugeAttributes()
{
	static const TArray<FGameplayAttribute> GaugeAttributes = {
		UACAttributeSet::GetHealthAttribute(),
		UACAttributeSet::GetStaminaAttribute(),
		UACAttributeSet::GetPostureAttribute(),
		UACAttributeSet::GetGuardGaugeAttribute(),
		UACAttributeSet::GetBurnGaugeAttribute()
	};
	return GaugeAttributes;
}

const TArray<FGameplayAttribute>& ACAttributeAudit::GetMetaAttributes()
{
	static const TArray<FGameplayAttribute> MetaAttributes = {
		UACAttributeSet::GetDamageTakenAttribute(),
		UACAttributeSet::GetPostureDamageTakenAttribute(),
		UACAttributeSet::GetGuardDamageTakenAttribute(),
		UACAttributeSet::GetBurnAccumulationAttribute()
	};
	return MetaAttributes;
}

TArray<FACAttributeProfile> ACAttributeAudit::CollectAllProfiles()
{
	using namespace ACAttributeAuditInternal;

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

	const UACAttributeSet* AttributeSetDefaults = GetDefault<UACAttributeSet>();
	const TArray<FGameplayAttribute>& TrackedAttributes = GetTrackedAttributes();

	TArray<FACAttributeProfile> Profiles;
	Profiles.Reserve(StartupAssets.Num());

	for (const FAssetData& AssetData : StartupAssets)
	{
		const UACDataAsset_StartupDataBase* StartupData = Cast<UACDataAsset_StartupDataBase>(AssetData.GetAsset());
		if (!StartupData)
		{
			continue;
		}

		FACAttributeProfile& Profile = Profiles.AddDefaulted_GetRef();
		Profile.CharacterName = AssetData.AssetName.ToString();
		Profile.StartupDataPath = AssetData.GetSoftObjectPath();

		// 생성자 기본값으로 먼저 채운 뒤, 초기화 GE가 건드리는 것만 덮어쓴다
		Profile.Entries.Reserve(TrackedAttributes.Num());
		for (const FGameplayAttribute& Attribute : TrackedAttributes)
		{
			FACAttributeEntry& Entry = Profile.Entries.AddDefaulted_GetRef();
			Entry.Attribute = Attribute;
			Entry.Source = EACAttributeSource::NotSet;
			Entry.Value = AttributeSetDefaults ? Attribute.GetNumericValue(AttributeSetDefaults) : 0.f;
		}

		TArray<UClass*> EffectClasses;
		ReadStartupEffectClasses(StartupData, EffectClasses);

		// 적용 순서대로 순회한다. 뒤에 오는 GE가 앞을 덮어쓰는 것이 런타임 동작과 같다
		for (UClass* EffectClass : EffectClasses)
		{
			const UGameplayEffect* EffectCDO = EffectClass->GetDefaultObject<UGameplayEffect>();
			if (!EffectCDO)
			{
				continue;
			}

			for (const FGameplayModifierInfo& Modifier : EffectCDO->Modifiers)
			{
				const int32 EntryIndex = FindEntryIndex(Profile.Entries, Modifier.Attribute);
				if (EntryIndex == INDEX_NONE)
				{
					continue;
				}

				ApplyModifierToEntry(Modifier, EffectCDO, Profile.Entries[EntryIndex]);

				if (UCurveTable* Table = Profile.Entries[EntryIndex].CurveTable.Get())
				{
					Profile.ReferencedCurveTables.AddUnique(Table);
				}
			}
		}
	}

	return Profiles;
}

bool ACAttributeAudit::SetCurveValue(UCurveTable* Table, const FName RowName, const float Level, const float NewValue)
{
	if (!Table || RowName == NAME_None)
	{
		return false;
	}

	FRealCurve* Curve = Table->FindCurveUnchecked(RowName);
	if (!Curve)
	{
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetCurveValueTransaction", "Edit Attribute Curve Value"));
	Table->Modify();

	Curve->UpdateOrAddKey(Level, NewValue);

	Table->MarkPackageDirty();
	Table->OnCurveTableChanged().Broadcast();
	return true;
}

#undef LOCTEXT_NAMESPACE
