# 플레이어 콤보 가이드

## 개요

공격 콤보 시스템은 `UACAbility_Attack`(공통 베이스)과 `UACPlayerAbility_Attack` / `UACEnemyAbility_Attack`(분리 구현)으로 구성됩니다.

플레이어 콤보의 핵심은 **어빌리티별 `AttackMontages` 배열 + 캐릭터에 있는 하나의 공유 콤보 상태**입니다.
약공격·강공격·스페셜이 각각 별도의 Gameplay Ability지만 콤보 단계는 `AACPlayerCharacter`가 들고 있어,
배열 길이가 서로 달라도 단계가 이어집니다.

적(Enemy)은 이 공유 상태를 쓰지 않습니다. `UACEnemyAbility_Attack`은 자체 `MontageSelectionMode`(Random / Sequential / ComboSequence)로 몽타주를 고르고 콤보 흐름은 StateTree가 제어합니다.

---

## 1. 상태 변수

### 공유 콤보 상태 — `AACPlayerCharacter`

| 변수 | 의미 |
|------|------|
| `SharedComboCount` | **다음 '일반' 단계의 0-기반 인덱스**. 몽타주를 고른 직후 `선택 인덱스 + 1`로 갱신 |
| `bSharedComboFinisherReady` | 다음 공격을 대상 배열의 **마지막 몽타주(피니셔)** 로 연결해야 함 |
| `bSharedComboFinisherPlaying` | 지금 재생 중인 몽타주가 피니셔임. 일반 콤보 체인을 차단 |
| `SharedComboResetTimerHandle` | 이어치기 유예 타이머 핸들 (어빌리티 간 공유) |
| `ComboResetDelay` | 이어치기 유예 시간 (기본 2.0초) |
| `ResetSharedComboState()` | 위 상태 + 타이머를 **한 번에** 초기화. 모든 리셋 경로가 이 함수만 호출 |

### 어빌리티 인스턴스 상태

| 변수 | 위치 | 의미 |
|------|------|------|
| `CurrentComboCount` | `UACAbility_Attack` | 활성화마다 증가하는 인스턴스 로컬 카운트. **적 전용**으로 남아 있음 |
| `SelectedComboStage` | `UACPlayerAbility_Attack` | 실제로 선택된 몽타주의 1-기반 단계. 데미지 계산에 전달 |
| `bComboChaining` | `UACPlayerAbility_Attack` | 체인 전환 중임을 표시해 `HandleComboCancelled`의 리셋을 건너뜀 |
| `bParticipatesInSharedCombo` | `UACPlayerAbility_Attack` | `false`면 공유 콤보 미참여 (단발성 스페셜) |

모든 공격 어빌리티는 `InstancedPerActor`라 활성화 사이에도 인스턴스 상태가 유지됩니다.

---

## 2. 몽타주 선택 규칙

`UACPlayerAbility_Attack::SelectAttackMontage()`

```
AttackMontages 비어 있음 → nullptr (ActivateAbility가 조기 종료)

bParticipatesInSharedCombo == false (스페셜)
    └─ AttackMontages 중 랜덤 1개 재생, 공유 상태 건드리지 않음, SelectedComboStage = 0

bParticipatesInSharedCombo == true (약/강공격)
    ├─ LastIndex = AttackMontages.Num() - 1
    │
    ├─ SelectedIndex = bSharedComboFinisherReady
    │                  ? LastIndex                                   // 피니셔 예약 소비
    │                  : Clamp(SharedComboCount, 0, LastIndex)       // 범위 밖이면 마지막 타
    │
    ├─ bSharedComboFinisherPlaying = (SelectedIndex == LastIndex)
    ├─ bSharedComboFinisherReady   = (SelectedIndex == LastIndex - 1)
    ├─ SharedComboCount            = SelectedIndex + 1
    └─ SelectedComboStage          = SelectedIndex + 1
```

인덱스는 항상 `[0, LastIndex]`로 Clamp되므로 범위 초과·음수가 발생하지 않습니다.
배열 길이가 1이면 `LastIndex - 1`이 `-1`이라 예약이 서지 않고, 그 몽타주가 항상 피니셔로 처리됩니다.

### 단계 전환 예시 (약 4개 / 강 5개)

| 입력 | 진입 시 Ready | 선택 | 재생 | 이후 Playing / Ready / Shared |
|---|---|---|---|---|
| 약 | false | `Clamp(0)` = 0 | 약1 | false / false / 1 |
| → 강 | false | `Clamp(1)` = 1 | **강2** | false / false / 2 |
| 약,약,약 | — | 0,1,2 | 약1,약2,**약3** | false / **true** (2 == 4-2) / 3 |
| → 강 | **true** | `LastIndex` = 4 | **강5 피니셔** | **true** / false / 5 |
| 강×3 → 약 | false | `Clamp(3)` = 3 | **약4 피니셔** | **true** / false / 4 |
| 강×4 → 약 | **true** | `LastIndex` = 3 | **약4 피니셔** | **true** / false / 4 |

핵심은 **길이가 다른 배열 사이를 오갈 때 단순 Clamp만으로는 피니셔에 도달하지 못한다**는 점입니다.
약3타(피니셔 직전)에서 강으로 넘어가면 `Clamp(3)`은 강4타를 고르지만, `bSharedComboFinisherReady` 덕분에 강5타 피니셔로 연결됩니다.

---

## 3. 연계 구간 (Window) — 두 종류

몽타주에 배치하는 AnimNotifyState가 ASC에 Loose GameplayTag를 부여하는 구간에서만 다음 공격으로 이어집니다.

| | ComboWindow | SpecialLinkWindow |
|---|---|---|
| NotifyState | `UACAnimNotifyState_ComboWindow` | `UACAnimNotifyState_SpecialLinkWindow` |
| 표시 이름 | Combo Window | Special Link Window |
| 태그 | `Player.Status.ComboWindow` | `Player.Status.SpecialLinkWindow` |
| 배치 위치 | 일반 공격 몽타주 | **피니셔 몽타주 후반(후딜) — 권장 0.2~0.4초** |
| 약/강 연계 | 허용 | 허용 안 함 |
| 스페셜 연계 | 허용 (일반 공격 중일 때) | 허용 (**피니셔 중일 때 유일한 경로**) |

두 태그 모두 `NotifyBegin`에서 추가, `NotifyEnd`에서 제거됩니다.
몽타주가 중단돼도 `UAnimInstance`가 활성 NotifyState에 `NotifyEnd`를 호출하므로(`TriggerAnimNotifies` / `EndNotifyStates`) 별도의 방어적 태그 제거 경로는 두지 않습니다.

---

## 4. 입력 라우팅

`AACPlayerCharacter::Input_AbilityInputPressed()`

```
현재 활성 중인 UACPlayerAbility_Attack 이 있는가?
│
├─ 있음 + 스페셜 입력 (InputTag.SpecialWeaponAbility.*)
│     └─ TryTriggerSpecialAttack()
│            ├─ 피니셔 재생 중 → SpecialLinkWindow 필요
│            └─ 일반 공격 중   → ComboWindow 필요
│        실패해도 일반 활성화 경로로 넘기지 않음 (진행 중인 공격 유지)
│
├─ 있음 + 약/강 입력
│     └─ TriggerComboChain()
│            ├─ 피니셔 재생 중이면 거부
│            └─ ComboWindow 필요
│
└─ 없음 (중립 상태)
      └─ ACAbilitySystemComponent::OnAbilityInputPressed() — 기존 단독 발동 경로
```

스페셜 입력 판정은 상위 태그 `InputTag.SpecialWeaponAbility` 하나로 처리합니다 (`UACPlayerAbility_Attack::IsSpecialAttackInputTag`).
하위 `LightAttack` / `RightAttack` / `CombinedAttack`이 모두 자동 포함되므로 태그를 추가해도 코드 수정이 필요 없습니다.

### 스페셜 전환 순서 (`TryTriggerSpecialAttack`)

```
① IsActive / bParticipatesInSharedCombo / 스페셜 입력 여부
② 상황에 맞는 Window 태그 확인
③ InputTag를 Dynamic Spec Source Tag로 가진 '비활성' Spec 검색
     (UACAbilitySystemComponent::FindInactiveAbilitySpecByInputTag)
④ 어빌리티 종료 '전' 사전 검증 — CheckCooldown / CheckCost / 스태미나 > 0
     ↑ 여기까지 실패하면 EndAbility를 호출하지 않으므로 진행 중인 피니셔가 끊기지 않는다
⑤ EndAbility(bWasCancelled=true) → HandleComboCancelled → ResetSharedComboState()
⑥ TryActivateAbility(SpecialHandle)
```

> **왜 `CanActivateAbility`로 한 번에 검사하지 않는가**
> 현재 공격이 활성 중이면 `BlockAbilitiesWithTag`(`Player.Ability.Attack.Light`)가 ASC에 걸려 있어
> `DoesAbilitySatisfyTagRequirements`가 항상 실패합니다. 그래서 블록 태그와 무관하게 판정할 수 있는
> 쿨다운·비용·스태미나만 사전 검사하고, 나머지는 활성화 실패 후 안전한 정리로 처리합니다.

---

## 5. 리셋 경로

| 트리거 | 처리 | 결과 |
|--------|------|------|
| 일반 공격 자연 완료 / 소프트 캔슬 | `HandleComboComplete` → `SetTimer(ComboResetDelay)` | 이어치기 창 제공 |
| **피니셔** 완료 | `HandleComboComplete` → `ResetSharedComboState()` | 지연 없이 즉시 초기화 |
| **스페셜** 완료 | 동일 (`bParticipatesInSharedCombo == false`) | 지연 없이 즉시 초기화 |
| 강제 취소 (피격·닷지 등) | `EndAbility(cancelled)` → `HandleComboCancelled` → `ResetSharedComboState()` | 즉시 초기화 |
| 리셋 타이머 만료 | `OnComboResetTimerExpired` → `ResetSharedComboState()` | 즉시 초기화 |
| 체인 후 다음 어빌리티 활성화 실패 | `TriggerComboChain` 말미에서 정리 | 예약된 피니셔가 남지 않음 |
| **새 공격이 Commit에 성공** | `ActivateAbility`에서 기존 타이머 `ClearTimer` | 이전 타이머가 이번 몽타주 도중 만료되지 않음 |

체인 전환 중(`bComboChaining == true`)에는 `HandleComboCancelled`가 콤보 단계를 유지하고 타이머만 취소합니다.

```
정상 완료                              취소
OnMontageEnded()                       OnMontageCancelled()
  └─ EndAbility(cancelled=false)         └─ EndAbility(cancelled=true)
  └─ HandleComboComplete()                    └─ HandleComboCancelled()
        ├─ 피니셔/스페셜 → 즉시 리셋                ├─ 체인 중 → 타이머만 취소
        └─ 그 외 → 리셋 타이머                      └─ 그 외 → ResetSharedComboState + Super
```

> **주의:** `HandleComboComplete`는 반드시 `EndAbility` 이후에 호출해야 합니다.
> `Super::EndAbility` 내부 task cleanup 도중 `bIsActive`가 아직 `true`인 상태로
> `OnMontageCancelled`가 재진입할 수 있고, 그러면 방금 건 타이머가 지워집니다.

---

## 6. 콤보 데미지 단계

데미지 계산기(`ACCalculation_DamageTaken`)에 전달하는 콤보 횟수는
`UACAbility_Attack::GetComboDamageCount()`가 결정합니다.

| 클래스 | 반환값 |
|--------|--------|
| `UACAbility_Attack` (기본) | `CurrentComboCount` |
| `UACPlayerAbility_Attack` | `SelectedComboStage` — **실제로 선택된 몽타주 단계** |
| `UACEnemyAbility_Attack` | 오버라이드 없음. 애초에 `bApplyComboDamageBonus = false`라 주입 자체가 없음 |

플레이어가 약↔강을 섞어도 데미지 단계가 재생된 몽타주와 일치하고, 피니셔 이후에는 단계가 초기화되므로 배율이 무한히 누적되지 않습니다. 카운터 어택은 `SelectedComboStage = 0`이라 콤보 배율이 붙지 않습니다(카운터 전용 배율만 적용).

배율 공식은 [GAS 공격 흐름 가이드](./GAS_AttackFlow_Guide.md) 3장을 참고하세요.

---

## 7. 에디터 설정

### 약/강공격 Gameplay Ability

```
Class Defaults → Montage → Attack Montages
```
- **배열 순서 = 콤보 순서**, **마지막 원소 = 피니셔**
- `Combo → Participates In Shared Combo` **체크 유지**

### 스페셜 Gameplay Ability

```
Class Defaults → Combo → Participates In Shared Combo  ← 체크 해제
```
- 체크를 해제해야 단발 공격으로 동작합니다. 켜둔 채로 두면 스페셜 몽타주가 콤보 단계로 해석되고 콤보 데미지 보너스가 붙습니다.
- `AttackMontages`에 여러 개를 넣으면 **매번 랜덤으로 하나**가 재생됩니다 (콤보 단계 아님).

### 몽타주

| 대상 | 배치할 NotifyState |
|------|--------------------|
| 약/강 일반 타 | `Combo Window` (타격 판정 이후 연계 허용 구간) |
| 약/강 **피니셔**(배열 마지막) | `Special Link Window` (타격 이후 후딜, 0.2~0.4초 권장) |
| 스페셜 | 없음 |

피니셔에 `Combo Window`가 남아 있어도 코드가 약/강 체인을 막지만, 의도를 명확히 하려면 제거를 권장합니다.

### 이어치기 유예 시간

```
BP_PlayerCharacter → Class Defaults → Combat|Combo → Combo Reset Delay (기본 2.0초)
```
어빌리티가 아니라 **플레이어 캐릭터**에 있습니다 (모든 공격 어빌리티가 공유하기 때문).

---

## 8. 관련 파일

| 역할 | 파일 |
|------|------|
| 공유 콤보 상태 | `Character/Player/ACPlayerCharacter.h / .cpp` |
| 공통 공격 어빌리티 | `GameplayAbilitySystem/Abilities/Common/ACAbility_Attack.h / .cpp` |
| 플레이어 공격 어빌리티 | `GameplayAbilitySystem/Abilities/Player/ACPlayerAbility_Attack.h / .cpp` |
| 적 공격 어빌리티 | `GameplayAbilitySystem/Abilities/Enemy/ACEnemyAbility_Attack.h / .cpp` |
| Spec 검색 헬퍼 | `GameplayAbilitySystem/ACAbilitySystemComponent.h / .cpp` |
| 콤보 연계 구간 | `Animation/AnimNotify/ACAnimNotifyState_ComboWindow.h / .cpp` |
| 스페셜 연계 구간 | `Animation/AnimNotify/ACAnimNotifyState_SpecialLinkWindow.h / .cpp` |
| 상태 태그 | `GameplayTags/ACGameplayTags_Player.h / .cpp` |
| 입력 태그 | `GameplayTags/ACGameplayTags_Input.h / .cpp` |
