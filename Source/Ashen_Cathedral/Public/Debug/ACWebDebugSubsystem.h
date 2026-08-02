// 웹 디버그 대시보드의 게임 측 서버 — HTTP 정적 서빙 + WebSocket 스트리밍 + 링 버퍼 관리.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "GameplayTagContainer.h"
#include "Debug/ACWebDebugTypes.h"

#if AC_WEB_DEBUG
	// FHttpResultCallback 은 TFunction<> 의, FHttpRouteHandle 은 TSharedPtr<> 의 typedef 다.
	// 전방 선언으로 대체할 수 없으므로 HTTPServer 헤더를 직접 들인다(Shipping 에서는 통째로 빠진다).
	#include "HttpResultCallback.h"
	#include "HttpServerRequest.h"
#endif

#include "ACWebDebugSubsystem.generated.h"

class IHttpRouter;
class IWebSocketServer;
class INetworkingWebSocket;
class UAbilitySystemComponent;
struct FHttpRouteHandleInternal;

/**
 * @brief 전투 기록을 모아 브라우저로 흘려보내는 디버그 서버.
 *
 * Shipping 빌드에서는 AC_WEB_DEBUG=0 이므로 이 클래스의 본문이 통째로 비어 있고,
 * 기록 훅들도 전부 컴파일되지 않는다.
 *
 * 서버는 기본적으로 꺼져 있으며 콘솔 명령으로 켠다.
 *   ac.WebDebug 1        서버 on/off
 *   ac.WebDebug.Port     HTTP 포트 (기본 8091, WebSocket 은 +1)
 *   ac.WebDebug.Rate     전송 Hz (기본 15)
 *
 * 바인딩 주소는 127.0.0.1 로 고정되어 있다. 외부 인터페이스에는 절대 바인딩하지 않는다.
 */
UCLASS()
class ASHEN_CATHEDRAL_API UACWebDebugSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#if AC_WEB_DEBUG
	/** 월드 어디서든 서브시스템을 얻는 헬퍼. 서버가 꺼져 있어도 인스턴스는 유효하다. */
	static UACWebDebugSubsystem* Get(const UObject* WorldContextObject);

	/** 액터가 플레이어인지 보스인지 판정한다. 플레이어 폰이 아니면 전부 boss 로 본다. */
	static EACWebDebugSource ResolveSource(const AActor* Actor);

	/**
	 * @brief 새 전투의 시간 원점을 잡고 세션 헤더를 갱신·전송한다.
	 *
	 * @param BossId 보스 식별 태그 (MetaProgression.BossID.*)
	 * @param Attempt 이 보스에 대한 시도 회차
	 * @param Weapon 플레이어 무기 태그 (Player.Weapon.*)
	 * @param RunSeed 런 시드
	 */
	void BeginCombat(const FGameplayTag& BossId, int32 Attempt, const FGameplayTag& Weapon, int32 RunSeed);

	/**
	 * @brief ASC 의 상태 태그 변화를 구독해 tag begin/end 구간을 기록하기 시작한다.
	 * 같은 ASC 를 여러 번 넘겨도 중복 구독되지 않는다.
	 */
	void RegisterAbilitySystem(UAbilitySystemComponent* ASC);

	#pragma region 기록 API. 기존 게임플레이 코드가 한 줄씩 호출한다
	/**
	 * @brief 게이지 현재값을 갱신한다. 실제 전송은 15Hz 틱에서, 값이 변했을 때만 일어난다.
	 * @param Owner 게이지 소유 액터
	 * @param Key Health / Stamina / Posture / GuardGauge / BurnGauge 중 하나
	 * @param Current 현재 절대값
	 * @param Max 대응 최대값. 0 이면 norm 은 0 으로 보낸다
	 */
	void RecordGauge(const AActor* Owner, const TCHAR* Key, float Current, float Max);

	/** 어빌리티 활성/종료 구간을 기록한다 */
	void RecordAbility(const AActor* Owner, const FString& AbilityName, EACWebDebugPhase Phase);

	/** 태그 부여/제거 구간을 기록한다 */
	void RecordTag(const AActor* Owner, const FGameplayTag& Tag, EACWebDebugPhase Phase);

	/** 피격 1건을 기록한다 */
	void RecordHit(const AActor* Target, float Damage, const FACWebDebugHitMeta& Meta);

	/** 마커 1건을 기록한다. Key 는 §3.2 의 허용값만 쓴다 */
	void RecordMarker(const AActor* Owner, const TCHAR* MarkerKey);
	#pragma endregion

	/** 링 버퍼 전체를 Saved/WebDebug/snapshots/death-{timestamp}.json 으로 저장한다 */
	void SaveDeathSnapshot();

	/** 전투 시작 기준 경과 초 */
	float GetCombatTime() const;

	bool IsServerRunning() const { return bServerRunning; }

private:
	void StartServer();
	void StopServer();
	bool Tick(float DeltaSeconds);

	void PushEvent(FACWebDebugEvent&& Event);

	#pragma region HTTP
	void BindRoutes();

	/**
	 * @brief Saved/WebDebug 아래의 파일을 서빙하고, 파일이 없으면 index.html 로 폴백한다(SPA 라우팅).
	 *
	 * 루트 경로 "/" 는 BindRoute 로 바인딩할 수 없으므로(FHttpPath::IsValidPath 가 루트를 거부한다)
	 * 이 핸들러는 모든 요청보다 먼저 도는 RequestPreprocessor 로 등록한다.
	 * @return /api/ 로 시작하는 요청이면 false 를 돌려 정규 라우트로 넘긴다
	 */
	bool HandleStaticRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleRunListRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleRunGetRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	#pragma endregion

	#pragma region WebSocket
	void OnClientConnected(INetworkingWebSocket* Socket);
	void SendToSocket(INetworkingWebSocket* Socket, const FString& Payload);
	void Broadcast(const FString& Payload);
	#pragma endregion

	#pragma region 직렬화
	FString MakeSessionJson() const;
	/** 이벤트 배열을 events 배치 JSON 한 덩어리로 만든다 */
	static FString MakeEventsJson(const TArray<FACWebDebugEvent>& Events);
	/** 링 버퍼 전체를 events 배치 JSON 으로 만든다 (늦게 붙은 클라이언트에게 과거를 보낸다) */
	FString MakeRingBufferJson() const;
	#pragma endregion

	FACWebDebugRingBuffer RingBuffer;
	/** 아직 전송하지 않은 이벤트 — 15Hz 틱에서 한 번에 묶어 보낸다 */
	TArray<FACWebDebugEvent> Pending;

	FACWebDebugSession Session;

	/** 게이지 최신값 캐시. 키는 "player:Health" 형태이며, 값이 변했을 때만 스냅샷을 남긴다 */
	struct FGaugeSample
	{
		float Norm = -1.f;
		float Raw = 0.f;
		bool bDirty = false;
	};
	TMap<FString, FGaugeSample> GaugeSamples;

	TSharedPtr<IHttpRouter> Router;
	TArray<TSharedPtr<const FHttpRouteHandleInternal>> RouteHandles;
	/** 정적 파일 catch-all 프리프로세서 핸들 — 서버를 끌 때 해제한다 */
	FDelegateHandle StaticPreprocessorHandle;
	// TUniquePtr 은 소멸자가 완전 타입을 요구해 UHT 생성 코드에서 터진다.
	// TSharedPtr 은 삭제자를 생성 시점에 타입 소거하므로 전방 선언만으로 들고 있을 수 있다.
	TSharedPtr<IWebSocketServer> WebSocketServer;
	TArray<INetworkingWebSocket*> Clients;

	FTSTicker::FDelegateHandle TickHandle;

	/** 이미 태그 구독을 건 ASC — 중복 구독 방지 */
	TSet<TWeakObjectPtr<UAbilitySystemComponent>> RegisteredSystems;

	double CombatStartSeconds = 0.0;
	double LastSendSeconds = 0.0;
	bool bServerRunning = false;
#endif // AC_WEB_DEBUG
};
