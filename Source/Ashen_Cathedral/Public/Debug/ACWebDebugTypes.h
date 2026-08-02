// 웹 디버그 대시보드와 주고받는 이벤트 구조체 + 60초 링 버퍼.
// Tools/WebDebug/src/lib/schema.ts 와 같은 계약을 구현한다 — 한쪽만 바꾸면 안 된다.

#pragma once

#include "CoreMinimal.h"

#if AC_WEB_DEBUG

namespace ACWebDebug
{
	/** JSON 스키마 버전. 필드를 바꿀 때 함께 올린다. */
	inline constexpr int32 SchemaVersion = 1;

	/** 링 버퍼가 유지하는 과거 구간 (초) */
	inline constexpr float RingBufferSeconds = 60.f;
}

enum class EACWebDebugEventType : uint8
{
	Gauge,
	Ability,
	Tag,
	Hit,
	Marker
};

enum class EACWebDebugSource : uint8
{
	Player,
	Boss
};

enum class EACWebDebugPhase : uint8
{
	None,
	Begin,
	End,
	Instant
};

/** hit 이벤트에만 붙는 부가 정보 */
struct FACWebDebugHitMeta
{
	TArray<FString> AttackTags;
	FString SourceAbility;
	FString Direction;
	float PostureDamage = 0.f;
	float GuardDamage = 0.f;
	bool bWasBlocked = false;
	bool bWasParried = false;
};

/** 타임라인 이벤트 1건. 전투 시작 기준 경과 초(T)를 시간축으로 쓴다. */
struct FACWebDebugEvent
{
	float T = 0.f;
	EACWebDebugEventType Type = EACWebDebugEventType::Marker;
	EACWebDebugSource Src = EACWebDebugSource::Player;
	FString Key;

	/** gauge 전용 — 0.0~1.0 정규화 값. 차트는 이 값만 쓴다 */
	float Norm = 0.f;
	bool bHasNorm = false;

	/** 절대값 (툴팁 표시용) */
	float Raw = 0.f;
	bool bHasRaw = false;

	EACWebDebugPhase Phase = EACWebDebugPhase::None;

	TSharedPtr<FACWebDebugHitMeta> Meta;
};

/** WebSocket 연결 직후 1회, 전투 시작 시 갱신되어 전송되는 세션 헤더 */
struct FACWebDebugSession
{
	FString SessionId;
	int32 RunSeed = 0;
	FString BossId;
	int32 Attempt = 0;
	FString Weapon;
	FString BuildConfig;
	FString StartedAtUtc;
};

/**
 * 최근 RingBufferSeconds 구간만 유지하는 이벤트 버퍼.
 * 앞쪽에서 버리는 일이 잦으므로 시작 오프셋을 두고, 오프셋이 커지면 한 번에 압축한다.
 */
class FACWebDebugRingBuffer
{
public:
	void Add(FACWebDebugEvent&& Event)
	{
		Events.Add(MoveTemp(Event));
		Trim();
	}

	void Reset()
	{
		Events.Reset();
		Head = 0;
	}

	int32 Num() const { return Events.Num() - Head; }

	const FACWebDebugEvent& operator[](int32 Index) const { return Events[Head + Index]; }

private:
	void Trim()
	{
		const float Newest = Events.Last().T;
		const float Oldest = Newest - ACWebDebug::RingBufferSeconds;
		while (Head < Events.Num() && Events[Head].T < Oldest)
		{
			++Head;
		}
		// 앞쪽 절반이 비면 한 번에 압축한다 — 매 프레임 RemoveAt(0) 하는 비용을 피한다
		if (Head > 0 && Head * 2 >= Events.Num())
		{
			Events.RemoveAt(0, Head, EAllowShrinking::No);
			Head = 0;
		}
	}

	TArray<FACWebDebugEvent> Events;
	int32 Head = 0;
};

#endif // AC_WEB_DEBUG
