# GAS 가드 브레이크(Guard Break) 가이드

## 개요

플레이어가 공격을 **막을 때마다 가드 게이지가 누적**되고, 임계값에 도달하면 방어가 실제로 무너지는 시스템입니다.
세키로의 체간 압박과 같은 결로, "무한정 막고만 있을 수는 없다"는 긴장을 만듭니다.

핵심 설계 원칙:
- **모든 블록이 자원을 소모한다.** 강공격만 위험한 게 아니라, 약공격도 누적되면 뚫린다.
- **패링은 소모하지 않는다.** 완벽한 방어에는 대가가 없다 — 블록 vs 패링의 차별점.
- **체간(Posture)과 완전히 독립이다.** 태그·Attribute·GE를 공유하지 않아 서로 간섭하지 않는다.
- **판정은 AttributeSet이 한다.** 어빌리티는 부하를 넣기만 하고, 임계값 판단은 `HandleGuardDamage`가 담당한다.

---

## 1. 전체 흐름

```
[1. 막아냄]
   PawnCombatComponent::OnHitTargetActor
   → UACFunctionLibrary::TryTriggerSuccessfulBlockEvent
   → Player.Event.SuccessfulBlock 발송 (Payload.InstigatorTags = 공격의 속성 태그)
         ↓
[2. 부하 적용]
   UACPlayerAbility_Block::OnSuccessfulBlockEventReceived
   → 패링이면 여기서 종료 (게이지 누적 없음)
   → 순수 블록이면 ApplyGuardDamage
      · 공격에 GuardBreakWeightTag가 있으면 GuardBreakHeavyAmount, 없으면 GuardBreakAmountPerHit
      · GE_Player_GuardDamage 적용 (SetByCaller: Shared.SetByCaller.GuardDamage)
         ↓
[3. 누적·판정]
   UACAttributeSet::HandleGuardDamage  (GuardDamageTaken 메타 Attribute 소비)
   → GuardBreakResistance만큼 감쇄
   → GuardGauge에 누적
   → GE_Player_GuardDecayCooldown 재적용 (회복 유예 리셋)
   → GuardGauge >= MaxGuardGauge 이면
      · 게이지 0으로 리셋
      · Shared.Event.GuardBrokenTriggered 발송
         ↓
[4. 가드 붕괴]
   UACPlayerAbility_Block::OnGuardBrokenEventReceived → TriggerGuardBreak
   → GE_Player_GuardBreak 적용 (Player.Status.GuardBroken 부여)
   → EndAbility (Blocking 태그·이동제한 GE 해제 = 실제로 방어가 풀림)
   → GuardBreakMontage 재생
         ↓
[5. 회복]
   GE_Player_GuardDecay (상시 주기형)
   → Shared.Status.GuardDecayBlocked가 없을 때만 동작
   → 초당 GuardGaugeRegenRate만큼 GuardGauge 감소
```

---

## 2. GameplayEffect 4종

### GE_Player_GuardDamage — 게이지 채우기

| | |
|---|---|
| **의도** | 막아낼 때마다 가드 게이지에 부하를 넣는다 |
| **적용 시점** | 블록 성공 시 **매번** |
| **연결 위치** | `GA_Player_Block` → `Guard Damage Effect` |
| **적용 주체** | `UACPlayerAbility_Block::ApplyGuardDamage` |

```
Duration Policy : Instant
Modifiers
  Attribute   : ACAttributeSet.GuardDamageTaken     ← 메타 Attribute (GuardGauge 아님)
  Modifier Op : Add
  Magnitude   : Set by Caller
    Data Tag  : Shared.SetByCaller.GuardDamage
```

> **주의**: Attribute를 `GuardGauge`로 직접 지정하면 `HandleGuardDamage`가 호출되지 않아
> 감쇄·임계값 판정·유예 리셋이 전부 건너뛰어진다. 반드시 메타 Attribute를 경유해야 한다.

---

### GE_Player_GuardBreak — 붕괴 후 경직

| | |
|---|---|
| **의도** | 가드가 무너진 뒤 잠시 아무것도 못 하게 만든다 |
| **적용 시점** | 게이지가 최대치에 도달했을 때 **1회** |
| **연결 위치** | `GA_Player_Block` → `Guard Break Effect` |
| **적용 주체** | `UACPlayerAbility_Block::TriggerGuardBreak` |

```
Duration Policy : Has Duration (1.0)
Granted Tags    : Player.Status.GuardBroken
Modifiers       : 없음
```

이 태그가 하는 일:
- `UACPlayerAbility_Block`의 `ActivationBlockedTags` → **경직 동안 재블록 불가**
- `UACAttributeSet`의 `HitReactImmunityTags` → **가드 브레이크 몽타주가 히트리액트에 덮이지 않음**

> **Duration은 `GuardBreakMontage` 길이 이상**으로 잡아야 한다.
> 짧으면 몽타주 재생 도중 태그가 풀려, 같은 콤보의 다음 타격이 보내는 HitReact가 몽타주를 밀어낸다.

---

### GE_Player_GuardDecay — 상시 자연 회복

| | |
|---|---|
| **의도** | 안 맞고 버티면 가드 게이지가 서서히 회복된다 |
| **적용 시점** | 캐릭터 스폰 시 붙어 **상시 유지** |
| **연결 위치** | StartupData → `StartUpGameplayEffects` 배열 |

```
Duration Policy : Infinite
Period          : 0.1
Execute Periodic Effect on Application : false

Components → Target Tag Reqs (While GE is Active)
  Ongoing Tag Requirements
    Must Not Have Tags : Shared.Status.GuardDecayBlocked

Modifiers
  Attribute   : ACAttributeSet.GuardGauge
  Modifier Op : Add (Base)
  Magnitude   : Attribute Based
    Backing Attribute : ACAttributeSet.GuardGaugeRegenRate
    Coefficient       : -0.1        ← Period(0.1) × RegenRate = 초당 RegenRate 감소
    Snapshot          : false       ← 카드 버프를 즉시 반영하려면 꺼야 한다
```

---

### GE_Player_GuardDecayCooldown — 회복 유예

| | |
|---|---|
| **의도** | 막은 직후 잠시 회복을 막아, 연타를 계속 막으면 게이지가 쌓이기만 하게 한다 |
| **적용 시점** | 가드 부하를 받을 때마다 **재적용**(Duration 리셋) |
| **연결 위치** | StartupData → `GuardDecayDelayEffectClass` |
| **적용 주체** | `UACAttributeSet::HandleGuardDamage` |

```
Duration Policy : Has Duration (1.0)
Granted Tags    : Shared.Status.GuardDecayBlocked
Modifiers       : 없음
Stacking
  Stacking Type           : Aggregate by Target (또는 Stack Per Target)
  Stack Limit Count       : 1
  Stack Duration Refresh  : Refresh on Successful Application   ← 필수
```

> `Refresh on Successful Application`이 없으면 첫 블록 이후 1초 뒤 무조건 회복이 시작되어,
> 연타를 막아도 압박이 생기지 않는다.

---

## 3. Attribute 5종 (`UACAttributeSet`)

| Attribute | 초기값 | 설정 위치 | 카드 버프 |
|---|---|---|---|
| `GuardGauge` | 0 | (런타임 누적) | — |
| `MaxGuardGauge` | 1 (안전값) | **GE_Player_Init** | ✅ 최대치 증가 |
| `GuardBreakResistance` | 0 | GE_Player_Init (선택) | ✅ 부하 감쇄 |
| `GuardGaugeRegenRate` | 1 (안전값) | **GE_Player_Init** | ✅ 회복 속도 |
| `GuardDamageTaken` | 0 | (메타, 소비 후 리셋) | — |

> 생성자의 `1.f`는 0 나눗셈과 즉시 브레이크를 피하기 위한 **안전값**일 뿐이다.
> `GE_Player_Init`에 Modifier를 넣지 않으면 `MaxGuardGauge`가 1로 남아 **첫 블록에 바로 브레이크**가 난다.

---

## 4. 어빌리티 프로퍼티 (`GA_Player_Block` → `Block|GuardBreak`)

| 프로퍼티 | 기본값 | 의미 |
|---|---|---|
| `GuardDamageEffect` | — | 게이지를 채우는 GE. **비우면 가드 게이지 기능 전체가 꺼진다** |
| `GuardBreakAmountPerHit` | 20 | 일반 공격 1회 부하 |
| `GuardBreakWeightTag` | `Shared.Attack.Weight.Heavy` | 이 태그가 실린 공격은 무겁게 취급 |
| `GuardBreakHeavyAmount` | 60 | 강공격 1회 부하 |
| `GuardBreakMontage` | — | 가드 붕괴 시 재생. **비우면 브레이크가 발생하지 않는다** |
| `GuardBreakEffect` | — | 붕괴 시 적용할 경직 GE |

`GuardBreakAmountPerHit` / `GuardBreakHeavyAmount`는 Attribute가 아니라 어빌리티 설계값이다.
"이 공격이 얼마나 무거운가"는 캐릭터 스탯이 아니라 공격 쪽 데이터이기 때문이다.

---

## 5. 공격에 무게 태그 붙이기

보스 공격 몽타주의 `AN_IncomingAttackWarning` 노티파이에서:

```
Attack Defense Tags = { Shared.Attack.Blockable,       (기존)
                        Shared.Attack.Weight.Heavy }   (추가)
```

노티파이 단위이므로 **몽타주마다, 콤보 구간마다 다르게** 지정할 수 있다.
태그를 안 붙인 공격은 자동으로 `GuardBreakAmountPerHit`(일반량)로 처리된다.

---

## 6. 게이지가 오르지 않는 경우

| 상황 | 이유 |
|---|---|
| 패링 성공 | 완전 방어 — `OnSuccessfulBlockEventReceived`에서 분기가 갈린다 |
| 블록 실패 | `IsSuccessfulBlock`이 false → 이벤트 자체가 오지 않음 (뒤/측면, `Unblockable` 공격) |
| 사망 / 처형 / 체간 붕괴 중 | `HandleGuardDamage`에서 조기 반환 |
| 이미 `Player.Status.GuardBroken` | 중복 발동 방지 |

---

## 7. 밸런스 조정

기본값(임계 100 / 일반 20 / 강타 60 / 회복 25 / 유예 1초) 기준:

```
일반 공격만 : 5번 막으면 브레이크
강타만      : 2번 막으면 브레이크
섞이면      : 일반 2 + 강타 1 = 100 → 브레이크
```

| 증상 | 조정 |
|---|---|
| 너무 쉽게 뚫린다 | `MaxGuardGauge` ↑ 또는 `GuardBreakAmountPerHit` ↓ |
| 강공격이 안 무섭다 | `GuardBreakHeavyAmount` ↑ (90이면 두 방에 브레이크) |
| 압박이 없다 (너무 잘 회복) | `GuardGaugeRegenRate` ↓ 또는 유예 GE Duration ↑ |
| 경직이 짧다/길다 | `GE_Player_GuardBreak`의 Duration (단, 몽타주 길이 이상 유지) |

유예 GE Duration은 **보스 콤보의 타격 간격**과 비교해 정하면 좋다.
콤보 간격보다 길게 잡으면 "콤보 도중엔 절대 회복되지 않고, 콤보가 끝나야 회복된다"가 성립한다.

---

## 8. 연결 체크리스트

| 대상 | 연결 위치 |
|---|---|
| `GE_Player_GuardDamage` | `GA_Player_Block` → `Guard Damage Effect` |
| `GE_Player_GuardBreak` | `GA_Player_Block` → `Guard Break Effect` |
| `GE_Player_GuardDecay` | StartupData → `StartUpGameplayEffects` 배열 |
| `GE_Player_GuardDecayCooldown` | StartupData → `GuardDecayDelayEffectClass` |
| `MaxGuardGauge` / `GuardGaugeRegenRate` Modifier | `GE_Player_Init` |
| `GuardBreakMontage` / `GuardBreakWeightTag` | `GA_Player_Block` |
| `Shared.Attack.Weight.Heavy` | 보스 강공격 몽타주의 `AN_IncomingAttackWarning` |

---

## 9. 관련 태그

| 태그 | 부여 주체 | 용도 |
|---|---|---|
| `Player.Status.GuardBroken` | `GE_Player_GuardBreak` | 경직, 재블록 차단, HitReact 면역 |
| `Shared.Status.GuardDecayBlocked` | `GE_Player_GuardDecayCooldown` | 회복 유예 |
| `Shared.Attack.Weight.Heavy` | 공격 몽타주 노티파이 | 무거운 공격 표식 |
| `Shared.SetByCaller.GuardDamage` | 코드(`ApplyGuardDamage`) | 부하량 전달 |
| `Shared.Event.GuardBrokenTriggered` | `UACAttributeSet` | 붕괴 통보 이벤트 |

체간 시스템(`Shared.Status.PostureBroken`, `Shared.Status.PostureDecayBlocked`)과는
태그·Attribute·GE를 하나도 공유하지 않으므로 서로 독립적으로 튜닝할 수 있다.
