#include "PPPresetJsonReader.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	/** 객체 하나를 프리셋 항목으로 해석한다. Settings가 없으면 객체 자체를 설정으로 본다. */
	bool ParsePresetObject(const TSharedPtr<FJsonObject>& InObject, const FString& InFallbackName, FPPPresetEntry& OutEntry)
	{
		if (!InObject.IsValid())
		{
			return false;
		}

		OutEntry.PresetName = InFallbackName;
		InObject->TryGetStringField(TEXT("Preset"), OutEntry.PresetName);
		InObject->TryGetStringField(TEXT("Package"), OutEntry.PackagePath);

		const TSharedPtr<FJsonObject>* SettingsObject = nullptr;
		if (InObject->TryGetObjectField(TEXT("Settings"), SettingsObject) && SettingsObject && SettingsObject->IsValid())
		{
			OutEntry.Settings = *SettingsObject;
		}
		else
		{
			// Settings 래퍼가 없는 형태 — 객체 자체가 설정 묶음이다.
			OutEntry.Settings = InObject;
		}

		return OutEntry.Settings.IsValid() && !OutEntry.PresetName.IsEmpty();
	}
}

bool FPPPresetJsonReader::LoadFromFile(const FString& InFilePath, TArray<FPPPresetEntry>& OutPresets, FString& OutError)
{
	OutPresets.Reset();

	FString FileContents;
	if (!FFileHelper::LoadFileToString(FileContents, *InFilePath))
	{
		OutError = FString::Printf(TEXT("파일을 읽을 수 없습니다: %s"), *InFilePath);
		return false;
	}

	return LoadFromString(FileContents, OutPresets, OutError);
}

bool FPPPresetJsonReader::LoadFromString(const FString& InJsonText, TArray<FPPPresetEntry>& OutPresets, FString& OutError)
{
	OutPresets.Reset();
	OutError.Reset();

	if (InJsonText.IsEmpty())
	{
		OutError = TEXT("JSON 내용이 비어 있습니다.");
		return false;
	}

	// 최상위가 배열인지 객체인지 모르므로, 배열을 먼저 시도하고 실패하면 객체로 읽는다.
	{
		TArray<TSharedPtr<FJsonValue>> RootArray;
		const TSharedRef<TJsonReader<>> ArrayReader = TJsonReaderFactory<>::Create(InJsonText);
		if (FJsonSerializer::Deserialize(ArrayReader, RootArray))
		{
			for (int32 Index = 0; Index < RootArray.Num(); ++Index)
			{
				const TSharedPtr<FJsonObject>* Item = nullptr;
				if (RootArray[Index].IsValid() && RootArray[Index]->TryGetObject(Item) && Item)
				{
					FPPPresetEntry Entry;
					if (ParsePresetObject(*Item, FString::Printf(TEXT("Preset_%d"), Index), Entry))
					{
						OutPresets.Add(MoveTemp(Entry));
					}
				}
			}

			if (OutPresets.Num() > 0)
			{
				return true;
			}
		}
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> ObjectReader = TJsonReaderFactory<>::Create(InJsonText);
	if (!FJsonSerializer::Deserialize(ObjectReader, RootObject) || !RootObject.IsValid())
	{
		OutError = TEXT("JSON 파싱에 실패했습니다. 최상위가 배열이나 객체여야 합니다.");
		return false;
	}

	// 형태 2: { "Presets": [ ... ] }
	const TArray<TSharedPtr<FJsonValue>>* PresetsArray = nullptr;
	if (RootObject->TryGetArrayField(TEXT("Presets"), PresetsArray) && PresetsArray)
	{
		for (int32 Index = 0; Index < PresetsArray->Num(); ++Index)
		{
			const TSharedPtr<FJsonObject>* Item = nullptr;
			if ((*PresetsArray)[Index].IsValid() && (*PresetsArray)[Index]->TryGetObject(Item) && Item)
			{
				FPPPresetEntry Entry;
				if (ParsePresetObject(*Item, FString::Printf(TEXT("Preset_%d"), Index), Entry))
				{
					OutPresets.Add(MoveTemp(Entry));
				}
			}
		}

		if (OutPresets.Num() > 0)
		{
			return true;
		}
	}

	// 형태 3: { "이름": { ... }, ... }
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : RootObject->Values)
	{
		const TSharedPtr<FJsonObject>* Item = nullptr;
		if (Pair.Value.IsValid() && Pair.Value->TryGetObject(Item) && Item)
		{
			FPPPresetEntry Entry;
			if (ParsePresetObject(*Item, Pair.Key, Entry))
			{
				OutPresets.Add(MoveTemp(Entry));
			}
		}
	}

	if (OutPresets.Num() == 0)
	{
		OutError = TEXT("프리셋을 하나도 찾지 못했습니다. Preset/Package/Settings 구조를 확인하세요.");
		return false;
	}

	return true;
}
