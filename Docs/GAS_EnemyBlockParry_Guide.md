# GAS Enemy Block / Parry / CounterAttack 가이드

## 개요

이 문서는 Enemy(Ashen Knight)가 플레이어의 공격을 예고받아 Block/Parry로 방어하고,
Parry 성공 시 카운터 공격까지 이어지는 전체 흐름을 설명합니다.

핵심 설계 원칙:
- **Notify는 예고만 보낸다.** 방어 성공 여부는 Notify 시점이 아니라 실제 Hit 시점에 판정한다.
- **판정은 공통 함수 하나로 통일한다.** 데미지 감쇄 / 성공 이벤트 / HitReact 억제 / Hit Cue 억제가
  전부 `IsSuccessfulBlock` / `IsSuccessfulParry`를 공유해 서로 어긋나지 않는다.
- **확률·쿨다운·거리 조건은 BT가 담당한다.** 어빌리티는 활성화되면 항상 정직하게 수행한다.

---

## 1. 전체 흐름

```
[1. 예고]
   Player 공격 몽타주의 AN_IncomingAttackWarning (Notify)
   → 현재 실행 중인 공격 어빌리티에 CurrentAttackDefenseTags 저장
   → SphereOverlapActors로 반경 내 Enemy 탐색
   → Shared.Event.Combat.IncomingAttack 발송
     (Payload: Instigator=공격자, EventMagnitude=예상 히트 시간, InstigatorTags=방어 태그)
         ↓
[2. 수신/판단]
   AACEnemyController::OnIncomingAttackEventReceived
   → 상태 가드 (Dead / PostureBroken / Attacking / Phase2 전환 중이면 무시)
   → Blackboard 기록: IncomingAttackActor, IncomingAttackTimeToImpact,
                      bIncomingAttackParryable, bIncomingAttackBlockable
   → 타이머로 예상 히트 시간 + GraceTime 뒤 자동 소거
         ↓
   BT_AshenKnight
   → BB Bool 체크 + BTDecorator_ComputeChance(확률) + ACBTDecorator_RandomCooldown(쿨다운)
   → ACBTTask_ActivateAbilityByTagAndWait(Enemy.Ability.Parry 또는 Enemy.Ability.Block)
         ↓
[3. 방어 자세]
   GA_Boss_Parry: Startup 딜레이 → Shared.Status.Parry 부여(판정 창) → 창 종료 시 제거
   GA_Boss_Block: Enemy.Status.Blocking 부여 + 자세 몽타주 유지
         ↓
[4. 실제 판정]  (Player 공격이 실제로 Hit되는 순간)
   ACCalculation_DamageTaken::Execute_Implementation
   → IsSuccessfulParry: 데미지 0 + 공격자 체간 역공/경직 + Enemy.Event.ParrySuccess 발송
   → IsSuccessfulBlock: 데미지 90% 감소 + 방어자 체간 소폭 누적
   → 둘 다 아니면: 일반 피격
         ↓
[5. 카운터]  (Parry 성공 시)
   GA_Boss_Parry가 ParrySuccess 이벤트 수신
   → 성공 Cue 실행 → CounterAttackAbilityTag의 어빌리티 활성화
   → 카운터 어빌리티 종료까지 Parry 어빌리티 유지 (BT 대기 유지)
```

---

## 2. 공통 판정 함수 (UACFunctionLibrary)

모든 방어 판정의 단일 진입점입니다. 이 함수들 외의 곳에서 방어 성공을 직접 계산하지 않습니다.

| 함수 | 판정 내용 |
|------|-----------|
| `IsAttackBlockable(Tags)` | `Shared.Attack.Blockable` 있고 `Unblockable` 없음 (태그 가능 여부만) |
| `IsAttackParryable(Tags)` | `Shared.Attack.Parryable` 있고 `Unparryable` 없음 (태그 가능 여부만) |
| `IsActorBlocking(Actor)` | `Player.Status.Blocking` 또는 `Enemy.Status.Blocking` 보유 |
| `IsValidBlock(Attacker, Defender)` | 방어자 정면 각도 판정 (아래 참조) |
| `IsSuccessfulBlock(Attacker, Defender, Tags)` | 실제 Block 성공 (아래 4조건 모두 충족) |
| `IsSuccessfulParry(Attacker, Defender, Tags)` | 실제 Parry 성공 (Block과 대칭) |

### Block/Parry 성공 조건

```
IsSuccessfulBlock = Attacker/Defender 유효
                  && IsAttackBlockable(공격 태그)
                  && IsActorBlocking(방어자)
                  && IsValidBlock(정면 각도)

IsSuccessfulParry = Attacker/Defender 유효
                  && IsAttackParryable(공격 태그)
                  && 방어자가 Shared.Status.Parry 보유
                  && IsValidBlock(정면 각도)
```

### IsValidBlock — 위치 기반 정면 판정

```
Dot(방어자 Forward, (공격자 위치 - 방어자 위치).정규화) >= cos(AngleThreshold)
```

- 기본 AngleThreshold 60도 = 방어자 정면 60도 콘 안에 공격자가 있어야 유효
- 공격 태그가 없거나 뒤/측면 피격이면 Blocking/Parry 상태여도 일반 피격 처리

### 이 판정을 사용하는 4곳 (기준 일치 보장)

| 사용처 | 역할 |
|--------|------|
| `ACCalculation_DamageTaken` | 데미지 감쇄/무효화 |
| `TryTriggerSuccessfulBlockEvent` | `Player.Event.SuccessfulBlock` 발송 여부 |
| `ACAttributeSet::HandleDamageAndTriggerHitReact` | Enemy Block 성공 시 HitReact 억제 |
| `ACAbility_Attack::PlayHitGameplayCue` | 일반 Hit Cue 억제 |

---

## 3. 공격 예고 — AN_IncomingAttackWarning

방어/패링 가능 여부의 **원본 데이터는 Notify 자신**이 가집니다.
같은 어빌리티가 여러 몽타주(콤보)를 써도 타격마다 다른 속성을 줄 수 있습니다.

### Notify 프로퍼티 (몽타주에서 타격별 설정)

| 프로퍼티 | 기본값 | 설명 |
|----------|--------|------|
| `AttackDefenseTags` | (비어 있음) | `Shared.Attack.Blockable/Parryable/Unblockable/Unparryable` 조합 |
| `ExpectedHitTime` | 0.3 | Notify 발생~실제 타격까지 예상 시간(초). Payload의 EventMagnitude로 전달 |
| `ThreatLevel` | 0.5 | BT 확장용 위협도 (현재 미소비) |
| `DetectionRadius` | 1500 | 예고를 받을 Enemy 탐색 반경(cm). 무기 리치 수준으로 줄이는 것을 권장 |

### 데이터 일치 트릭

```
Notify 발생
   → AttackAbility->SetCurrentAttackDefenseTags(자신의 AttackDefenseTags)   // ①
   → Payload.InstigatorTags에도 같은 태그를 실어 Enemy에게 발송              // ②

실제 Hit 발생
   → ACAbility_Attack::CreateDamageEffectSpec
   → CurrentAttackDefenseTags(①에서 저장된 값)를 Spec의 DynamicAssetTags에 주입
   → ACCalculation_DamageTaken이 GetDynamicAssetTags()로 읽어 판정
```

예고(②)와 실제 판정(①→Spec)이 같은 데이터를 참조하므로 어긋날 수 없습니다.

주의:
- `ActivateAbility` 시작 시 `CurrentAttackDefenseTags.Reset()` — 이전 공격의 태그 잔존 방지
- **Notify가 없는 공격 = 방어 불가 공격** (명시적 허용 방식). 기존 공격에 Notify를 배치하지
  않으면 Player의 해당 공격은 Boss가 방어할 수 없고, Boss 공격 몽타주에 Notify가 없으면
  Player도 그 공격을 Block/Parry할 수 없다.

---

## 4. 이벤트 수신 메커니즘 — GenericGameplayEventCallbacks

`AACEnemyController::OnPossess`에서 ASC의 이벤트 델리게이트를 구독합니다.

```cpp
ASC->GenericGameplayEventCallbacks
    .FindOrAdd(Shared_Event_Combat_IncomingAttack)
    .AddUObject(this, &ThisClass::OnIncomingAttackEventReceived);
```

- `GenericGameplayEventCallbacks`는 엔진 `UAbilitySystemComponent`의 기본 멤버(TMap)
- `SendGameplayEventToActor` → `ASC->HandleGameplayEvent` 내부에서 엔진이 자동 브로드캐스트
  (구독 등록은 수동, 발화는 자동)
- **정확히 일치하는 태그만** 발화 (태그 계층 매칭 없음)
- 전달되는 `FGameplayEventData*`는 발송 측 스택 변수의 포인터 → **콜백 안에서 즉시 값 복사**
  (멤버에 포인터 저장 금지 — 브로드캐스트 종료 후 댕글링)
- `ACPressureDetectionComponent`(`Shared.Event.HitReact` 구독)와 동일 패턴

---

## 5. Blackboard 예고 수명 관리 — 타이머와 세대 카운터

예고 정보가 Blackboard에 영원히 남으면 BT가 뒤늦게 엉뚱한 방어를 시도하므로,
`예상 히트 시간 + GraceTime(0.15초)` 뒤 자동 소거합니다.

```cpp
const int32 WarningId = ++IncomingAttackWarningId;                  // 세대 번호 발급
FTimerDelegate ClearDelegate = FTimerDelegate::CreateUObject(
    this, &ThisClass::ClearIncomingAttackBlackboard, WarningId);    // 번호를 payload로 박제
GetWorldTimerManager().SetTimer(IncomingAttackClearTimerHandle, ClearDelegate, ClearDelay, false);
```

- `FTimerDelegate` = "어떤 객체의 어떤 함수를 어떤 인자로 호출할지"를 담는 봉투.
  `CreateUObject`의 트레일링 인자(payload)는 **바인딩 시점에 복사**되어 발화 시 파라미터로 전달됨
- 시간은 델리게이트가 아니라 `SetTimer`가 관리 (무엇/언제의 분리)
- **같은 핸들로 SetTimer 재호출 = 기존 타이머 취소 후 교체** → 지우기 타이머는 항상 최신 예고 것 하나만 존재
- `ClearIncomingAttackBlackboard(ExpectedWarningId)`는 박제된 번호 ≠ 현재 번호면 무시
  → 낡은 지우기 콜백이 신선한 예고를 지우는 것을 구조적으로 차단 (세대 카운터 2차 안전장치)
- 마지막 인자 `false` = 1회성 (발화 후 자동 소멸). 발화 **전** 취소는 `ClearTimer` 사용

---

## 6. Boss Parry 어빌리티 (UACEnemyAbility_Parry)

```
ActivateAbility
   → CommitAbility → ParryMontage 재생(연출) → WaitDelay(StartupDuration)
         ↓
EnterParryWindow
   → Shared.Status.Parry 부여 (실제 판정 창 — 기존 판정 코드가 무수정으로 인식)
   → 레이스 시작:  WaitDelay(ParryWindowDuration)  ── 타임아웃 = 실패
                  WaitGameplayEvent(Enemy.Event.ParrySuccess) ── 수신 = 성공
         ↓ (먼저 도착한 쪽이 승리)
[실패] ExitParryWindow(false)
   → Parry 태그 제거 → WaitDelay(FailureRecoveryDuration) 후딜 → EndAbility
[성공] OnParrySuccessEventReceived
   → ExecuteSuccessfulParryCue (GameplayCue.FX.Parry)
   → ExitParryWindow(true)
      → Parry 태그 제거 → 레이스 태스크 정리
      → TryActivateCounterAttackAbility
      → 카운터 종료까지 Parry 어빌리티 유지 (즉시 EndAbility 하지 않음)
```

### 에디터 설정값

| 프로퍼티 | 기본값 | 설명 |
|----------|--------|------|
| `StartupDuration` | 0.12 | 패링 자세 진입 선딜(초) |
| `ParryWindowDuration` | 0.20 | Shared.Status.Parry 유지 시간 = 판정 창 |
| `FailureRecoveryDuration` | 0.6 | 실패 시 후딜 |
| `ParryMontage` | — | 자세 연출 몽타주 (판정과 무관) |
| `SuccessfulParryCueTag` | GameplayCue.FX.Parry | 성공 Cue (Player와 공유, 교체 가능) |
| `CounterAttackAbilityTag` | — | 성공 시 실행할 카운터 어빌리티 AssetTag |

- Parry 시도 확률/쿨다운은 어빌리티에 없음 — BT의 `BTDecorator_ComputeChance` /
  `ACBTDecorator_RandomCooldown`이 전담

### WaitDelay 레이스 패턴

`UAbilityTask_WaitDelay`는 내부적으로 `SetTimer`를 쓰되 **어빌리티 수명에 묶인** 태스크입니다.

- 생성(`WaitDelay`) → 델리게이트 바인딩 → `ReadyForActivation()` 3단계 (바인딩 틈을 주기 위한 분리)
- 어빌리티 종료 시 태스크 자동 정리 + `ShouldBroadcastAbilityTaskDelegates()` 가드
  → 죽은 어빌리티의 콜백이 발화하지 않음 (raw SetTimer에 없는 안전장치)
- 사용 구분: **어빌리티 안의 딜레이는 WaitDelay, 어빌리티 밖(컨트롤러 등)은 SetTimer**
- "제한 시간 안에 X가 일어나는가" = WaitDelay(타임아웃)와 WaitGameplayEvent(신호)를 동시에 걸고
  먼저 온 쪽이 진 쪽을 `EndTask()`로 정리하는 레이스 구조

---

## 7. 카운터 공격 — 종료 동기화 (SpecHandle 추적)

Parry 성공 직후 어빌리티를 바로 끝내면 BT의 `ActivateAbilityByTagAndWait(Enemy.Ability.Parry)`가
완료되고, 아직 살아있는 IncomingAttack BB 값 때문에 Block 브랜치가 선택되어 카운터가 끊깁니다.
따라서 **카운터 어빌리티가 끝날 때까지 Parry 어빌리티를 유지**합니다.

```
TryActivateCounterAttackAbility
   → GetActivatableAbilities()에서 GetAssetTags().HasTag(CounterAttackAbilityTag)로 Spec 검색
     (SpecHandle이 필요하므로 TryActivateAbilitiesByTag 대신 직접 검색)
   → SpecHandle 기록 + ASC->OnAbilityEnded.AddUObject(...) 바인딩   ← 활성화 "전"에 바인딩
     (동기적으로 즉시 끝나는 케이스도 놓치지 않기 위함)
   → TryActivateAbility(SpecHandle)
         ↓
카운터 어빌리티 종료 (실제 종료 시점 — Delay/몽타주 길이 하드코딩 없음)
   → OnCounterAttackAbilityEnded(EndedData)
   → EndedData.AbilitySpecHandle == 추적 중인 SpecHandle 확인
   → Parry 어빌리티 EndAbility
```

- Parry의 `EndAbility`는 어떤 종료 경로에서든 델리게이트 해제 + SpecHandle 초기화 (누수 방지)
- Parry 종료가 카운터를 취소하지 않음 (Cancel/Block 태그 관계 없음)
- 카운터 어빌리티는 정식 공격 어빌리티(BP)로 만들어 기존 공격 파이프라인
  (무기 콜리전 → MeleeHit → 데미지 GE)을 그대로 사용

---

## 8. Boss Block 어빌리티 (UACEnemyAbility_Block)

Block은 단발 반응이 아니라 **자세 유지형**입니다. Block 성공이 어빌리티를 끊지 않아야 합니다.

```
ActivateAbility
   → Enemy.Status.Blocking 부여 (ActivationOwnedTags)
   → WaitGameplayEvent(Player.Event.SuccessfulBlock) 구독
   → BlockMontage(자세) 재생 — 몽타주 길이 = 최대 Block 유지 시간
         ↓
Block 성공 (TryTriggerSuccessfulBlockEvent가 이벤트 발송)
   → SuccessfulBlockCueTag 실행
   → 기존 자세 몽타주 태스크를 EndTask()로 "조용히" 종료   ← 핵심
   → BlockHitMontage(움찔) 재생 → 끝나면 다시 자세 몽타주 복귀
```

### 몽타주 전환 패턴 (상태 유지 어빌리티의 핵심 테크닉)

- **의도한 전환**: 기존 태스크를 먼저 `EndTask()` → OnInterrupted 콜백이 발화하지 않음 → 어빌리티 유지
- **외부 강제 중단**(그로기/사망 몽타주 오버라이드): OnInterrupted 발화 → EndAbility (진짜 종료 조건)

### Block 종료 조건

| 유지되는 경우 | 종료되는 경우 |
|---------------|---------------|
| Blockable 공격을 성공적으로 블록 (움찔+Cue 후 복귀) | 자세 몽타주 자연 완료 (= 유지 시간 종료) |
| BlockHit 몽타주 재생 중 | Unblockable 공격 피격 → HitReact가 몽타주 중단 |
| | PostureBroken/사망 |
| | BT/AI의 명시적 취소 |

### HitReact 억제 (ACAttributeSet)

Block 성공해도 데미지가 10% 남아 HitReact 이벤트가 발송되고, 피격 몽타주가 Block 몽타주를
끊는 문제가 있었습니다. `HandleDamageAndTriggerHitReact`에서 다음 조건이면 HitReact를 생략합니다:

```
Enemy.Status.Blocking 보유            ← Player는 절대 가질 수 없는 태그 (Player 흐름 무영향)
&& IsSuccessfulBlock(공통 판정)        ← Unblockable/뒤측면/Block 실패 시 HitReact 정상 발송
```

부수 효과: 블록 성공 히트는 `Shared.Event.HitReact`가 발송되지 않으므로
`ACPressureDetectionComponent`의 압박 카운트에 집계되지 않음.

---

## 9. Parry 성공 시 데미지 계산 처리 (ACCalculation_DamageTaken)

Hit 순간 한 프레임 안에서 동기적으로 처리됩니다.

```
IsSuccessfulParry == true
   ├─ FinalDamageDone = 0, FinalPostureDamage = 0
   ├─ 공격자(Source)에게 UACGameplayEffect_PostureCounter 적용
   │    (BasePostureDamage × 1.5 — 슈퍼아머는 HitReact 모션만 생략할 뿐 체간 누적을 막지 않으므로
   │     별도 우회 없이 그대로 누적된다. Dead/PostureBroken/Executed/Invincible일 때만
   │     HandlePostureDamage에서 누적이 무효화됨)
   ├─ 공격자에게 StaggerEffectClass(BP GE) 적용
   │    (Shared.SetByCaller.StaggerDuration = FRandRange(Min, Max) 주입
   │     → Shared.Status.Stagger 부여 → 공격 어빌리티 ActivationBlockedTags가 재공격 잠금)
   └─ 방어자에게 Enemy.Event.ParrySuccess 발송
        (Boss의 GA_Parry 레이스를 깨움. Player Parry 시엔 리스너 없음 = no-op)

IsSuccessfulBlock == true (Parry 실패 시 검사)
   ├─ FinalDamageDone × 0.1  (90% 감쇄)
   └─ FinalPostureDamage = BasePostureDamage × 0.8  (방어자 체간 누적)
```

- Execution 출력(AddOutputModifier)은 Target만 수정 가능하므로, Source 방향 효과(역공/경직)는
  Source ASC에 GE를 직접 적용하는 패턴 사용 (흡혈/반사 데미지와 동일한 관용구)
- Stagger 지속시간 범위는 `CalcDT_Default`(DamageCalculation BP)의
  `StaggerDurationMin/Max`(기본 0.5~0.8)로 조정

---

## 10. PressureCounter 통합

압박 반격(`UACEnemyAbility_PressureCounter`)도 동일 태그 시스템을 사용합니다.

- `PressureCounterDefenseTags` 기본값: `Shared.Attack.Parryable` + `Shared.Attack.Unblockable`
  → **Block으로는 뚫리고, 정확한 타이밍의 Parry만 통하는 공격**
- 근접/Instant AOE/Sustained AOE 3경로 모두 같은 태그를 Spec 주입 + 이벤트 발송에 사용
- BP에서 태그 조합만 바꾸면 공격 성격 변경 가능 (예: Blockable 추가, Unparryable 교체)

---

## 11. 관련 GameplayTag 목록

| 태그 | 용도 |
|------|------|
| `Shared.Event.Combat.IncomingAttack` | 공격 예고 이벤트 (성공 아님, 반응 기회만) |
| `Shared.Attack.Blockable / Parryable` | 공격의 방어 허용 속성 (Notify에서 설정) |
| `Shared.Attack.Unblockable / Unparryable` | 방어 금지 속성 (허용보다 우선) |
| `Shared.Status.Parry` | 패링 판정 창 (Player/Enemy 공용 — 기존 태그 재사용) |
| `Player.Status.Blocking` / `Enemy.Status.Blocking` | 블록 상태 (IsActorBlocking이 OR로 검사) |
| `Shared.Status.Stagger` | 패링당한 공격자의 경직 (재공격 잠금) |
| `Shared.SetByCaller.StaggerDuration` | Stagger GE 지속시간 주입 |
| `Player.Event.SuccessfulBlock` | 방어 성공 이벤트 (Player/Enemy Block 어빌리티가 구독) |
| `Enemy.Event.ParrySuccess` | Hit 판정→GA_Parry 성공 콜백 |
| `Enemy.Ability.Parry / Block` | BT 태스크가 활성화할 어빌리티 AssetTag |
| `Enemy.Ability.Parry.CounterAttack` | Parry 성공 시 실행할 카운터 어빌리티 AssetTag |

---

## 12. 관련 파일 목록

| 역할 | 파일 |
|------|------|
| 공통 판정 함수 | `ACFunctionLibrary.h / .cpp` |
| 공격 예고 Notify | `ACAnimNotify_IncomingAttackWarning.h / .cpp` |
| 예고 수신/Blackboard 동기화 | `ACEnemyController.h / .cpp` |
| Boss Parry 어빌리티 | `ACEnemyAbility_Parry.h / .cpp` |
| Boss Block 어빌리티 | `ACEnemyAbility_Block.h / .cpp` |
| Block 공용 베이스 | `ACGameplayAbility_Block.h / .cpp` |
| 공격 어빌리티 (태그 저장/Spec 주입) | `ACAbility_Attack.h / .cpp` |
| 데미지 계산 (실제 판정) | `ACCalculation_DamageTaken.h / .cpp` |
| HitReact 억제 | `ACAttributeSet.h / .cpp` |
| 압박 반격 (태그 통합) | `ACEnemyAbility_PressureCounter.h / .cpp` |
| BT 어빌리티 실행 태스크 | `ACBTTask_ActivateAbilityByTagAndWait.h / .cpp` |
| 전투 태그 | `ACGameplayTags_Combat.h / .cpp` |
| 적 태그 | `ACGameplayTags_Enemy.h / .cpp` |
