# GAS 무기 데미지 시스템 가이드

## 전체 구조

```
[공격 어빌리티 활성화]
    │
    ├─ GetPlayerCurrentEquippedWeaponDamageAtLevel() 호출
    │      → DataTable(FACWeaponStatRow)에서 기본 데미지 읽기
    │
    ├─ GE_Damage 생성 + SetByCaller로 값 설정
    │      → Shared.SetByCaller.BaseDamage         = 무기 기본 데미지
    │      → Shared.SetByCaller.AttackType.Light   = 일반 콤보 단계 (일반 공격 시)
    │      → Shared.SetByCaller.AttackType.Heavy   = 강공격 콤보 단계 (강공격 시)
    │      → Shared.SetByCaller.CounterAttackBonus = 카운터 보너스 (카운터 시)
    │      → Shared.SetByCaller.GroggyDamage       = 그로기 데미지
    │
    └─ GE_Damage를 대상에게 적용
           ↓
    UACCalculation_DamageTaken::Execute_Implementation()
           → 콤보 배율 계산
           → AttackPower(Source) × (1 - DefensePower(Target), 최대 95%)
           → DamageTaken(메타) += FinalDamage
           → GroggyDamageTaken(메타) += GroggyDamage
           ↓
    UACAttributeSet::PostGameplayEffectExecute()
           ├─ DamageTaken 감지 → HandleDamageAndTriggerHitReact()
           │      → Health -= DamageDone
           │      → Health <= 0: Shared_Status_Dead 태그 추가 + Shared_Event_Death 발송
           │      → Health > 0:  Shared_Event_HitReact 발송
           │
           └─ GroggyDamageTaken 감지 → HandleGroggyDamage()
                  → GroggyGauge += (GroggyDamage - GroggyResistance)
                  → GroggyGauge >= Max: Shared_Event_GroggyTriggered 발송
```

---

## 데미지 계산식

콤보 단계는 **실제로 재생된 몽타주의 1-기반 단계**입니다. `AttackMontages` 길이를 넘지 않고,
피니셔(배열 마지막 타)가 끝나면 초기화되므로 배율이 무한히 누적되지 않습니다.
단계가 0이면(카운터 어택 / 단발성 스페셜 / 적 공격) 배율을 적용하지 않습니다.
자세한 단계 산출 규칙은 [플레이어 콤보 가이드](./GAS_ComboReset_Guide.md)를 참고하세요.

### 일반 공격
```
배율 = (콤보 단계 - 1) × 0.05 + 1.0
예) 1단계: 1.0x / 2단계: 1.05x / 3단계: 1.10x / 4단계: 1.15x
```

### 강공격
```
배율 = 콤보 단계 × 0.15 + 1.0
예) 1단계: 1.15x / 2단계: 1.30x / 3단계: 1.45x / 5단계: 1.75x
```

### 최종 데미지
```
FinalDamage = BaseDamage × 콤보배율 × AttackPower × (1 - DefensePower)
DefensePower 최대 0.95 (최소 5% 데미지 보장)
```

---

## DataTable 설정

### FACWeaponStatRow 필드 (DT_WeaponStat)

| 필드 | 설명 |
|---|---|
| `Damage` | 기본 데미지 (GetPlayerCurrentEquippedWeaponDamageAtLevel로 읽음) |
| `HeavyAttackGroggyDamage` | 강공격 그로기 데미지 |
| `CounterAttackGroggyDamage` | 카운터 그로기 데미지 |
| `AttackSpeed` | 공격 속도 배율 |
| `StaminaCost` | 스태미나 소비량 |

### DA_WeaponData 설정

| 필드 | 설정값 |
|---|---|
| `WeaponStatRow` > Data Table | `DT_WeaponStat` |
| `WeaponStatRow` > Row Name | 해당 무기의 Row 이름 |

---

## GameplayEffect 설정 (GE_Damage)

| 항목 | 값 |
|---|---|
| Duration Policy | `Instant` |
| Execution Calculations | `UACCalculation_DamageTaken` |
| Modifier | 없음 (계산은 ExecCalc에서 처리) |

---

## 공격 어빌리티에서 GE 적용 방법

```cpp
// 일반 공격 예시
FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass);

// 기본 데미지 설정 (DataTable에서 읽은 값)
SpecHandle.Data->SetSetByCallerMagnitude(
    ACGameplayTags::Shared_SetByCaller_BaseDamage,
    GetPlayerCurrentEquippedWeaponDamageAtLevel(InLevel)
);

// 콤보 단계 설정 (둘 중 해당하는 것만 설정, 나머지는 0)
// 실제 구현은 ACAbility_Attack::OnHitTarget이 GetComboDamageCount()로 값을 얻어 주입한다
SpecHandle.Data->SetSetByCallerMagnitude(
    ACGameplayTags::Shared_SetByCaller_AttackType_Light,
    GetComboDamageCount()  // 일반 공격
);

// 그로기 데미지 설정
SpecHandle.Data->SetSetByCallerMagnitude(
    ACGameplayTags::Shared_SetByCaller_GroggyDamage,
    GetPlayerCurrentWeaponHeavyGroggyDamage(InLevel)  // 강공격 시
);

// 대상에게 적용
ApplyGameplayEffectSpecToTarget(Handle, ActorInfo, ActivationInfo, SpecHandle, TargetDataHandle);
```

> ⚠️ SetByCaller 태그를 설정하지 않으면 `UACCalculation_DamageTaken`에서 0으로 읽히므로 반드시 명시적으로 설정해야 한다.

---

## Meta Attribute 패턴 (DamageTaken / GroggyDamageTaken)

`DamageTaken`과 `GroggyDamageTaken`은 실제 값을 저장하지 않는 임시 메타 어트리뷰트.

```
ExecCalc에서 DamageTaken += FinalDamage
    ↓
PostGameplayEffectExecute 감지 (ACAttributeSet.cpp → HandleDamageAndTriggerHitReact)
    ↓
Health -= DamageTaken
DamageTaken = 0 (초기화)
사망 / HitReact 이벤트 발송
```

GE가 `Health`를 직접 건드리지 않고 `DamageTaken`을 경유하기 때문에,
무적(Invincible), 사망(Dead) 상태 체크가 한 곳에서 일괄 처리된다.

---

## 상태 태그 면역

| 태그 | 효과 |
|---|---|
| `Shared.Status.Invincible` | DamageTaken을 0으로 초기화 (피해 무효) |
| `Shared.Status.Dead` | DamageTaken, GroggyDamageTaken 모두 무시 |
| `Shared.Status.SuperArmor` | HitReact 이벤트 발송 생략 (데미지는 들어감) |
| `Shared.Status.Groggy` | GroggyDamageTaken 무시 |

---

## 관련 코드 위치

| 역할 | 파일 |
|---|---|
| 무기 DataTable Row 조회 | `PlayerCombatComponent.cpp` → `GetCurrentWeaponStatRow` |
| 무기 기본 데미지 반환 | `PlayerCombatComponent.cpp` → `GetPlayerCurrentEquippedWeaponDamageAtLevel` |
| 무기 그로기 데미지 반환 | `PlayerCombatComponent.cpp` → `GetPlayerCurrentWeaponHeavyGroggyDamage` / `GetPlayerCurrentWeaponCounterGroggyDamage` |
| 데미지 계산 (ExecCalc) | `ACCalculation_DamageTaken.cpp` → `Execute_Implementation` |
| Attribute Capture 정의 | `ACStructTypes.h` → `FCADamageCapture` |
| 데미지 적용 및 이벤트 발송 | `ACAttributeSet.cpp` → `HandleDamageAndTriggerHitReact` |
| 그로기 처리 | `ACAttributeSet.cpp` → `HandleGroggyDamage` |
| 무기 DataTable Row 구조 | `ACDataTable_Weapon.h` → `FACWeaponStatRow` |
| 무기 DataAsset | `ACDataAsset_WeaponData.h` → `WeaponStatRow` |