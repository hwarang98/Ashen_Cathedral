# GAS 스태미나 시스템 설정 가이드

## 전체 구조

```
[행동 발생]
    │
    ├─ 스프린트 시작    → GE_SprintStaminaDrain 적용 (Infinite Periodic)
    │                      → 0.1초마다 StaminaCost += N
    │
    ├─ 공격 등 일회성  → GE_ConsumeStamina 적용
    │                      → StaminaCost += N (1회)
    │
    └─ StaminaCost 변화 감지 (PostGameplayEffectExecute)
           → Stamina -= StaminaCost
           → GE_StaminaRegenDelay 적용 (N초간 회복 차단)

[스태미나 회복]
    GE_StaminaRegen (Infinite Periodic, 항상 실행 중)
        → Player.Status.Stamina.RegenBlocked 태그가 없을 때만 작동
```

---

## GameplayEffect 목록

### 1. GE_StaminaRegen
> 스태미나 자연 회복. Startup Data에서 자동 적용되어 항상 실행 중.

| 항목 | 값 |
|---|---|
| Duration Policy | `Infinite` |
| Period | `0.1` |
| Modifier > Attribute | `Stamina` |
| Modifier > Modifier Op | `Add` |
| Modifier > Magnitude | `ScalableFloat` |
| **Ongoing Tag Requirements → Ignore Tags** | `Player.Status.Stamina.RegenBlocked` |

> ⚠️ `Application Tag Requirements`가 아닌 **`Ongoing Tag Requirements`** 사용. Application은 최초 적용 시에만 검사, Ongoing은 매 틱마다 검사.

**초당 회복량 조정:**

초당 회복량 = `Magnitude ÷ Period`

| 원하는 초당 회복량 | Period | Magnitude |
|---|---|---|
| 초당 5 회복 | 0.1 | 0.5 |
| 초당 10 회복 | 0.1 | **1.0** |
| 초당 20 회복 | 0.1 | 2.0 |

Period를 0.1로 고정하면 Magnitude만 조정하면 된다.
Period를 변경하고 싶다면 `Magnitude = 원하는 초당 회복량 × Period`로 계산한다.

---

### 2. GE_StaminaRegenDelay
> 스태미나 소비 직후 회복을 N초간 차단하는 GE.
> `ACAbilitySystemComponent.StaminaRegenDelayEffectClass`에 등록.

| 항목 | 값 |
|---|---|
| Duration Policy | `Has Duration` |
| Duration Magnitude | `ScalableFloat` |
| Modifier | 없음 |
| **Granted Tags** | `Player.Status.Stamina.RegenBlocked` |
| Stacking > Stacking Type | `Aggregate by Target` |
| Stacking > Stack Duration Refresh Policy | `Refresh on Successful Application` |

**딜레이 시간 조정:**

`Duration Magnitude`의 ScalableFloat 값을 변경한다.

| 원하는 딜레이 | Duration Magnitude |
|---|---|
| 1초 후 회복 시작 | 1.0 |
| 3초 후 회복 시작 | **3.0** |
| 5초 후 회복 시작 | 5.0 |

**Stacking 설정이 필요한 이유:**
연속으로 스태미나를 소비할 때(스프린트 중 공격 등) GE가 중복 적용되면 딜레이 타이머가 리셋되어야 한다.
`Refresh on Successful Application`이 없으면 첫 번째 딜레이가 끝나는 즉시 회복이 시작된다.

---

### 3. GE_ConsumeStamina
> 공격 등 일회성 스태미나 소비용 GE.

| 항목 | 값 |
|---|---|
| Duration Policy | `Instant` |
| Modifier > Attribute | `StaminaCost` *(Meta Attribute)* |
| Modifier > Modifier Op | `Add` |
| Modifier > Magnitude | `SetByCaller` |
| SetByCaller Tag | `Player.SetByCaller.StaminaCost` |

소비량은 GE 자체에 고정값이 없고, 어빌리티 C++ 코드에서 `SetByCallerTagMagnitude`로 호출 시마다 지정한다.

```cpp
// 어빌리티에서 사용 예시
FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(ConsumeStaminaEffectClass);
SpecHandle.Data->SetByCallerTagMagnitudes.Add(ACGameplayTags::Player_SetByCaller_StaminaCost, 20.f);
ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
```

| 소비량 | SetByCallerTagMagnitude 값 |
|---|---|
| 20 소비 | 20.f |
| 30 소비 | 30.f |
| 50 소비 | 50.f |

---

### 4. GE_SprintStaminaDrain
> 스프린트 중 주기적 스태미나 소비 GE.
> `GA_Sprint (UACAbility_Sprint)`의 `SprintStaminaDrainEffectClass`에 등록.

| 항목 | 값 |
|---|---|
| Duration Policy | `Infinite` |
| Period | `0.1` |
| Modifier > Attribute | `StaminaCost` *(Meta Attribute)* |
| Modifier > Modifier Op | `Add` |
| Modifier > Magnitude | `ScalableFloat` |

> ⚠️ Modifier Attribute는 `Stamina`가 아닌 **`StaminaCost`** (Meta Attribute)를 사용해야 한다.
> `Stamina`를 직접 감소시키면 Regen Delay가 트리거되지 않는다.

스프린트 시작 시 적용, 종료 시 `RemoveActiveGameplayEffect`로 제거됨.

**초당 소비량 조정:**

초당 소비량 = `Magnitude ÷ Period`

| 원하는 초당 소비량 | Period | Magnitude |
|---|---|---|
| 초당 5 소비 | 0.1 | 0.5 |
| 초당 10 소비 | 0.1 | **1.0** |
| 초당 20 소비 | 0.1 | 2.0 |

---

## Meta Attribute 패턴 (StaminaCost)

`StaminaCost`는 실제 값을 저장하지 않는 임시 메타 어트리뷰트.

```
GE에서 StaminaCost += N
    ↓
PostGameplayEffectExecute 감지 (ACAttributeSet.cpp → HandleStaminaConsumption)
    ↓
Stamina -= StaminaCost
StaminaCost = 0 (초기화)
GE_StaminaRegenDelay 적용
```

GE가 `Stamina`를 직접 건드리지 않고 `StaminaCost`를 경유하기 때문에,
스태미나 소비가 발생하는 모든 경우에 Regen Delay가 자동으로 트리거된다.

---

## DA_PlayerStartup 설정

| 필드 | 설정값 |
|---|---|
| Initial Gameplay Effects | `GE_StaminaRegen` 추가 |
| Stamina Regen Delay Effect Class | `GE_StaminaRegenDelay` |

---

## GA_Sprint 설정

| 필드 | 설정값 |
|---|---|
| Sprint Stamina Drain Effect Class | `GE_SprintStaminaDrain` |
| Sprint Speed | 500 |
| Sprint Stop Threshold | 50 |

> 적(Enemy) StartupData에서는 `Sprint Stamina Drain Effect Class`를 **None**으로 두면 스태미나 소비 없이 무한 스프린트.

---

## 관련 코드 위치

| 역할 | 파일 |
|---|---|
| StaminaCost 처리 | `ACAttributeSet.cpp` → `HandleStaminaConsumption` |
| RegenDelay GE 클래스 보관 | `ACAbilitySystemComponent.h` → `StaminaRegenDelayEffectClass` |
| RegenDelay GE 전달 | `ACDataAsset_PlayerStartupData.cpp` → `GiveToAbilitySystemComponent` |
| 스프린트 종료 시 RegenDelay 적용 | `ACAbility_Sprint.cpp` → `EndAbility` |
