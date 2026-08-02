// 웹 디버그 대시보드의 게임 측 서버 구현.

#include "Debug/ACWebDebugSubsystem.h"

#if AC_WEB_DEBUG

	#include "AbilitySystemComponent.h"
	#include "Character/Player/ACPlayerCharacter.h"
	#include "Debug/ACRunLogSubsystem.h"
	#include "HttpPath.h"
	#include "HttpServerModule.h"
	#include "HttpServerRequest.h"
	#include "HttpServerResponse.h"
	#include "IHttpRouter.h"
	#include "IWebSocketNetworkingModule.h"
	#include "IWebSocketServer.h"
	#include "INetworkingWebSocket.h"
	#include "Misc/App.h"
	#include "Misc/FileHelper.h"
	#include "Misc/Paths.h"
	#include "Modules/ModuleManager.h"
	#include "Serialization/JsonSerializer.h"
	#include "Serialization/JsonWriter.h"
	#include "GameplayTags/ACGameplayTags_Combat.h"
	#include "GameplayTags/ACGameplayTags_Enemy.h"
	#include "GameplayTags/ACGameplayTags_Player.h"
	#include "GameplayTags/ACGameplayTags_Shared.h"

DEFINE_LOG_CATEGORY_STATIC(LogACWebDebug, Log, All);

using FCondensedWriter = TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>;

namespace
{
	TAutoConsoleVariable<int32> CVarWebDebugEnabled(
		TEXT("ac.WebDebug"),
		0,
		TEXT("웹 디버그 서버 on/off. 1 이면 127.0.0.1 에서 HTTP/WebSocket 서버를 연다."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarWebDebugPort(
		TEXT("ac.WebDebug.Port"),
		8091,
		TEXT("웹 디버그 HTTP 포트. WebSocket 은 이 값 +1 을 쓴다."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarWebDebugRate(
		TEXT("ac.WebDebug.Rate"),
		15,
		TEXT("웹 디버그 전송 Hz. 게이지 스냅샷 주기이기도 하다."),
		ECVF_Default);

	/** 대시보드가 기록하는 게이지 5종. 이 목록에 없는 어트리뷰트는 기록하지 않는다. */
	const TCHAR* const TrackedGauges[] = {
		TEXT("Health"),
		TEXT("Stamina"),
		TEXT("Posture"),
		TEXT("GuardGauge"),
		TEXT("BurnGauge"),
	};

	const TCHAR* SourceToString(EACWebDebugSource Src)
	{
		return Src == EACWebDebugSource::Player ? TEXT("player") : TEXT("boss");
	}

	const TCHAR* TypeToString(EACWebDebugEventType Type)
	{
		switch (Type)
		{
			case EACWebDebugEventType::Gauge:
				return TEXT("gauge");
			case EACWebDebugEventType::Ability:
				return TEXT("ability");
			case EACWebDebugEventType::Tag:
				return TEXT("tag");
			case EACWebDebugEventType::Hit:
				return TEXT("hit");
			default:
				return TEXT("marker");
		}
	}

	const TCHAR* PhaseToString(EACWebDebugPhase Phase)
	{
		switch (Phase)
		{
			case EACWebDebugPhase::Begin:
				return TEXT("begin");
			case EACWebDebugPhase::End:
				return TEXT("end");
			case EACWebDebugPhase::Instant:
				return TEXT("instant");
			default:
				return nullptr;
		}
	}

	/** 타임라인에 구간으로 남길 상태 태그들. 여기 없는 태그는 구독하지 않는다. */
	const TArray<FGameplayTag>& GetTrackedTags()
	{
		static TArray<FGameplayTag> Tags = {
			// 무적 / 판정 구간 — 타임라인의 핵심
			ACGameplayTags::Shared_Status_Invincible,
			ACGameplayTags::Shared_Status_Parry,
			ACGameplayTags::Shared_Status_SuperArmor,
			ACGameplayTags::Player_Status_ComboWindow,
			ACGameplayTags::Player_Status_SpecialLinkWindow,
			// 상태
			ACGameplayTags::Shared_Status_PostureBroken,
			ACGameplayTags::Shared_Status_Stagger,
			ACGameplayTags::Shared_Status_HitReact,
			ACGameplayTags::Shared_Status_HitReact_Front,
			ACGameplayTags::Shared_Status_HitReact_Back,
			ACGameplayTags::Shared_Status_HitReact_Left,
			ACGameplayTags::Shared_Status_HitReact_Right,
			ACGameplayTags::Shared_Status_Dead,
			ACGameplayTags::Shared_Status_CanCounterAttack,
			ACGameplayTags::Player_Status_GuardBroken,
			ACGameplayTags::Player_Status_Blocking,
			ACGameplayTags::Player_Status_Rolling,
			ACGameplayTags::Player_Status_CriticalAttacking,
			ACGameplayTags::Player_Status_Stamina_RegenBlocked,
			ACGameplayTags::Player_ActionState_Attacking,
			ACGameplayTags::Player_ActionState_Dodging,
			ACGameplayTags::Player_ActionState_Parrying,
			ACGameplayTags::Player_ActionState_LockOn,
			ACGameplayTags::Enemy_Status_Attacking,
			ACGameplayTags::Enemy_Status_Blocking,
			ACGameplayTags::Enemy_Status_Dodging,
			ACGameplayTags::Enemy_Status_Strafing,
			ACGameplayTags::Enemy_Status_UnderAttack,
			ACGameplayTags::Enemy_Status_CriticalAttacking,
			ACGameplayTags::Enemy_Status_PressureCountering,
			ACGameplayTags::Enemy_Status_Phase2,
		};
		return Tags;
	}

	FString GetContentType(const FString& Extension)
	{
		if (Extension == TEXT("html")) return TEXT("text/html; charset=utf-8");
		if (Extension == TEXT("js")) return TEXT("text/javascript; charset=utf-8");
		if (Extension == TEXT("css")) return TEXT("text/css; charset=utf-8");
		if (Extension == TEXT("json")) return TEXT("application/json; charset=utf-8");
		if (Extension == TEXT("svg")) return TEXT("image/svg+xml");
		if (Extension == TEXT("png")) return TEXT("image/png");
		if (Extension == TEXT("jpg") || Extension == TEXT("jpeg")) return TEXT("image/jpeg");
		if (Extension == TEXT("woff2")) return TEXT("font/woff2");
		if (Extension == TEXT("ico")) return TEXT("image/x-icon");
		return TEXT("application/octet-stream");
	}

	FString GetWebRootDir()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("WebDebug"));
	}

	/** 이벤트 1건을 JSON 오브젝트로 쓴다 */
	void WriteEventJson(const TSharedRef<FCondensedWriter>& Writer, const FACWebDebugEvent& Event)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("t"), FMath::RoundToFloat(Event.T * 1000.f) / 1000.f);
		Writer->WriteValue(TEXT("type"), TypeToString(Event.Type));
		Writer->WriteValue(TEXT("src"), SourceToString(Event.Src));
		Writer->WriteValue(TEXT("key"), Event.Key);
		if (Event.bHasNorm)
		{
			Writer->WriteValue(TEXT("norm"), FMath::RoundToFloat(Event.Norm * 1000.f) / 1000.f);
		}
		if (Event.bHasRaw)
		{
			Writer->WriteValue(TEXT("raw"), Event.Raw);
		}
		if (const TCHAR* PhaseStr = PhaseToString(Event.Phase))
		{
			Writer->WriteValue(TEXT("phase"), PhaseStr);
		}
		if (Event.Meta.IsValid())
		{
			const FACWebDebugHitMeta& Meta = *Event.Meta;
			Writer->WriteObjectStart(TEXT("meta"));
			Writer->WriteArrayStart(TEXT("attackTags"));
			for (const FString& Tag : Meta.AttackTags)
			{
				Writer->WriteValue(Tag);
			}
			Writer->WriteArrayEnd();
			Writer->WriteValue(TEXT("sourceAbility"), Meta.SourceAbility);
			Writer->WriteValue(TEXT("direction"), Meta.Direction);
			Writer->WriteValue(TEXT("postureDamage"), Meta.PostureDamage);
			Writer->WriteValue(TEXT("guardDamage"), Meta.GuardDamage);
			Writer->WriteValue(TEXT("wasBlocked"), Meta.bWasBlocked);
			Writer->WriteValue(TEXT("wasParried"), Meta.bWasParried);
			Writer->WriteObjectEnd();
		}
		Writer->WriteObjectEnd();
	}
} // namespace

/* ────────────────────────────── 수명주기 ────────────────────────────── */

void UACWebDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Session.BuildConfig = LexToString(FApp::GetBuildConfiguration());
	Session.SessionId = FGuid::NewGuid().ToString(EGuidFormats::DigitsLower).Left(8);
	Session.StartedAtUtc = FDateTime::UtcNow().ToIso8601();
	CombatStartSeconds = FPlatformTime::Seconds();

	// CVar 상태를 매 프레임 확인해 서버를 켜고 끈다 — 콘솔 명령 등록보다 단순하고 PIE 재시작에도 안전하다
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UACWebDebugSubsystem::Tick), 0.f);
}

void UACWebDebugSubsystem::Deinitialize()
{
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}
	StopServer();
	Super::Deinitialize();
}

UACWebDebugSubsystem* UACWebDebugSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UACWebDebugSubsystem>() : nullptr;
}

EACWebDebugSource UACWebDebugSubsystem::ResolveSource(const AActor* Actor)
{
	return Cast<AACPlayerCharacter>(Actor) ? EACWebDebugSource::Player : EACWebDebugSource::Boss;
}

float UACWebDebugSubsystem::GetCombatTime() const
{
	return static_cast<float>(FPlatformTime::Seconds() - CombatStartSeconds);
}

/* ────────────────────────────── 서버 ────────────────────────────── */

void UACWebDebugSubsystem::StartServer()
{
	if (bServerRunning)
	{
		return;
	}

	const uint32 HttpPort = static_cast<uint32>(FMath::Clamp(CVarWebDebugPort.GetValueOnGameThread(), 1024, 65534));
	const uint32 WsPort = HttpPort + 1;

	// HTTPServer 는 기본 BindAddress 가 localhost 다. DefaultEngine.ini 에서 바꾸지 말 것 —
	// 이 서버는 절대 외부 인터페이스에 바인딩되어서는 안 된다.
	Router = FHttpServerModule::Get().GetHttpRouter(HttpPort, /*bFailOnBindFailure*/ true);
	if (!Router.IsValid())
	{
		UE_LOG(LogACWebDebug, Error, TEXT("HTTP 포트 %u 바인딩에 실패했습니다. ac.WebDebug.Port 로 다른 포트를 지정하고 ac.WebDebug 1 을 다시 실행하세요."), HttpPort);
		// 켜진 채로 두면 매 프레임 재시도하며 로그가 폭주한다
		CVarWebDebugEnabled->Set(0, ECVF_SetByCode);
		return;
	}
	BindRoutes();
	FHttpServerModule::Get().StartAllListeners();

	IWebSocketNetworkingModule& WsModule = FModuleManager::LoadModuleChecked<IWebSocketNetworkingModule>(TEXT("WebSocketNetworking"));
	// 모듈은 TUniquePtr 로 넘겨주지만, 헤더에서 불완전 타입으로 들고 있으려면 TSharedPtr 이어야 한다
	WebSocketServer = MakeShareable(WsModule.CreateServer().Release());

	FWebSocketClientConnectedCallBack ConnectedCallback;
	ConnectedCallback.BindUObject(this, &UACWebDebugSubsystem::OnClientConnected);

	if (!WebSocketServer.IsValid() || !WebSocketServer->Init(WsPort, ConnectedCallback, TEXT("127.0.0.1")))
	{
		UE_LOG(LogACWebDebug, Error, TEXT("WebSocket 포트 %u 초기화에 실패했습니다."), WsPort);
		WebSocketServer.Reset();
		StopServer();
		CVarWebDebugEnabled->Set(0, ECVF_SetByCode);
		return;
	}

	bServerRunning = true;
	LastSendSeconds = FPlatformTime::Seconds();
	UE_LOG(LogACWebDebug, Display, TEXT("웹 디버그 서버 시작 — http://127.0.0.1:%u  (ws://127.0.0.1:%u)"), HttpPort, WsPort);
}

void UACWebDebugSubsystem::StopServer()
{
	if (Router.IsValid())
	{
		if (StaticPreprocessorHandle.IsValid())
		{
			Router->UnregisterRequestPreprocessor(StaticPreprocessorHandle);
		}
		for (const FHttpRouteHandle& Handle : RouteHandles)
		{
			Router->UnbindRoute(Handle);
		}
		Router.Reset();
	}
	StaticPreprocessorHandle.Reset();
	RouteHandles.Reset();
	WebSocketServer.Reset();
	Clients.Reset();

	if (bServerRunning)
	{
		UE_LOG(LogACWebDebug, Display, TEXT("웹 디버그 서버 정지"));
	}
	bServerRunning = false;
}

bool UACWebDebugSubsystem::Tick(float DeltaSeconds)
{
	const bool bWanted = CVarWebDebugEnabled.GetValueOnGameThread() != 0;
	if (bWanted && !bServerRunning)
	{
		StartServer();
	}
	else if (!bWanted && bServerRunning)
	{
		StopServer();
	}

	if (!bServerRunning)
	{
		return true;
	}

	// lws 는 매 프레임 서비스해야 연결 수립·수신이 진행된다
	if (WebSocketServer.IsValid())
	{
		WebSocketServer->Tick();
	}

	const double Now = FPlatformTime::Seconds();
	const double Interval = 1.0 / FMath::Clamp(CVarWebDebugRate.GetValueOnGameThread(), 1, 60);
	if (Now - LastSendSeconds < Interval)
	{
		return true;
	}
	LastSendSeconds = Now;

	// 게이지 스냅샷 — 값이 변한 것만 남긴다
	const float T = GetCombatTime();
	for (TPair<FString, FGaugeSample>& Pair : GaugeSamples)
	{
		if (!Pair.Value.bDirty)
		{
			continue;
		}
		Pair.Value.bDirty = false;

		FString Src, Key;
		Pair.Key.Split(TEXT(":"), &Src, &Key);

		FACWebDebugEvent Event;
		Event.T = T;
		Event.Type = EACWebDebugEventType::Gauge;
		Event.Src = Src == TEXT("player") ? EACWebDebugSource::Player : EACWebDebugSource::Boss;
		Event.Key = Key;
		Event.Norm = Pair.Value.Norm;
		Event.bHasNorm = true;
		Event.Raw = Pair.Value.Raw;
		Event.bHasRaw = true;
		PushEvent(MoveTemp(Event));
	}

	if (Pending.Num() > 0 && Clients.Num() > 0)
	{
		Broadcast(MakeEventsJson(Pending));
	}
	Pending.Reset();

	return true;
}

/* ────────────────────────────── 기록 API ────────────────────────────── */

void UACWebDebugSubsystem::PushEvent(FACWebDebugEvent&& Event)
{
	Pending.Add(Event);
	RingBuffer.Add(MoveTemp(Event));
}

void UACWebDebugSubsystem::BeginCombat(const FGameplayTag& BossId, int32 Attempt, const FGameplayTag& Weapon, int32 RunSeed)
{
	CombatStartSeconds = FPlatformTime::Seconds();
	RingBuffer.Reset();
	Pending.Reset();
	GaugeSamples.Reset();

	Session.BossId = BossId.IsValid() ? BossId.ToString() : FString();
	Session.Attempt = Attempt;
	Session.Weapon = Weapon.IsValid() ? Weapon.ToString() : FString();
	Session.RunSeed = RunSeed;
	Session.StartedAtUtc = FDateTime::UtcNow().ToIso8601();

	if (bServerRunning)
	{
		Broadcast(MakeSessionJson());
	}
}

void UACWebDebugSubsystem::RecordGauge(const AActor* Owner, const TCHAR* Key, float Current, float Max)
{
	if (!bServerRunning || !Owner)
	{
		return;
	}

	bool bTracked = false;
	for (const TCHAR* Tracked : TrackedGauges)
	{
		if (FCString::Strcmp(Tracked, Key) == 0)
		{
			bTracked = true;
			break;
		}
	}
	if (!bTracked)
	{
		return;
	}

	const float Norm = Max > 0.f ? FMath::Clamp(Current / Max, 0.f, 1.f) : 0.f;
	const FString Id = FString::Printf(TEXT("%s:%s"), SourceToString(ResolveSource(Owner)), Key);

	FGaugeSample& Sample = GaugeSamples.FindOrAdd(Id);
	// 소수 3자리에서 같은 값이면 스냅샷을 남기지 않는다 (같은 값 반복 금지)
	if (FMath::IsNearlyEqual(Sample.Norm, Norm, 0.0005f))
	{
		return;
	}
	Sample.Norm = Norm;
	Sample.Raw = Current;
	Sample.bDirty = true;
}

void UACWebDebugSubsystem::RecordAbility(const AActor* Owner, const FString& AbilityName, EACWebDebugPhase Phase)
{
	if (!bServerRunning || !Owner)
	{
		return;
	}
	FACWebDebugEvent Event;
	Event.T = GetCombatTime();
	Event.Type = EACWebDebugEventType::Ability;
	Event.Src = ResolveSource(Owner);
	Event.Key = AbilityName;
	Event.Phase = Phase;
	PushEvent(MoveTemp(Event));
}

void UACWebDebugSubsystem::RecordTag(const AActor* Owner, const FGameplayTag& Tag, EACWebDebugPhase Phase)
{
	if (!bServerRunning || !Owner || !Tag.IsValid())
	{
		return;
	}
	FACWebDebugEvent Event;
	Event.T = GetCombatTime();
	Event.Type = EACWebDebugEventType::Tag;
	Event.Src = ResolveSource(Owner);
	Event.Key = Tag.ToString();
	Event.Phase = Phase;
	PushEvent(MoveTemp(Event));
}

void UACWebDebugSubsystem::RecordHit(const AActor* Target, float Damage, const FACWebDebugHitMeta& Meta)
{
	if (!bServerRunning || !Target)
	{
		return;
	}
	FACWebDebugEvent Event;
	Event.T = GetCombatTime();
	Event.Type = EACWebDebugEventType::Hit;
	Event.Src = ResolveSource(Target);
	Event.Key = TEXT("damage_taken");
	Event.Raw = Damage;
	Event.bHasRaw = true;
	Event.Meta = MakeShared<FACWebDebugHitMeta>(Meta);
	PushEvent(MoveTemp(Event));
}

void UACWebDebugSubsystem::RecordMarker(const AActor* Owner, const TCHAR* MarkerKey)
{
	if (!bServerRunning || !Owner)
	{
		return;
	}
	FACWebDebugEvent Event;
	Event.T = GetCombatTime();
	Event.Type = EACWebDebugEventType::Marker;
	Event.Src = ResolveSource(Owner);
	Event.Key = MarkerKey;
	PushEvent(MoveTemp(Event));
}

void UACWebDebugSubsystem::RegisterAbilitySystem(UAbilitySystemComponent* ASC)
{
	if (!ASC || RegisteredSystems.Contains(ASC))
	{
		return;
	}
	RegisteredSystems.Add(ASC);

	TWeakObjectPtr<UAbilitySystemComponent> WeakASC(ASC);
	for (const FGameplayTag& Tag : GetTrackedTags())
	{
		ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
			.AddWeakLambda(this, [this, WeakASC](const FGameplayTag CallbackTag, int32 NewCount)
			{
				UAbilitySystemComponent* Source = WeakASC.Get();
				AActor* Avatar = Source ? Source->GetAvatarActor() : nullptr;
				if (!Avatar)
				{
					return;
				}
				const EACWebDebugPhase Phase = NewCount > 0 ? EACWebDebugPhase::Begin : EACWebDebugPhase::End;
				RecordTag(Avatar, CallbackTag, Phase);

				// 타이밍 표본 수집은 런 로그 쪽 몫이다 — 태그 구독은 여기 한 곳만 둔다
				if (UACRunLogSubsystem* RunLog = UACRunLogSubsystem::Get(Avatar))
				{
					RunLog->OnTrackedTagChanged(Avatar, CallbackTag, NewCount > 0);
				}
			});
	}
}

/* ────────────────────────────── 스냅샷 ────────────────────────────── */

void UACWebDebugSubsystem::SaveDeathSnapshot()
{
	if (RingBuffer.Num() <= 0)
	{
		return;
	}

	FString Json;
	const TSharedRef<FCondensedWriter> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("schema"), ACWebDebug::SchemaVersion);
	Writer->WriteObjectStart(TEXT("session"));
	Writer->WriteValue(TEXT("kind"), TEXT("session"));
	Writer->WriteValue(TEXT("schema"), ACWebDebug::SchemaVersion);
	Writer->WriteValue(TEXT("sessionId"), Session.SessionId);
	Writer->WriteValue(TEXT("runSeed"), Session.RunSeed);
	Writer->WriteValue(TEXT("bossId"), Session.BossId);
	Writer->WriteValue(TEXT("attempt"), Session.Attempt);
	Writer->WriteValue(TEXT("weapon"), Session.Weapon);
	Writer->WriteValue(TEXT("buildConfig"), Session.BuildConfig);
	Writer->WriteValue(TEXT("startedAtUtc"), Session.StartedAtUtc);
	Writer->WriteObjectEnd();
	Writer->WriteArrayStart(TEXT("events"));
	for (int32 Index = 0; Index < RingBuffer.Num(); ++Index)
	{
		WriteEventJson(Writer, RingBuffer[Index]);
	}
	Writer->WriteArrayEnd();
	Writer->WriteObjectEnd();
	Writer->Close();

	const FString FileName = FString::Printf(TEXT("death-%s.json"), *FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S")));
	const FString FullPath = FPaths::Combine(GetWebRootDir(), TEXT("snapshots"), FileName);
	if (FFileHelper::SaveStringToFile(Json, *FullPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogACWebDebug, Display, TEXT("사망 스냅샷 저장 — %s"), *FullPath);
	}
	else
	{
		UE_LOG(LogACWebDebug, Warning, TEXT("사망 스냅샷 저장 실패 — %s"), *FullPath);
	}
}

/* ────────────────────────────── 직렬화 ────────────────────────────── */

FString UACWebDebugSubsystem::MakeSessionJson() const
{
	FString Json;
	const TSharedRef<FCondensedWriter> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("kind"), TEXT("session"));
	Writer->WriteValue(TEXT("schema"), ACWebDebug::SchemaVersion);
	Writer->WriteValue(TEXT("sessionId"), Session.SessionId);
	Writer->WriteValue(TEXT("runSeed"), Session.RunSeed);
	Writer->WriteValue(TEXT("bossId"), Session.BossId);
	Writer->WriteValue(TEXT("attempt"), Session.Attempt);
	Writer->WriteValue(TEXT("weapon"), Session.Weapon);
	Writer->WriteValue(TEXT("buildConfig"), Session.BuildConfig);
	Writer->WriteValue(TEXT("startedAtUtc"), Session.StartedAtUtc);
	Writer->WriteObjectEnd();
	Writer->Close();
	return Json;
}

FString UACWebDebugSubsystem::MakeEventsJson(const TArray<FACWebDebugEvent>& Events)
{
	FString Json;
	const TSharedRef<FCondensedWriter> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("kind"), TEXT("events"));
	Writer->WriteArrayStart(TEXT("events"));
	for (const FACWebDebugEvent& Event : Events)
	{
		WriteEventJson(Writer, Event);
	}
	Writer->WriteArrayEnd();
	Writer->WriteObjectEnd();
	Writer->Close();
	return Json;
}

FString UACWebDebugSubsystem::MakeRingBufferJson() const
{
	FString Json;
	const TSharedRef<FCondensedWriter> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("kind"), TEXT("events"));
	Writer->WriteArrayStart(TEXT("events"));
	for (int32 Index = 0; Index < RingBuffer.Num(); ++Index)
	{
		WriteEventJson(Writer, RingBuffer[Index]);
	}
	Writer->WriteArrayEnd();
	Writer->WriteObjectEnd();
	Writer->Close();
	return Json;
}

/* ────────────────────────────── WebSocket ────────────────────────────── */

void UACWebDebugSubsystem::OnClientConnected(INetworkingWebSocket* Socket)
{
	if (!Socket)
	{
		return;
	}
	Clients.Add(Socket);

	FWebSocketInfoCallBack ClosedCallback;
	ClosedCallback.BindWeakLambda(this, [this, Socket]()
	{
		Clients.Remove(Socket);
	});
	Socket->SetSocketClosedCallBack(ClosedCallback);

	// 늦게 붙어도 과거가 보이도록 세션 헤더와 링 버퍼 전체를 먼저 보낸다
	SendToSocket(Socket, MakeSessionJson());
	if (RingBuffer.Num() > 0)
	{
		SendToSocket(Socket, MakeRingBufferJson());
	}

	UE_LOG(LogACWebDebug, Display, TEXT("대시보드 접속 — %s (총 %d)"), *Socket->RemoteEndPoint(true), Clients.Num());
}

void UACWebDebugSubsystem::SendToSocket(INetworkingWebSocket* Socket, const FString& Payload)
{
	if (!Socket)
	{
		return;
	}
	// lws 서버는 항상 바이너리 프레임으로 보낸다. 클라이언트(ws.ts)가 ArrayBuffer 를 UTF-8 로 디코딩한다.
	const FTCHARToUTF8 Utf8(*Payload);
	Socket->Send(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length(), /*bPrependSize*/ false);
}

void UACWebDebugSubsystem::Broadcast(const FString& Payload)
{
	for (INetworkingWebSocket* Socket : Clients)
	{
		SendToSocket(Socket, Payload);
	}
}

/* ────────────────────────────── HTTP ────────────────────────────── */

void UACWebDebugSubsystem::BindRoutes()
{
	// 구체적인 경로가 먼저 매칭되므로 "/" 는 나머지 전부를 받는 정적 파일 핸들러가 된다
	RouteHandles.Add(Router->BindRoute(
		FHttpPath(TEXT("/api/runs")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateUObject(this, &UACWebDebugSubsystem::HandleRunListRequest)));

	RouteHandles.Add(Router->BindRoute(
		FHttpPath(TEXT("/api/runs/:id")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateUObject(this, &UACWebDebugSubsystem::HandleRunGetRequest)));

	// 루트 "/" 는 BindRoute 로 못 잡는다(FHttpRouter::BindRoute 의 check(IsValidPath) 가 루트를 거부한다).
	// 프리프로세서는 모든 요청보다 먼저 돌고 false 를 돌려주면 위 라우트로 넘어가므로 catch-all 로 알맞다.
	StaticPreprocessorHandle = Router->RegisterRequestPreprocessor(
		FHttpRequestHandler::CreateUObject(this, &UACWebDebugSubsystem::HandleStaticRequest));
}

bool UACWebDebugSubsystem::HandleStaticRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	// 프리프로세서라 모든 요청이 여기부터 들어온다. API 와 GET 이외는 정규 라우트에 넘긴다.
	if (Request.Verb != EHttpServerRequestVerbs::VERB_GET)
	{
		return false;
	}
	if (Request.RelativePath.GetPath().StartsWith(TEXT("/api/")) || Request.RelativePath.GetPath() == TEXT("/api"))
	{
		return false;
	}

	FString RelativePath = Request.RelativePath.GetPath();
	RelativePath.RemoveFromStart(TEXT("/"));

	// 상위 디렉터리 탈출 차단 — Saved/WebDebug 밖은 절대 서빙하지 않는다
	if (RelativePath.Contains(TEXT("..")))
	{
		OnComplete(FHttpServerResponse::Error(EHttpServerResponseCodes::BadRequest, TEXT("BadPath")));
		return true;
	}

	const FString Root = GetWebRootDir();
	FString FullPath = RelativePath.IsEmpty() ? FPaths::Combine(Root, TEXT("index.html")) : FPaths::Combine(Root, RelativePath);

	// 파일이 없고 확장자도 없으면 SPA 라우트(/timeline, /stats)이므로 index.html 을 돌려준다
	if (!FPaths::FileExists(FullPath) && FPaths::GetExtension(FullPath).IsEmpty())
	{
		FullPath = FPaths::Combine(Root, TEXT("index.html"));
	}

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *FullPath))
	{
		OnComplete(FHttpServerResponse::Error(
			EHttpServerResponseCodes::NotFound,
			TEXT("NotFound"),
			FString::Printf(TEXT("%s 를 찾을 수 없습니다. Tools/WebDebug 에서 npm run build 를 먼저 실행하세요."), *RelativePath)));
		return true;
	}

	OnComplete(FHttpServerResponse::Create(MoveTemp(Bytes), GetContentType(FPaths::GetExtension(FullPath).ToLower())));
	return true;
}

bool UACWebDebugSubsystem::HandleRunListRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	const FString RunLogDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RunLogs"));
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(RunLogDir, TEXT("*.json")), true, false);
	Files.Sort();

	FString Json;
	const TSharedRef<FCondensedWriter> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	Writer->WriteArrayStart();
	for (const FString& File : Files)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("id"), FPaths::GetBaseFilename(File));
		Writer->WriteValue(TEXT("file"), FString::Printf(TEXT("runs/%s"), *File));
		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();
	Writer->Close();

	OnComplete(FHttpServerResponse::Create(Json, TEXT("application/json; charset=utf-8")));
	return true;
}

bool UACWebDebugSubsystem::HandleRunGetRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	const FString* IdParam = Request.PathParams.Find(TEXT("id"));
	if (!IdParam)
	{
		OnComplete(FHttpServerResponse::Error(EHttpServerResponseCodes::BadRequest));
		return true;
	}

	// 파일명으로 그대로 쓰이므로 안전한 문자만 허용한다
	FString Id = *IdParam;
	Id.RemoveFromEnd(TEXT(".json"));
	for (const TCHAR Char : Id)
	{
		if (!FChar::IsAlnum(Char) && Char != TEXT('-') && Char != TEXT('_'))
		{
			OnComplete(FHttpServerResponse::Error(EHttpServerResponseCodes::BadRequest));
			return true;
		}
	}

	const FString FullPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RunLogs"), Id + TEXT(".json"));
	FString Contents;
	if (!FFileHelper::LoadFileToString(Contents, *FullPath))
	{
		OnComplete(FHttpServerResponse::Error(EHttpServerResponseCodes::NotFound));
		return true;
	}

	OnComplete(FHttpServerResponse::Create(Contents, TEXT("application/json; charset=utf-8")));
	return true;
}

#else // !AC_WEB_DEBUG

void UACWebDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UACWebDebugSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

#endif // AC_WEB_DEBUG
