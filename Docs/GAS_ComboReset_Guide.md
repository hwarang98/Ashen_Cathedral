# 콤보 리셋 가이드

## 개요

공격 콤보 시스템은 `UACAbility_Attack`(공통 베이스)과 `UACPlayerAbility_Attack` / `UACEnemyAbility_Attack`(분리 구현)으로 구성됩니다.

콤보 리셋은 **정상 완료** 와 **취소** 두 가지 경로로 발생합니다.

---

## 콤보 카운트 관리 원칙

| 항목 | 설명 |
|------|------|
| 상태 변수 | `CurrentComboCount` (int32) |
| 인스턴싱 | `InstancedPerActor` — 콤보 상태를 액터 생존 주기 동안 유지 |
| 순환 | `CurrentComboCount >= AttackMontages.Num()` 이면 0으로 초기화 |

---

## 콤보 진행 흐름

```
ActivateAbility()
    │
    ├─ CurrentComboCount >= AttackMontages.Num() ?
    │       └─ Yes → CurrentComboCount = 0 (배열 끝 도달 시 자동 순환)
    │
    ├─ Shared_Status_CanCounterAttack 태그 보유?
    │       └─ Yes → CounterAttackMontage 재생, CurrentComboCount = 0
    │
    ├─ AttackMontages[CurrentComboCount] 재생
    │
    └─ CurrentComboCount++
```

---

## 리셋 경로 1 — 정상 완료 (타이머 리셋)

몽타주가 자연스럽게 끝나거나 블렌드아웃될 때의 흐름입니다.

```
OnMontageEnded()
    │
    ├─ EndAbility(bWasCancelled=false)   // task cleanup 완료
    │
    └─ HandleComboComplete()             // cleanup 이후 호출 (재진입 안전)
            │
            ├─ 기존 ComboResetTimerHandle 클리어 (SetTimer가 내부적으로 처리)
            └─ SetTimer(ComboResetDelay=2.0f초) → OnComboResetTimerExpired()
                    │
                    └─ ResetComboCount() → CurrentComboCount = 0
```

**핵심 (부분 콤보 이어치기):** 콤보 도중 입력을 멈췄다가 `ComboResetDelay`(기본값 2초) 안에 다시 공격하면 `CurrentComboCount`가 유지되어 중단된 지점부터 이어받습니다.
예) 1타 → 2타 → 1.5초 후 재입력 → 3타 (리셋 없이 이어치기)

풀 콤보(마지막 타) 완료 후에는 `ActivateAbility` 내부 순환 처리(`CurrentComboCount >= Num() → 0`)가 타이머와 무관하게 항상 1타로 돌아옵니다.

> **주의:** `HandleComboComplete`는 반드시 `EndAbility` 이후에 호출해야 합니다.
> GAS의 `Super::EndAbility` 내부 task cleanup 도중 `bIsActive`가 아직 `true`인 상태에서
> `OnMontageCancelled`가 재진입 호출될 수 있으며, 이 경우 `HandleComboCancelled → ClearTimer`가
> 방금 설정한 타이머를 지웁니다.

---

## 리셋 경로 2 — 취소 (즉시 리셋)

몽타주가 피격·닷지 등으로 중단될 때의 흐름입니다.

```
OnMontageCancelled()
    │
    └─ EndAbility(bWasCancelled=true)
            │
            └─ HandleComboCancelled()
                    │
                    ├─ ClearTimer(ComboResetTimerHandle)   // 진행 중인 타이머 정리
                    └─ ResetComboCount() → CurrentComboCount = 0
```

**핵심:** 취소 시에는 타이머 없이 즉시 `CurrentComboCount = 0`으로 초기화됩니다.

---

## 두 경로 비교

| 구분 | 트리거 | 타이머 | 결과 |
|------|--------|--------|------|
| 정상 완료 | 몽타주 블렌드아웃/완료 | `ComboResetDelay` 후 리셋 | 이어치기 창 제공 |
| 취소 | 피격·닷지 등 중단 | 없음 (즉시) | 콤보 즉시 초기화 |

---

## 자식 클래스 확장

`UACAbility_Attack`을 상속한 어빌리티에서 아래 두 함수를 오버라이드합니다.

```cpp
// 콤보 정상 완료 시 동작 정의
virtual void HandleComboComplete() override;

// 콤보 취소 시 동작 정의
virtual void HandleComboCancelled() override;
```

### 구현 예시

| 어빌리티 | HandleComboComplete | HandleComboCancelled |
|----------|---------------------|----------------------|
| `UACPlayerAbility_Attack` | 타이머 후 리셋, 패링 윈도우 활성화 (TODO) | ClearTimer → Super |
| `UACEnemyAbility_Attack` | 빈 구현 (BT가 콤보 제어) | Super (즉시 리셋) |

리셋이 필요하면 `ResetComboCount()`를 호출하세요.

---

## ComboResetDelay 조정

`UACPlayerAbility_Attack`의 `ComboResetDelay` 프로퍼티를 블루프린트 디폴트에서 수정할 수 있습니다.

```
블루프린트 에디터 → Class Defaults → Combo → ComboResetDelay (기본값: 2.0초)
```

값을 크게 할수록 마지막 콤보 이후 이어치기 가능 시간이 늘어납니다.
