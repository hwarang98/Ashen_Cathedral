# StateTree 런타임 가이드 (UE 5.7)

이 문서는 UE 5.7 StateTree 플러그인의 **런타임 동작**을 엔진 소스 기준으로 정리한 것입니다.
에디터 모듈(`StateTreeEditorModule`)은 다루지 않습니다.

> **이 문서는 소스의 대체물이 아니라 지도입니다.**
> 모든 항목에 실제 엔진 파일 경로와 줄 번호를 붙여두었습니다.
> 정확한 동작이 필요하면 항상 원본을 확인하세요. 엔진 버전이 올라가면 줄 번호는 어긋날 수 있습니다.

**엔진 소스 루트**
```
C:\Program Files\Epic Games\UE_5.7\Engine\Plugins\Runtime\StateTree\Source\StateTreeModule
C:\Program Files\Epic Games\UE_5.7\Engine\Plugins\Runtime\GameplayStateTree\Source\GameplayStateTreeModule
```

---

## 1. 모듈 구성

| 모듈 | 역할 | 런타임 포함 |
|------|------|:---:|
| `StateTreeModule` | 실행 엔진 전체 (컨텍스트, 노드, 바인딩, 이벤트) | O |
| `StateTreeDeveloper` | 컴파일 지원 | X |
| `StateTreeEditorModule` | 에디터 그래프/컴파일러 | X |
| `GameplayStateTreeModule` | 게임플레이 연동 (컴포넌트, AI 태스크, BT 브리지) | O |

가장 중요한 파일 두 개:

- `Public/StateTreeExecutionContext.h` (80KB) — 런타임 공개 API 전부
- `Private/StateTreeExecutionContext.cpp` (335KB) — 실행 로직 전부

---

## 2. 노드 4종

StateTree의 모든 노드는 `FStateTreeNodeBase`를 상속하는 **USTRUCT**입니다 (UObject 아님).
BP로 만들 때만 UObject 래퍼가 씌워집니다.

| 노드 | 베이스 | 파일 | 역할 |
|------|--------|------|------|
| Task | `FStateTreeTaskBase` | `Public/StateTreeTaskBase.h:19` | 활성 상태에서 실행되는 로직 |
| Condition | `FStateTreeConditionBase` | `Public/StateTreeConditionBase.h:21` | 상태 진입 가능 여부 판정 |
| Evaluator | `FStateTreeEvaluatorBase` | `Public/StateTreeEvaluatorBase.h:18` | 의사결정용 데이터 계산·노출 |
| Consideration | `Public/Considerations/StateTreeCommonConsiderations.h` | | 유틸리티 점수 계산 |

### 2.1 Task 인터페이스

`Public/StateTreeTaskBase.h:45-89`

```cpp
virtual EStateTreeRunStatus EnterState(Context, Transition) const;   // 상태 진입
virtual void              ExitState(Context, Transition) const;      // 상태 이탈
virtual void              StateCompleted(Context, Status, States) const;
virtual EStateTreeRunStatus Tick(Context, DeltaTime) const;
virtual void              TriggerTransitions(Context) const;         // bShouldAffectTransitions 필요
```

**주의: Task는 `const` 메서드입니다.** 상태를 저장하려면 인스턴스 데이터 구조체에 넣어야 합니다.
노드 자체는 에셋에 공유되는 불변 데이터입니다.

### 2.2 Task 동작 플래그

`Public/StateTreeTaskBase.h:115-142`. 성능에 직결되므로 중요합니다.

| 플래그 | 기본값 | 의미 |
|--------|:---:|------|
| `bShouldStateChangeOnReselect` | true | 이미 활성이던 상태를 재선택해도 EnterState/ExitState를 다시 받음 |
| `bShouldCallTick` | true | Tick() 호출. **false면 프로퍼티 복사도 안 일어남** |
| `bShouldCallTickOnlyOnEvents` | false | 이벤트가 있을 때만 Tick. `bShouldCallTick`이 true면 무의미 |
| `bShouldCopyBoundPropertiesOnTick` | true | Tick 전 바인딩 프로퍼티 복사 |
| `bShouldCopyBoundPropertiesOnExitState` | true | ExitState 전 바인딩 프로퍼티 복사 |
| `bShouldAffectTransitions` | false | TriggerTransitions() 호출 |
| `bConsideredForCompletion` | true | false면 백그라운드 실행 — 상태 완료 판정에 관여하지 않음 |

`bShouldStateChangeOnReselect`는 엔진 주석에 설계 의도가 명시돼 있습니다
(`StateTreeTaskBase.h:110-114`): **애니메이션 재생 같은 "액션형" 태스크는 true**,
자식 상태에서 계속 유지돼야 하는 **"자원 점유형" 태스크는 false**.

---

## 3. 실행 모델

### 3.1 Tick 흐름

`Private/StateTreeExecutionContext.cpp:1698`

```
Tick(DeltaTime)
 ├─ TickPrelude()                  :1640  유효성 검사 + 외부 데이터 수집 + 페이즈 진입
 ├─ TickUpdateTasksInternal()      :1753  Evaluator/글로벌 태스크 → 활성 상태 태스크 Tick
 ├─ TickTriggerTransitionsInternal():1845  전이 판정 및 적용
 └─ TickPostlude()                 :1677  지연된 Stop 처리
```

세 단계는 개별 공개 API로도 노출됩니다 (`TickUpdateTasks`, `TickTriggerTransitions`).
틱 분할이 필요할 때 씁니다.

### 3.2 전이 루프 — MaxIterations = 5

`Private/StateTreeExecutionContext.cpp:1862`

```cpp
static constexpr int32 MaxIterations = 5;
```

한 틱 안에서 전이를 **최대 5번까지 반복**합니다.
엔진 주석(`:1860-1861`)의 설명: EnterState가 실패했을 때 즉시 다른 상태를 찾게 하여,
이벤트 기반 트리가 다음 틱/이벤트를 기다리지 않아도 되게 하기 위함입니다.

실행 중인 상태를 찾으면 즉시 루프를 빠져나옵니다 (`:1911`).
**5회 안에 안정된 상태를 못 찾으면 그 틱은 그대로 종료됩니다** — 무한 전이 루프를 짜면
경고 없이 조용히 어긋나므로, 진입 조건이 서로 배타적인지 확인해야 합니다.

### 3.3 전이 적용 순서

`Private/StateTreeExecutionContext.cpp:1867-1907`

```
TriggerTransitions() 성공
 → BeginApplyTransition()
 → ExitState()                     이전 상태 태스크 역순 정리
 → [TargetState가 완료 상태면]     TreeRunStatus 확정 후 종료
 → EnterState()                    새 상태 태스크 진입
 → LastTickStatus != Running 이면 StateCompleted()
```

---

## 4. 전이 (Transition)

### 4.1 트리거 종류

`Public/StateTreeTypes.h:183-207` — **비트 플래그입니다**

| 값 | 비트 | 의미 |
|----|:---:|------|
| `OnStateCompleted` | 0x3 | 성공 또는 실패 (아래 둘의 합) |
| `OnStateSucceeded` | 0x1 | 상태 성공 시 |
| `OnStateFailed` | 0x2 | 상태 실패 시 |
| `OnTick` | 0x4 | 매 틱 |
| `OnEvent` | 0x8 | 특정 이벤트 수신 시 |
| `OnDelegate` | 0x10 | 델리게이트 브로드캐스트 시 |

### 4.2 우선순위

`Public/StateTreeTypes.h:212-230`

```
Low < Normal < Medium < High < Critical
```

**동시에 여러 전이가 발동하면 "가장 높은 우선순위 중 첫 번째"가 선택됩니다**
(`:210` 주석). 같은 우선순위면 트리 정의 순서가 승자를 결정합니다.
보스 AI에서 "피격 반응이 공격을 끊어야 한다" 같은 규칙은 우선순위로 표현하세요.

### 4.3 선택 실패 시 폴백

`Public/StateTreeTypes.h:550-557`

| 값 | 동작 |
|----|------|
| `None` | 폴백 없음 — 전이 실패 |
| `NextSelectableSibling` | 다음 선택 가능한 형제 상태를 찾아 선택 |

---

## 5. 상태 선택 동작

`Public/StateTreeTypes.h:152-178`

| 값 | 동작 |
|----|------|
| `None` | 직접 선택 불가 |
| `TryEnterState` | 자식이 있어도 이 상태를 선택 |
| `TrySelectChildrenInOrder` | 자식을 정의 순서대로 시도 (기본) |
| `TrySelectChildrenAtRandom` | 자식 순서를 섞어서 시도 |
| `TrySelectChildrenWithHighestUtility` | 유틸리티 점수 최고인 자식. 동점이면 순서대로 |
| `TrySelectChildrenAtRandomWeightedByUtility` | 정규화된 유틸리티 점수를 확률로 사용 |
| `TryFollowTransitions` | 자식 선택 대신 전이를 시도 |

**보스 패턴 랜덤화에는 뒤쪽 세 개가 직접 쓸모 있습니다.**
BT의 `BTDecorator_ComputeChance` + 랜덤 쿨다운 조합을 StateTree에서는
`TrySelectChildrenAtRandomWeightedByUtility` + Consideration으로 표현합니다.

### 5.1 상태 타입

`Public/StateTreeTypes.h:133-149`

| 타입 | 의미 |
|------|------|
| `State` | 태스크 + 자식 상태 |
| `Group` | 자식 상태만 |
| `Linked` | 같은 트리 내 다른 상태로 연결 |
| `LinkedAsset` | 다른 StateTree 에셋의 Root로 연결 |
| `Subtree` | 링크 대상이 될 수 있는 서브트리 |

`LinkedAsset`은 런타임에 `LinkedStateTreeOverrides`로 교체 가능합니다
(`StateTreeComponent.h:94-105`). 보스 페이즈별로 트리 에셋을 갈아끼우는 데 쓸 수 있습니다.

---

## 6. 이벤트

`Public/StateTreeEvents.h`

### 6.1 구조

```cpp
struct FStateTreeEvent            // :28
{
    FGameplayTag     Tag;         // 이벤트 식별
    FInstancedStruct Payload;     // 선택적 페이로드
    FName            Origin;      // 선택적 발신처
};
```

### 6.2 큐 제한 — MaxActiveEvents = 64

`Public/StateTreeEvents.h:155`

```cpp
static constexpr int32 MaxActiveEvents = 64;
```

**한 번에 버퍼링 가능한 이벤트는 64개입니다.** 초과하면 `SendEvent`가 false를 반환합니다.
매 틱 이벤트를 쏘는 구조를 만들면 이 한계에 닿습니다 — 이벤트는 "상태 변화 알림"으로만 쓰고,
지속적인 값 전달은 Evaluator나 프로퍼티 바인딩을 쓰세요.

이벤트는 전이 처리 단계에서 소비되며, `ConsumeEvent`로 명시적 제거도 가능합니다
(`StateTreeExecutionContext.cpp:2005`).

---

## 7. 게임플레이 연동

### 7.1 UStateTreeComponent

`GameplayStateTreeModule/Public/Components/StateTreeComponent.h:39`

`UBrainComponent`를 상속합니다. 주요 API:

| 함수 | 용도 |
|------|------|
| `SetStateTree(UStateTree*)` | 트리 교체 — **실행 중이면 무시됨** (`:77-81`) |
| `SendStateTreeEvent(Tag, Payload, Origin)` | 이벤트 발송 (`:117-120`) |
| `GetStateTreeRunStatus()` | 현재 실행 상태 (`:124`) |
| `OnStateTreeRunStatusChanged` | 실행 상태 변화 델리게이트 (`:128`) |
| `AddLinkedStateTreeOverrides(Tag, Ref)` | 링크된 트리 런타임 교체 (`:101`) |
| `StartLogic` / `StopLogic` / `PauseLogic` | BrainComponent 인터페이스 (`:54-59`) |

`bStartLogicAutomatically`(기본 true, `:187`)가 BeginPlay에서 자동 시작을 결정합니다.

### 7.2 UStateTreeAIComponent

`Public/Components/StateTreeAIComponent.h:16`

`UStateTreeComponent`를 상속하고 **스키마만 교체**합니다.
`StateTreeAIComponentSchema`는 AIController 접근을 보장합니다.
AIController에 붙일 때는 항상 이쪽을 씁니다.

### 7.3 이 프로젝트의 연결 방식

`Source/Ashen_Cathedral/Private/Controllers/ACStateTreeController.cpp`

`AACStateTreeController`는 **GAS/Perception 신호를 StateTree 이벤트로 변환하는 어댑터** 역할만 합니다.
판단은 전부 트리 안에 있습니다.

| 소스 신호 | 변환된 StateTree 이벤트 | 위치 |
|-----------|------------------------|------|
| Perception 감지 성공 | `Enemy.StateTree.Event.TargetAcquired` | `:109` |
| Perception 감지 상실 | `Enemy.StateTree.Event.TargetLost` | `:116` |
| `Enemy.State.PressureReady` 태그 추가 | `Enemy.StateTree.Event.PressureReady` | `:125` |
| `Shared.Event.Combat.IncomingAttack` | `Enemy.StateTree.Event.IncomingAttack` + 페이로드 | `:152` |

**BT 버전과의 설계 차이** (`ACStateTreeController.h:80` 주석):
BT는 Blackboard에 값을 걸어두고 만료 타이머로 지웠지만, StateTree는 이벤트만 보내고
반응 지속 시간은 상태가 스스로 관리합니다. 타이머가 사라집니다.

**주의 사항** (`ACStateTreeController.cpp:61-66`):
`UStateTreeComponent`는 `UBrainComponent` 초기화를 건너뛰어 `AAIController::BrainComponent`에
등록되지 않습니다. 따라서 **언포제스 시 자동 정지하지 않으므로 `StopLogic()`을 직접 호출해야 합니다.**

### 7.4 BT와의 브리지

BT에서 StateTree를 서브트리로 실행할 수 있습니다:

- `Public/BehaviorTree/Tasks/BTTask_RunStateTree.h`
- `Public/BehaviorTree/Tasks/BTTask_RunDynamicStateTree.h`

기존 BT 자산을 유지하면서 일부만 StateTree로 옮길 때의 이행 경로입니다.

---

## 8. Blueprint 노드 작성

`Public/Blueprint/StateTreeTaskBlueprintBase.h:21`

### 8.1 오버라이드 가능한 이벤트

| BP 이벤트 | C++ | 비고 |
|-----------|-----|------|
| `EnterState` | `ReceiveLatentEnterState` (`:38`) | latent |
| `ExitState` | `ReceiveExitState` (`:45`) | |
| `StateCompleted` | `ReceiveStateCompleted` (`:54`) | |
| `Tick` | `ReceiveLatentTick` (`:68`) | latent |

### 8.2 완료 처리 — 반환값이 아니라 FinishTask

**이전 방식(반환값으로 상태 전달)은 deprecated 되었습니다** (`:70-76`).
현재는 `FinishTask(bSucceeded)` 노드를 호출합니다 (`:87`). 기본 상태는 Running입니다.

### 8.3 GameplayTask 사용 시 주의

엔진 주석(`:32-33`)이 명시합니다:

> GameplayTask 등 latent 액션은 EnterState에서 시작하되,
> GameplayTask의 수명이 StateTree 태스크에 묶인다면 **ExitState에서 수동으로 취소해야 합니다.**

Tick에서 latent 액션을 시작하면 태스크가 폭증한다는 경고도 함께 있습니다 (`:60-62`).

### 8.4 델리게이트

| 함수 | 용도 |
|------|------|
| `BroadcastDelegate(Dispatcher)` | 바인딩된 콜백과 전이를 발동 (`:91`) |
| `BindDelegate(Listener, Delegate)` | 콜백 등록 (`:99`) |
| `UnbindDelegate(Listener)` | 해제 (`:103`) |

Dispatcher와 Listener의 연결은 에디터에서 설정합니다.
`OnDelegate` 전이 트리거와 함께 쓰면 이벤트 큐를 거치지 않는 직접 통지가 됩니다.

---

## 9. 성능 관련 메모

1. **틱 스케줄링** — `FStateTreeComponentExecutionExtension::ScheduleNextTick`
   (`StateTreeComponent.h:30`)이 다음 틱 시점을 제어합니다. 컴포넌트는
   `ConditionalEnableTick` / `DisableTick`으로 틱을 껐다 켭니다 (`:162-163`).
   **모든 태스크가 틱을 필요로 하지 않으면 컴포넌트 자체가 틱을 멈춥니다.**
   `bShouldCallTick = false`가 실제로 프레임을 아낀다는 뜻입니다.

2. **프로퍼티 복사 비용** — 바인딩된 프로퍼티는 Tick/ExitState 직전에 복사됩니다.
   불필요하면 `bShouldCopyBoundPropertiesOnTick`을 끄세요.

3. **이벤트 큐 64개 제한** — 6.2절 참조.

4. **전이 반복 5회 제한** — 3.2절 참조.

---

## 10. 더 읽을 곳

| 주제 | 파일 |
|------|------|
| 실행 컨텍스트 전체 API | `Public/StateTreeExecutionContext.h` |
| 비동기 실행 | `Public/StateTreeAsyncExecutionContext.h` |
| 인스턴스 데이터 레이아웃 | `Public/StateTreeInstanceData.h` |
| 프로퍼티 바인딩 | `Public/StateTreePropertyBindings.h` |
| 병렬 트리 실행 | `Public/Tasks/StateTreeRunParallelStateTreeTask.h` |
| 내장 조건 | `Public/Conditions/StateTreeCommonConditions.h`, `StateTreeGameplayTagConditions.h` |
| 디버거 | `Public/Debugger/StateTreeDebugger.h` |
| **사용 예제** | `StateTreeTestSuite/` — 실제 동작을 확인하는 가장 빠른 방법 |

`StateTreeTestSuite`는 16개 파일 546KB로, 엔진이 스스로 검증하는 시나리오들입니다.
"이 조합이 되나?" 싶을 때 문서보다 여기를 먼저 보는 편이 빠릅니다.
