#include "PPPresetApplier.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Components/PostProcessComponent.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/Actor.h"
#include "Engine/Scene.h"
#include "ScopedTransaction.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "PPPresetApplier"

namespace PPPresetApplierPrivate
{
	/** JSON 값에서 숫자 성분들을 뽑는다. 배열, {X,Y,Z,W}, {R,G,B,A}, 스칼라를 모두 받는다. */
	bool ExtractComponents(const TSharedPtr<FJsonValue>& InValue, TArray<double>& OutComponents)
	{
		OutComponents.Reset();

		const TArray<TSharedPtr<FJsonValue>>* AsArray = nullptr;
		if (InValue->TryGetArray(AsArray) && AsArray)
		{
			for (const TSharedPtr<FJsonValue>& Element : *AsArray)
			{
				double Number = 0.0;
				if (Element.IsValid() && Element->TryGetNumber(Number))
				{
					OutComponents.Add(Number);
				}
			}
			return OutComponents.Num() > 0;
		}

		const TSharedPtr<FJsonObject>* AsObject = nullptr;
		if (InValue->TryGetObject(AsObject) && AsObject && AsObject->IsValid())
		{
			// R/G/B/A 우선, 없으면 X/Y/Z/W. 두 표기가 섞여 들어오는 JSON이 흔하다.
			const bool bUseRGBA = (*AsObject)->HasField(TEXT("R")) || (*AsObject)->HasField(TEXT("G"));
			static const TCHAR* KeysRGBA[4] = { TEXT("R"), TEXT("G"), TEXT("B"), TEXT("A") };
			static const TCHAR* KeysXYZW[4] = { TEXT("X"), TEXT("Y"), TEXT("Z"), TEXT("W") };
			const TCHAR* const* Keys = bUseRGBA ? KeysRGBA : KeysXYZW;

			for (int32 Index = 0; Index < 4; ++Index)
			{
				double Number = 0.0;
				if ((*AsObject)->TryGetNumberField(Keys[Index], Number))
				{
					OutComponents.Add(Number);
				}
				else
				{
					break;
				}
			}
			return OutComponents.Num() > 0;
		}

		double Scalar = 0.0;
		if (InValue->TryGetNumber(Scalar))
		{
			OutComponents.Add(Scalar);
			return true;
		}

		return false;
	}

	/** JSON에서 에셋 경로를 뽑는다. Texture2D 같은 클래스 접두 래핑도 벗겨낸다. */
	bool ExtractAssetPath(const TSharedPtr<FJsonValue>& InValue, FString& OutPath)
	{
		OutPath.Reset();

		if (!InValue->TryGetString(OutPath))
		{
			const TSharedPtr<FJsonObject>* AsObject = nullptr;
			if (InValue->TryGetObject(AsObject) && AsObject && AsObject->IsValid())
			{
				if (!(*AsObject)->TryGetStringField(TEXT("AssetPath"), OutPath))
				{
					(*AsObject)->TryGetStringField(TEXT("ObjectPath"), OutPath);
				}
			}
		}

		if (OutPath.IsEmpty())
		{
			return false;
		}

		// 0x27은 작은따옴표. ClassName'경로' 형태에서 경로만 남긴다.
		const TCHAR QuoteChar = static_cast<TCHAR>(0x27);
		int32 QuoteIndex = INDEX_NONE;
		if (OutPath.FindChar(QuoteChar, QuoteIndex))
		{
			OutPath = OutPath.RightChop(QuoteIndex + 1);
			if (OutPath.Len() > 0 && OutPath[OutPath.Len() - 1] == QuoteChar)
			{
				OutPath.LeftChopInline(1);
			}
		}

		OutPath.TrimStartAndEndInline();
		return !OutPath.IsEmpty();
	}

	/** 프로젝트에 실제로 존재하는 에셋만 로드한다. 없으면 nullptr을 돌려주고 경로를 기록한다. */
	UObject* ResolveExistingAsset(const FString& InPath, FPPApplyPlan& InOutPlan)
	{
		const FSoftObjectPath SoftPath(InPath);
		if (!SoftPath.IsValid())
		{
			InOutPlan.MissingAssetPaths.AddUnique(InPath);
			return nullptr;
		}

		const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		const FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(SoftPath, false);
		if (!AssetData.IsValid())
		{
			InOutPlan.MissingAssetPaths.AddUnique(InPath);
			return nullptr;
		}

		UObject* Loaded = SoftPath.TryLoad();
		if (!Loaded)
		{
			InOutPlan.MissingAssetPaths.AddUnique(InPath);
		}
		return Loaded;
	}

	bool SetEnumValue(UEnum* InEnum, FNumericProperty* InUnderlying, void* InValuePtr, const TSharedPtr<FJsonValue>& InValue)
	{
		if (!InEnum || !InUnderlying)
		{
			return false;
		}

		FString AsString;
		if (InValue->TryGetString(AsString))
		{
			int64 EnumValue = InEnum->GetValueByNameString(AsString);
			if (EnumValue == INDEX_NONE)
			{
				// AEM_Histogram 같은 짧은 이름을 EEnumName::AEM_Histogram 으로 보정한다.
				EnumValue = InEnum->GetValueByNameString(FString::Printf(TEXT("%s::%s"), *InEnum->GetName(), *AsString));
			}
			if (EnumValue == INDEX_NONE)
			{
				return false;
			}
			InUnderlying->SetIntPropertyValue(InValuePtr, EnumValue);
			return true;
		}

		double AsNumber = 0.0;
		if (InValue->TryGetNumber(AsNumber))
		{
			InUnderlying->SetIntPropertyValue(InValuePtr, static_cast<int64>(AsNumber));
			return true;
		}

		return false;
	}

	/** WeightedBlendables. { "Array": [ { "Weight": 1.0, "Object": "/Game/..." } ] } 또는 배열 그대로. */
	bool SetWeightedBlendables(void* InValuePtr, const TSharedPtr<FJsonValue>& InValue, FPPApplyPlan& InOutPlan)
	{
		const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
		if (!InValue->TryGetArray(Entries))
		{
			const TSharedPtr<FJsonObject>* AsObject = nullptr;
			if (InValue->TryGetObject(AsObject) && AsObject && AsObject->IsValid())
			{
				(*AsObject)->TryGetArrayField(TEXT("Array"), Entries);
			}
		}

		if (!Entries)
		{
			return false;
		}

		FWeightedBlendables* Blendables = static_cast<FWeightedBlendables*>(InValuePtr);
		Blendables->Array.Reset();

		for (const TSharedPtr<FJsonValue>& Entry : *Entries)
		{
			const TSharedPtr<FJsonObject>* EntryObject = nullptr;
			if (!Entry.IsValid() || !Entry->TryGetObject(EntryObject) || !EntryObject)
			{
				continue;
			}

			FWeightedBlendable Blendable;
			double Weight = 1.0;
			(*EntryObject)->TryGetNumberField(TEXT("Weight"), Weight);
			Blendable.Weight = static_cast<float>(Weight);

			const TSharedPtr<FJsonValue> ObjectValue = (*EntryObject)->TryGetField(TEXT("Object"));
			if (ObjectValue.IsValid())
			{
				FString AssetPath;
				if (ExtractAssetPath(ObjectValue, AssetPath))
				{
					// 없는 에셋은 경고만 남기고 항목을 건너뛴다. 적용 자체를 중단하지 않는다.
					UObject* Resolved = ResolveExistingAsset(AssetPath, InOutPlan);
					if (!Resolved)
					{
						continue;
					}
					Blendable.Object = Resolved;
				}
			}

			Blendables->Array.Add(Blendable);
		}

		return true;
	}

	/**
	 * @brief JSON 값 하나를 프로퍼티에 기록한다.
	 * @return 기록했으면 true. 타입이 안 맞거나 에셋이 없으면 false — 호출측이 건너뛴다.
	 */
	bool SetPropertyFromJson(FProperty* InProperty, void* InValuePtr, const TSharedPtr<FJsonValue>& InValue, FPPApplyPlan& InOutPlan)
	{
		if (!InProperty || !InValuePtr || !InValue.IsValid())
		{
			return false;
		}

		// bOverride_ 비트필드를 포함한 모든 bool
		if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(InProperty))
		{
			bool bValue = false;
			if (!InValue->TryGetBool(bValue))
			{
				double AsNumber = 0.0;
				if (!InValue->TryGetNumber(AsNumber))
				{
					return false;
				}
				bValue = !FMath::IsNearlyZero(AsNumber);
			}
			BoolProperty->SetPropertyValue(InValuePtr, bValue);
			return true;
		}

		// 순수 enum (ELumenRayLightingModeOverride 등)
		if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(InProperty))
		{
			return SetEnumValue(EnumProperty->GetEnum(), EnumProperty->GetUnderlyingProperty(), InValuePtr, InValue);
		}

		// TEnumAsByte 계열
		if (FByteProperty* ByteProperty = CastField<FByteProperty>(InProperty))
		{
			if (UEnum* Enum = ByteProperty->GetIntPropertyEnum())
			{
				return SetEnumValue(Enum, ByteProperty, InValuePtr, InValue);
			}
		}

		// int / float / double / enum 없는 byte
		if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(InProperty))
		{
			double AsNumber = 0.0;
			if (!InValue->TryGetNumber(AsNumber))
			{
				bool bValue = false;
				if (!InValue->TryGetBool(bValue))
				{
					return false;
				}
				AsNumber = bValue ? 1.0 : 0.0;
			}

			if (NumericProperty->IsFloatingPoint())
			{
				NumericProperty->SetFloatingPointPropertyValue(InValuePtr, AsNumber);
			}
			else
			{
				NumericProperty->SetIntPropertyValue(InValuePtr, static_cast<int64>(AsNumber));
			}
			return true;
		}

		// LUT 텍스처, 커브, 머티리얼 등
		if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(InProperty))
		{
			FString AssetPath;
			if (!ExtractAssetPath(InValue, AssetPath) || AssetPath.Equals(TEXT("None"), ESearchCase::IgnoreCase))
			{
				ObjectProperty->SetObjectPropertyValue(InValuePtr, nullptr);
				return true;
			}

			UObject* Resolved = ResolveExistingAsset(AssetPath, InOutPlan);
			if (!Resolved)
			{
				return false;
			}
			if (ObjectProperty->PropertyClass && !Resolved->IsA(ObjectProperty->PropertyClass))
			{
				return false;
			}
			ObjectProperty->SetObjectPropertyValue(InValuePtr, Resolved);
			return true;
		}

		if (FStructProperty* StructProperty = CastField<FStructProperty>(InProperty))
		{
			UScriptStruct* Struct = StructProperty->Struct;

			if (Struct == FWeightedBlendables::StaticStruct())
			{
				return SetWeightedBlendables(InValuePtr, InValue, InOutPlan);
			}

			TArray<double> Components;
			if (ExtractComponents(InValue, Components))
			{
				// JSON에 없는 성분은 기존 값을 유지한다. 알파나 W가 빠진 데이터가 흔하다.
				if (Struct == TBaseStructure<FLinearColor>::Get())
				{
					FLinearColor* Color = static_cast<FLinearColor*>(InValuePtr);
					if (Components.IsValidIndex(0)) { Color->R = static_cast<float>(Components[0]); }
					if (Components.IsValidIndex(1)) { Color->G = static_cast<float>(Components[1]); }
					if (Components.IsValidIndex(2)) { Color->B = static_cast<float>(Components[2]); }
					if (Components.IsValidIndex(3)) { Color->A = static_cast<float>(Components[3]); }
					return true;
				}
				if (Struct == TBaseStructure<FVector4>::Get())
				{
					FVector4* Vector = static_cast<FVector4*>(InValuePtr);
					if (Components.IsValidIndex(0)) { Vector->X = Components[0]; }
					if (Components.IsValidIndex(1)) { Vector->Y = Components[1]; }
					if (Components.IsValidIndex(2)) { Vector->Z = Components[2]; }
					if (Components.IsValidIndex(3)) { Vector->W = Components[3]; }
					return true;
				}
				if (Struct == TBaseStructure<FVector>::Get())
				{
					FVector* Vector = static_cast<FVector*>(InValuePtr);
					if (Components.IsValidIndex(0)) { Vector->X = Components[0]; }
					if (Components.IsValidIndex(1)) { Vector->Y = Components[1]; }
					if (Components.IsValidIndex(2)) { Vector->Z = Components[2]; }
					return true;
				}
				if (Struct == TBaseStructure<FVector2D>::Get())
				{
					FVector2D* Vector = static_cast<FVector2D*>(InValuePtr);
					if (Components.IsValidIndex(0)) { Vector->X = Components[0]; }
					if (Components.IsValidIndex(1)) { Vector->Y = Components[1]; }
					return true;
				}
			}
		}

		// 마지막 안전망 — 문자열로 들어온 값은 언리얼 텍스트 포맷으로 해석해본다.
		FString AsText;
		if (InValue->TryGetString(AsText))
		{
			class FSilentErrors : public FOutputDevice
			{
			public:
				virtual void Serialize(const TCHAR*, ELogVerbosity::Type, const FName&) override {}
			} SilentErrors;

			return InProperty->ImportText_Direct(*AsText, InValuePtr, nullptr, PPF_None, &SilentErrors) != nullptr;
		}

		return false;
	}

	/**
	 * @brief 미리보기와 실제 적용이 공유하는 단일 경로.
	 * @note 항상 사본에 기록한 뒤 비교하므로 미리보기 결과와 적용 결과가 어긋날 수 없다.
	 */
	void BuildResult(const FPPPresetEntry& InPreset, const FPostProcessSettings& InCurrent,
		FPostProcessSettings& OutResult, FPPApplyPlan& OutPlan)
	{
		OutPlan.Reset();
		OutResult = InCurrent;

		if (!InPreset.Settings.IsValid())
		{
			return;
		}

		FPostProcessSettings Original = InCurrent;
		UScriptStruct* SettingsStruct = FPostProcessSettings::StaticStruct();

		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : InPreset.Settings->Values)
		{
			// Preset/Package는 메타데이터이지 설정이 아니다.
			if (Pair.Key.Equals(TEXT("Preset")) || Pair.Key.Equals(TEXT("Package")) || Pair.Key.Equals(TEXT("Settings")))
			{
				continue;
			}

			FProperty* Property = SettingsStruct->FindPropertyByName(FName(*Pair.Key));
			if (!Property)
			{
				// 현재 엔진에 없는 프로퍼티 — 건너뛰고 경고만 남긴다.
				OutPlan.UnknownProperties.AddUnique(Pair.Key);
				continue;
			}

			void* NewValuePtr = Property->ContainerPtrToValuePtr<void>(&OutResult);
			void* OldValuePtr = Property->ContainerPtrToValuePtr<void>(&Original);

			FString OldText;
			Property->ExportTextItem_Direct(OldText, OldValuePtr, nullptr, nullptr, PPF_None);

			if (!SetPropertyFromJson(Property, NewValuePtr, Pair.Value, OutPlan))
			{
				// 실패한 프로퍼티는 원래 값으로 되돌린다. JSON에 없는 것과 동일하게 취급된다.
				Property->CopyCompleteValue(NewValuePtr, OldValuePtr);
				OutPlan.SkippedProperties.AddUnique(Pair.Key);
				continue;
			}

			FString NewText;
			Property->ExportTextItem_Direct(NewText, NewValuePtr, nullptr, nullptr, PPF_None);

			if (!OldText.Equals(NewText, ESearchCase::CaseSensitive))
			{
				OutPlan.Changes.Add(FPPPropertyChange{ Pair.Key, OldText, NewText });
			}

			if (Pair.Key.StartsWith(TEXT("bOverride_")))
			{
				bool bEnabled = false;
				if (Pair.Value->TryGetBool(bEnabled) && bEnabled)
				{
					OutPlan.AppliedOverrides.AddUnique(Pair.Key);
				}
			}
		}
	}
}

void FPPPresetApplier::BuildPlan(const FPPPresetEntry& InPreset, const FPostProcessSettings& InCurrent, FPPApplyPlan& OutPlan)
{
	FPostProcessSettings Discarded;
	PPPresetApplierPrivate::BuildResult(InPreset, InCurrent, Discarded, OutPlan);
}

void FPPPresetApplier::ApplyToSettings(const FPPPresetEntry& InPreset, FPostProcessSettings& InOutSettings, FPPApplyPlan& OutPlan)
{
	FPostProcessSettings Result;
	PPPresetApplierPrivate::BuildResult(InPreset, InOutSettings, Result, OutPlan);
	InOutSettings = Result;
}

namespace PPPresetApplierPrivate
{
	/** 볼륨과 컴포넌트가 공유하는 적용 경로. 트랜잭션·Modify·Dirty 처리를 한곳에 둔다. */
	bool ApplyToObject(const FPPPresetEntry& InPreset, UObject* InTarget, FPostProcessSettings& InOutSettings, FPPApplyPlan& OutPlan)
	{
		if (!IsValid(InTarget))
		{
			return false;
		}

		FPostProcessSettings Result;
		BuildResult(InPreset, InOutSettings, Result, OutPlan);

		// Undo/Redo — 트랜잭션 안에서 Modify를 부른 뒤에 값을 쓴다.
		const FScopedTransaction Transaction(NSLOCTEXT("PPPresetApplier", "ApplyPreset", "Apply Post Process Preset"));
		InTarget->Modify();

		// 컴포넌트라면 소유 액터도 함께 기록해야 되돌리기가 온전하다.
		if (const UActorComponent* AsComponent = Cast<UActorComponent>(InTarget))
		{
			if (AActor* Owner = AsComponent->GetOwner())
			{
				Owner->Modify();
			}
		}

		InOutSettings = Result;

		InTarget->PostEditChange();
		InTarget->MarkPackageDirty();

		return true;
	}
}

bool FPPPresetApplier::ApplyToVolume(const FPPPresetEntry& InPreset, APostProcessVolume* InVolume, FPPApplyPlan& OutPlan)
{
	if (!IsValid(InVolume))
	{
		return false;
	}
	return PPPresetApplierPrivate::ApplyToObject(InPreset, InVolume, InVolume->Settings, OutPlan);
}

bool FPPPresetApplier::ApplyToComponent(const FPPPresetEntry& InPreset, UPostProcessComponent* InComponent, FPPApplyPlan& OutPlan)
{
	if (!IsValid(InComponent))
	{
		return false;
	}
	return PPPresetApplierPrivate::ApplyToObject(InPreset, InComponent, InComponent->Settings, OutPlan);
}

#undef LOCTEXT_NAMESPACE
