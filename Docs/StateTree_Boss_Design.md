# StateTree 보스 AI 설계 (Ordan)

BT_Dummy(보스1, Behavior Tree)를 StateTree로 옮기기 위한 구조 설계 문서입니다.
런타임 동작의 근거는 [StateTree_Runtime_Guide.md](StateTree_Runtime_Guide.md)를 참조합니다.

---

## 1. BT_Dummy 현재 구조

Root Selector(`BTComposite_Selector_1`)의 자식 10개가 우선순위 순서입니다.
Root 서비스로 `BTService_GetDistToTarget`이 매 틱 `DistToTarget` 블랙보드 키를 갱신합니다.

| # | 브랜치 | 진입 조건 | 동작 |
|---|--------|-----------|------|
| 0 | Stop All Logic | `Shared.Status.PostureBroken` (abort Both) | Wait 10s |
| 1 | Stop All Logic | ShouldAbortAllLogic (abort Both) | Wait 10s |
| 2 | Phase2 | HP < 20% && !`Enemy.State.Phase2` | `Enemy.Ability.Phase2` |
| 3 | Incoming Defense | TargetActor 있음 && Dist <= 300 | Parry / Block |
| 4 | 압박 감지 | Dist < 350 && bPressureResponseRequested | Pressure.Counter / 후방회피 |
| 5 | 페이즈2 특수 | Random(0.2~0.4) && Phase2 태그 | Special.01 / 02 / 03 |
| 6 | 근접/후방닷지 | (조건 없음, 자식별 판정) | BackDodge / Melee |
| 7 | 대시 공격 | 600 <= Dist < 800, 쿨다운 8~12s | AttackType.Run / 전방회피 |
| 8 | 스트레이핑 | Dist <= 700 && TargetActor 있음 | 접근 / 회피 / Strafe |
| 9 | 추적 | Dist > 800 | MoveTo(TargetActor) |

---

## 2. BT 노드 → StateTree 대체표

**대부분의 보조 노드가 내장 기능으로 대체되어 삭제 가능합니다.**

| 현재 BT 노드 | StateTree 대체 | 비고 |
|--------------|---------------|------|
| `BTService_GetDistToTarget` + `DistToTarget` 키 | **Distance Compare** 조건 | `Conditions/StateTreeCommonConditions.h:206` — 두 액터 거리를 직접 잰다. 서비스도 키도 불필요 |
| `BTDecorator_Blackboard` (숫자 비교) | **Distance Compare** / **Float Compare** | `:73`, `:206` |
| `BTDecorator_Blackboard` (Is Set) | **Object Is Valid** | `Conditions/StateTreeObjectConditions.h:22` |
| `BTDecorator_ComputeChance` (BP) | **Random** 조건 | `:287` |
| `ACBTDecorator_RandomCooldown` (C++) | 전이의 **Delay Duration + Delay Random Variance** | 전이 속성으로 내장 |
| `BTDecorator_DoseActorHaveTag` (BP) | GameplayTag 조건 | `Conditions/StateTreeGameplayTagConditions.h` |
| `BTDecorator_TimeLimit` | 상태 전이 + Delay, 또는 태스크 내 타이머 | |
| Blackboard 전체 | 컨텍스트 바인딩 + 이벤트 페이로드 | `TargetActor`는 `AIController.TargetActor`로 직접 바인딩 |

**Blackboard 자산 자체가 필요 없어집니다.**

---

## 3. 목표 상태 구조

```
Root  (Selection Behavior: Try Select Children In Order)
│
├─ Dead              Shared.Status.Dead
├─ PostureBroken     Shared.Status.PostureBroken
├─ PhaseTransition   HP < 20% && !Enemy.State.Phase2   → Ability.Phase2
│
├─ Combat            [TargetActor Is Valid]                     (State — 아래 참고)
│   │  태스크: OrientToTarget (bConsideredForCompletion = false)
│   │
│   ├─ Defense           Required Event: IncomingAttack          (Group)
│   │   ├─ Parry             페이로드 bParryable + Random
│   │   └─ Block             페이로드 bBlockable
│   ├─ PressureCounter   Required Event: PressureReady
│   ├─ Phase2Special     [Enemy.State.Phase2] + Random           (Group)
│   │   ├─ Special01         Distance < MeleeDistance
│   │   ├─ Special02         Distance < PressureCounterDistance
│   │   └─ Special03         Distance < PressureCounterDistance
│   ├─ RunAttack         Distance RunAttackMin~Max
│   ├─ Chase             Distance > ChaseDistance
│   ├─ Melee             Distance < MeleeDistance                (Group)
│   │   ├─ BackDodge         Distance < BackDodgeDistance + Random
│   │   └─ Attack
│   │
│   └─ CloseRange        Distance <= StrafeDistance   ← 기본 폴백 (Group)
│       │  파라미터: StrafingLocation (FVector)
│       │
│       ├─ Approach         Random(0.7~0.9), 전이 Delay 3s
│       │                   → MoveTo(TargetActor, r=150)
│       ├─ Dodge            Distance < MeleeDistance + Random    (Group)
│       │   ├─ DodgeStart       회피 발동 + 1.5s
│       │   ├─ FindLocation     Run Env Query → StrafingLocation
│       │   └─ DodgeMove        MoveTo(TargetActor, r=100)
│       └─ Strafe           (조건 없음 — 폴백)                   (Group)
│           ├─ FindLocation     Run Env Query → StrafingLocation
│           └─ StrafeMove       MoveTo(StrafingLocation, r=20)
│
└─ Idle              (타겟 없음)
```

`CloseRange`를 `Combat`의 **마지막 자식**에, `Strafe`를 `CloseRange`의 마지막 자식에 두는 것이 중요합니다.
위 조건이 모두 실패해도 선택될 상태가 하나는 남아야 그룹 진입이 실패하지 않습니다.

### 3.0 State vs Group — Combat만 예외

`EStateTreeStateType`에는 태스크 유무로 갈리는 두 타입이 있습니다.

| Type | Tasks 섹션 | 용도 |
|------|:---:|------|
| `State` | 있음 | 태스크 + 자식 상태를 함께 가짐 |
| `Group` | **없음** | 자식 상태만 담는 순수 컨테이너 |

위 다이어그램의 `(Group)` 표기는 "자기 태스크 없이 자식만 분기하는 노드"를 뜻합니다.
`Defense`, `Phase2Special`, `Melee`, `CloseRange`, `Dodge`, `Strafe`가 여기 해당합니다.

**`Combat`은 예외적으로 `State`입니다.** `OrientToTarget` 태스크를 직접 들고 있어야 하기 때문입니다
(자식이 `Chase`든 `Melee`든 상관없이 계속 타겟을 보게 하려는 목적, 4.4절 참조).
`Group`으로 바꾸면 `Tasks` 섹션 자체가 사라져 태스크를 못 답니다.

### 3.1 EQS 결과 전달 패턴

`StateTreeRunEnvQueryTask.h:43-46` 주석에 명시된 정석 패턴을 따릅니다.

> The task is usually run in a **sibling state** to the result user,
> with the data being stored in the **parent state's parameters**.

BT에서 `Sequence(RunEQS → MoveTo)` + `StrafingLocation` 블랙보드 키였던 것이,
StateTree에서는 **부모 상태의 파라미터 + 형제 상태 2개**가 됩니다.

### 3.2 거리 값의 출처

위 구조의 거리 이름은 `UACDataAsset_BossTuning`의 프로퍼티입니다.
StateTree의 `Distance Compare` 조건에서 `AIController → TuningData → 각 값`으로 바인딩합니다.

루트 `Parameters`에 넣지 않은 이유는 그것이 **에셋마다 독립**이기 때문입니다
(`StateTree.h:165-169` — "**default** parameters of the state tree").
여러 보스가 값을 공유하려면 DataAsset을 거쳐야 합니다.

> DataAsset을 StateTree의 External Data로 직접 등록할 수는 없습니다.
> `StateTreeComponentSchema.cpp:51-56`의 `IsExternalItemAllowed`가
> `AActor` / `UActorComponent` / `UWorldSubsystem`만 허용합니다.

---

## 4. 번역 규칙 3가지

### 4.1 Selector 우선순위 → 형제 순서

`Try Select Children In Order`가 정의 순서대로 시도하므로 BT Selector와 1:1 대응됩니다.
가장 단순하게 옮겨지는 부분입니다.

### 4.2 FlowAbortMode → 부모 상태의 전이

**가장 큰 구조 변화입니다.**

BT에서 `FlowAbortMode = Both`로 하위 브랜치를 강제 중단시키던 것들
(`PostureBroken`, `ShouldAbortAllLogic`, `Phase2`)은 StateTree에서 **Root의 전이**로 올립니다.

부모 상태의 전이는 활성인 모든 자식에 적용되므로, 어느 상태에 있든 인터럽트가 걸립니다.
자식마다 중복해서 달 필요가 없습니다.

경합이 생기면 `Priority`로 해결합니다. `StateTreeTypes.h:210`에 명시된 대로
**"동시에 여러 전이가 발동하면 가장 높은 우선순위 중 첫 번째"**가 선택됩니다.

| 인터럽트 | Priority |
|----------|:---:|
| Dead | `Critical` |
| PostureBroken | `Critical` |
| PhaseTransition | `High` |
| 피격 반응 (Defense) | `High` |
| 나머지 | `Normal` |

### 4.3 폴링 조건 → 이벤트

BT는 매 틱 데코레이터를 재평가했지만, `AACStateTreeController`는 이미 신호를 이벤트로 변환해 보냅니다
(`ACStateTreeController.cpp:109`, `:116`, `:125`, `:152`).

이벤트로 전환되는 것:
- `bIncomingAttackParryable` / `bIncomingAttackBlockable` BB 불리언 → `IncomingAttack` 이벤트 페이로드
- 만료를 지우던 타이머 → 불필요 (반응 지속 시간은 상태가 관리)
- `bPressureResponseRequested` → `PressureReady` 이벤트

**거리 기반 조건만 폴링으로 남습니다.**

---

## 4.4 루트 설정 (Asset Details)

세 항목 모두 에셋 전역이며 성격이 다릅니다.

| 항목 | 방침 | 근거 |
|------|------|------|
| **Parameters** | 그 트리 고유 값만 (예: 페이즈2 전환 HP 비율) | 에셋마다 독립이므로 공유 값은 부적합 |
| **Evaluators** | **비워둔다** | 틱을 끄는 플래그가 없다 |
| **Global Tasks** | **비워둔다** | 완료되면 트리 전체가 멈춘다 |

**Evaluator를 쓰지 않는 이유:** `StateTreeEvaluatorBase.h`에는 `TreeStart` / `TreeStop` / `Tick`만 있고
Task가 가진 `bShouldCallTick` 같은 플래그가 **없습니다**. 하나라도 넣으면 트리가 매 틱 돌아야 하고,
`FACSTTask_Idle`로 얻은 "타겟 없을 때 무비용" 이점이 사라집니다.

BT의 `BTService_GetDistToTarget`을 Evaluator로 그대로 옮기고 싶은 유혹이 있으나,
`Distance Compare` 조건이 직접 거리를 재므로 불필요합니다.

**Global Tasks의 함정:** 글로벌 태스크가 완료되면 `RequestedStop`이 설정되어
트리 전체가 정지합니다 (`StateTreeExecutionContext.cpp:1813-1840`).
`Any` 드롭다운은 `EStateTreeTaskCompletionType` (`StateTreeTasksStatus.h:15-21`) — All / Any.

**상시 동작은 Global이 아니라 그룹 상태의 태스크로.**
BT에서 `ACBTService_OrientToTargetActor`를 9개 브랜치에 중복으로 달아뒀는데,
`Combat` 그룹의 태스크로 올리면 1개로 줄고 자식이 무엇이든 유지됩니다.
이때 `bConsideredForCompletion = false`를 주어 상태를 완료시키지 않게 합니다
(`StateTreeTaskBase.h:144-150`).

---

## 4.5 상태 색상

BT_Dummy의 코멘트 노드 색을 그대로 재사용해 두 에셋이 같은 언어로 읽히게 합니다.
Asset Details → Theme → Colors에 등록한 뒤 각 상태의 `Color` 드롭다운에서 선택합니다.

| 용도 | Linear RGB | 대상 |
|------|-----------|------|
| 중단 | `0.040, 0.040, 0.040` | Dead, PostureBroken |
| 페이즈 전환 | `0.723, 0.080, 0.684` | PhaseTransition |
| 방어 | `0.025, 0.029, 0.150` | Defense 그룹 |
| Parry | `0.488, 0.092, 0.134` | Parry |
| Block | `0.380, 0.125, 0.481` | Block |
| 공격 | `0.799, 0.072, 0.045` | Melee, Special, PressureCounter |
| 회피 | `1.000, 0.725, 0.252` | BackDodge, Dodge |
| 접근 | `0.671, 0.647, 0.053` | Approach, RunAttack |
| 스트레이핑 | `0.014, 0.150, 0.136` | Strafe |
| 추적 | `0.042, 0.198, 0.665` | Chase |

색은 **계열**을 나타내게 하고 상태마다 다른 색을 주지 않습니다.
"빨강 계열이면 공격, 파랑 계열이면 이동"이 한눈에 들어와야 Debugger에서 값을 합니다.

---

## 4.6 서브 에셋으로 분리하지 않는 이유

`LinkedAsset`으로 Defense 등을 별도 에셋으로 빼는 것은 **현 단계에서 권하지 않습니다.**

`LinkedAsset` 상태는 **새 실행 프레임**을 만들고 (`StateTreeExecutionContext.cpp:2735-2738`),
경계를 넘는 데이터는 **파라미터뿐**입니다 (`:2422-2424`):

> This state is a container for the linked state tree.
> **Its instance data matches the linked state tree parameters.**

즉 `AIController.TargetActor` 같은 직접 바인딩이 서브 에셋 안에서는 불가능하고,
링크 상태에 파라미터를 선언 → 바깥에서 채움 → 안에서 재바인딩이 필요합니다.
상태 3~4개짜리 그룹에는 배선 비용이 이득보다 큽니다.

**LinkedAsset의 용도는 런타임 교체입니다.**
`AddLinkedStateTreeOverrides(StateTag, Ref)` (`StateTreeComponent.h:101`)로
태그가 일치하는 링크 상태의 트리를 통째로 갈아끼웁니다.
나중에 페이즈별 패턴 세트를 스왑할 때가 그 시점이며, `Phase2Special`이 첫 후보입니다.

---

## 5. 구현 순서

한 번에 12개 상태를 만들지 않고 단계별로 검증합니다.

| 단계 | 범위 | 검증 방법 |
|:---:|------|-----------|
| 1 | `Idle` ↔ `Chase` (완료) | Debugger에서 상태 전환 확인 |
| 2 | `Combat` 그룹 + `Strafe` 폴백 | 거리에 따라 Chase/Strafe 왕복 |
| 3 | `Melee` (Attack) | 근접 시 공격 발동 |
| 4 | Root 인터럽트 (Dead / PostureBroken) | 피격·사망 시 즉시 중단 |
| 5 | `Defense` (Parry / Block) | 이벤트 페이로드 분기 |
| 6 | `PhaseTransition` + `Phase2Special` | HP 20% 이하 전환 |
| 7 | `PressureCounter`, `RunAttack`, `BackDodge` | 나머지 패턴 |

---

## 5.1 필요한 커스텀 노드

엔진에 **GAS 연동 StateTree 태스크가 하나도 없습니다.** (플러그인 전체 검색으로 확인)
BT는 `AIModule`이 GAS와 오래 엮여왔지만 StateTree는 그 통합이 없어 직접 포팅해야 합니다.

| BT 노드 | StateTree 대체 | 상태 |
|---------|---------------|:---:|
| — | `FACSTTask_Idle` | 완료 |
| `ACBTTask_ActivateAbilityByTag(AndWait)` | `FACSTTask_ActivateAbilityByTag` | 완료 |
| `ACBTService_OrientToTargetActor` | `FACSTTask_OrientToTarget` | 미완 |
| `BTTask_ToggleStrafingState` (BP) | 위 태스크에 통합 검토 | 미완 |
| `BTService_MotionWarpingUpdateTarget` (BP) | 워프 타겟 갱신 태스크 | 미완 |
| `BTTask_SendGameplayEvent` (BP) | 회피 어빌리티에 태그가 있으면 **불필요** | 판단 필요 |
| `BTTask_Wait` | Delay 전이로 대체 | 불필요 |
| `BTTask_MoveTo` / `BTTask_RunEQSQuery` | 내장 (`Move To` / `Run Env Query`) | 불필요 |

### 어빌리티 태스크의 틱 제거

BT 버전은 `TickTask`에서 매 틱 `IsAbilityStillRunning()`을 폴링했지만,
StateTree 버전은 **틱을 돌지 않습니다.**

`Context.MakeWeakExecutionContext()`로 약참조를 떠서 델리게이트에 넘기고
콜백에서 `FinishTask()`를 호출합니다. 엔진의 `Move To`가 쓰는 것과 같은 패턴입니다
(`StateTreeMoveToTask.cpp:150-155`).

| 조건 | 종료 감지 |
|------|-----------|
| `WaitOwnedTag` 설정 | `RegisterGameplayTagEvent` → `NewCount <= 0` |
| 미설정 | `ASC->OnAbilityEnded` |
| `MaxWaitTime > 0` | 월드 타이머 |

덕분에 긴 몽타주가 재생되는 동안 StateTree 컴포넌트가 틱을 멈출 수 있습니다.

**BT와 의도적으로 다른 점:** `ExitState`에서 `CancelAbilityHandle()`을 호출합니다.
StateTree는 부모 전이(`PostureBroken` 등)로 상태가 강제 종료될 수 있어
어빌리티가 남아 도는 것을 막아야 합니다. 엔진의 `Move To`도 `ExitState`에서
`ExternalCancel()`을 부릅니다 (`StateTreeMoveToTask.cpp:70-81`).
"인터럽트돼도 몽타주는 끝까지" 연출을 원하면 이 호출을 제거해야 합니다.

---

## 6. 알려진 함정

`StateTree_Runtime_Guide.md`에서 확인한 런타임 제약 중 이 설계에 직접 걸리는 것들입니다.

1. **태스크 없는 상태는 즉시 완료된다** (`StateTreeExecutionContext.cpp:4642`)
   `Idle` 같은 대기 상태에는 `FACSTTask_Idle`을 반드시 넣는다.
   판정 기준은 틱 여부가 아니라 `EnabledTasksNum`이므로, 틱을 꺼도 태스크가 있으면 유지된다.

2. **한 틱의 전이 반복은 5회 제한** (`:1862`)
   초과하면 경고 없이 그 틱이 끝난다. 형제 상태의 진입 조건이 서로 배타적인지 확인할 것.

3. **이벤트 큐는 64개** (`StateTreeEvents.h:155`)
   이벤트는 상태 변화 알림으로만 쓰고, 지속적인 값 전달은 조건/바인딩으로 처리한다.

4. **NavMesh 커버리지**
   `Move To`는 경로 탐색 즉시 실패 시 **로그 없이** Failed를 반환한다
   (`StateTreeMoveToTask.cpp:145-148`). 맵에 `NavMeshBoundsVolume`이 있는 것과
   보스 위치가 실제로 커버되는 것은 다른 문제다.

5. **Root 직속 인터럽트 상태에는 Enter Condition이 반드시 있어야 한다**
   `Groggy`(PostureBroken), `Dead`, `PhaseTransition`처럼 4.2절 방식으로 Root 전이를 다는 상태들이다.
   Root에 `On Tick` + `Critical` 전이만 걸고 **상태 자체의 Enter Conditions를 비워두면,
   게임 시작 직후 그 상태에 눌러앉아 `Combat` 아래로 내려가지 못한다.**
   Root의 `Try Select Children In Order` 하향 선택이 조건 없는 첫 자식을 무조건 고르기 때문이다.

   전이는 "인터럽트로 들어가는 문"만 만든다. "평소에 안 들어가게 막는 문지기"는 Enter Conditions뿐이다.
   두 경로(하향 선택 / 전이)가 각각 독립적으로 상태에 진입한다는 점이 BT Selector와 다르다.

   전이 타깃도 진입 시 Enter Conditions를 검사하므로(`StateTreeExecutionContext.cpp:6942-6945`)
   같은 조건을 양쪽에 두면 일관되게 동작한다. 전이 쪽 조건도 지우지 말 것 —
   지우면 매 틱 전이가 발동을 시도했다가 타깃 진입 조건에서 실패하는 낭비가 생긴다.

   조건 없는 상태는 Root의 **마지막 자식(폴백)** 자리에만 놓을 수 있다 (3절의 `Idle`, `CloseRange`).

6. **바인딩 노출 조건**
   컨트롤러 프로퍼티를 바인딩하려면 `CPF_Edit`가 필요하다 — `BlueprintReadOnly`만으로는 부족하고
   `VisibleAnywhere`/`EditAnywhere` 계열이어야 한다 (`PropertyBindingExtension.cpp:1142`).
   private이면 `meta = (AllowPrivateAccess = "true")`도 함께 붙인다.
   원인 파악이 안 될 때는 `Log LogPropertyBindingUtils Verbose`로 거절 사유를 직접 확인할 수 있다.
