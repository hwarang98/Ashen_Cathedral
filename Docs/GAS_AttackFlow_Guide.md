# GAS 공격 흐름 가이드

## 개요

이 문서는 플레이어 및 적(Ashen Knight 포함)의 공격이 GAS 파이프라인을 통해
최종 데미지·그로기·화상으로 반영되는 전체 흐름을 설명합니다.

---

## 1. 공통 공격 흐름 (플레이어 / 적 공통)

```
[무기 충돌 감지]
   UBoxComponent::OnComponentBeginOverlap
         ↓
   ACWeaponBase::OnCollisionBoxBeginOverlap
   → 적대 대상 여부 확인 (bIgnoreFriendly)
   → OnWeaponHitTarget.ExecuteIfBound(OtherActor)
         ↓
   PawnCombatComponent::OnHitTargetActor
   → Shared.Event.MeleeHit 이벤트 발송
         ↓
   ACAbility_Attack::OnHitTarget (WaitGameplayEvent 수신)
   → Spec 생성 (GE_DamageEffect)
   → SetByCaller 값 주입
   → ModifyDamageSpec 호출 (서브클래스 확장 포인트)
   → ApplyGameplayEffectSpecToTarget
         ↓
   ACCalculation_DamageTaken::Execute_Implementation
   → SetByCallerTagMagnitudes 순회하며 값 수집
   → 최종 데미지 계산
   → Output Modifier 출력 (DamageTaken, GroggyDamageTaken, BurnAccumulation)
         ↓
   ACAttributeSet::PostGameplayEffectExecute
   → Health 감소 / HitReact / Death 처리
   → GroggyGauge 누적 / Groggy 발동
   → BurnGauge 누적 / BurnDoT 발동
```

---

## 2. Spec에 주입되는 SetByCaller 태그 목록

| 태그 | 주입 위치 | 설명 |
|------|-----------|------|
| `Shared.SetByCaller.BaseDamage` | `ACAbility_Attack::OnHitTarget` | 무기 DataTable 기본 데미지 |
| `Shared.SetByCaller.AttackType.Light` | `ACAbility_Attack::OnHitTarget` | 일반 공격 콤보 횟수 (플레이어 전용) |
| `Shared.SetByCaller.AttackType.Heavy` | `ACAbility_Attack::OnHitTarget` | 강공격 콤보 횟수 (플레이어 전용) |
| `Shared.SetByCaller.CounterAttackBonus` | `ACAbility_Attack::OnHitTarget` | 카운터 공격 보너스 배율 |
| `Shared.SetByCaller.GroggyDamage` | `ACAbility_Attack::OnHitTarget` | 그로기 게이지 누적량 |
| `Shared.SetByCaller.FireBonusDamage` | `ACEnemyAbility_Attack::ModifyDamageSpec` | Phase2 화염 추가 데미지 (적 전용) |
| `Shared.SetByCaller.BurnBuildUp` | `ACEnemyAbility_Attack::ModifyDamageSpec` | Phase2 화상 게이지 누적량 (적 전용) |

---

## 3. 데미지 계산식 (ACCalculation_DamageTaken)

```
// 콤보 배율 적용 (플레이어 전용)
BaseDamage × ((콤보횟수 - 1) × 0.05 + 1.0)   // 일반 공격
BaseDamage × (콤보횟수 × 0.15 + 1.0)          // 강공격

// 최종 데미지
FinalDamage = (BaseDamage + FireBonusDamage) × AttackMultiplier × (1.0 - DefenseMultiplier)

// 카운터 공격 시
FinalDamage × CounterAttackBonus

// 방어율 상한: 95%
DefenseMultiplier = Clamp(DefensePower, 0.0, 0.95)
```

---

## 4. AttributeSet 처리 흐름

### 4-1. 데미지 처리 (HandleDamageAndTriggerHitReact)

```
DamageTaken 메타 어트리뷰트 감지
   ├─ Shared.Status.Invincible 태그 보유 → DamageTaken = 0, 종료
   ├─ Shared.Status.Dead 태그 보유 → DamageTaken = 0, 종료
   └─ 정상 처리
         → Health -= DamageTaken
         → Health <= 0: Shared.Event.Death 발송
         → Health >  0: Shared.Event.HitReact 발송
                           (Invincible, SuperArmor 태그 보유 시 HitReact 생략)
```

### 4-2. 그로기 처리 (HandleGroggyDamage)

```
GroggyDamageTaken 메타 어트리뷰트 감지
   ├─ Dead / Groggy / Invincible / SuperArmor 태그 보유 → 누적 생략
   └─ 정상 처리
         → ReducedDamage = Max(GroggyDamage - GroggyResistance, 0)
         → GroggyGauge += ReducedDamage
         → GroggyGauge >= MaxGroggyGauge
               → Shared.Event.GroggyTriggered 발송 → GA_Groggy 발동
```

### 4-3. 화상 처리 (HandleBurnBuildUp) — Phase2 전용

```
BurnAccumulation 메타 어트리뷰트 감지
   ├─ Dead 태그 보유 → 누적 생략
   └─ 정상 처리
         → BurnGauge += BurnBuildUp
         → BurnGauge >= MaxBurnGauge (기본 100)
               → BurnGauge = 0 (리셋)
               → Shared.Event.BurnTriggered 발송 → GA_BurnDoT 발동
```

---

## 5. Phase2 추가 흐름 (Ashen Knight 전용)

Phase2 진입 조건: `GA_AshenKnight_Phase2` 활성화 → ASC에 `Enemy.State.Phase2` 태그 부여

```
[Phase2 진입]
   GA_AshenKnight_Phase2::ActivateAbility
         ↓
   ① Phase2StatsEffect 즉시 적용 (Infinite GE — 스탯 영구 강화)
         ↓
   ② AI 이동 정지 (StopMovement + BrainComponent::PauseLogic)
      무적 태그 부여 (Shared.Status.Invincible)
         ↓
   ③ Phase2TransitionMontage 재생
      WaitGameplayEvent: Enemy.Event.Phase2.VisualActivate 대기
         ↓
   ④ AnimNotify_SendGameplayEvent 발송 (몽타주 특정 시점)
      → OnVisualActivateEventReceived 호출
            → 캐릭터 메시 화염 머티리얼 교체 (슬롯 0~4)
            → 무기 머티리얼 교체
            → 캐릭터 Niagara 이펙트 부착
            → 무기 Niagara 이펙트 부착
         ↓
   ⑤ 몽타주 종료 (OnPhase2MontageEnded)
      → 무적 태그 제거 (Shared.Status.Invincible)
      → BrainComponent::ResumeLogic (AI 재개)
         ↓
   ⑥ EndAbility 호출 안 함
      → Enemy.State.Phase2 태그 영구 유지
      → Phase2 공격 어빌리티에서 화염/화상/그로기 강화 자동 적용
```

### Phase2 공격 강화 (ModifyDamageSpec)

```
ACEnemyAbility_Attack::ModifyDamageSpec
   ├─ Enemy.State.Phase2 태그 없으면 → 즉시 반환 (Phase1 그대로)
   └─ Phase2 태그 있으면
         → FireBonusDamageMultiplierCurve.GetValueAtLevel(난이도)
               → Spec에 FireBonusDamage 추가 주입
         → BurnBuildUpAmountCurve.GetValueAtLevel(난이도)
               → Spec에 BurnBuildUp 추가 주입
         → GroggyDamageMultiplierCurve.GetValueAtLevel(난이도)
               → 기존 GroggyDamage × 배율로 덮어씀
```

---

## 6. HitReact 흐름 (적 전용)

```
Shared.Event.HitReact 이벤트 수신
         ↓
ACEnemyGameplayAbility_HitReact::ActivateAbility
   → 공격자와의 각도 계산 (ComputeHitReactDirectionTag)
         ├─ Front / Left / Right / Back 방향 판별
         └─ 해당 방향 몽타주 선택
   → UnderAttackEffect 적용 (Enemy.Status.UnderAttack 태그 부여)
   → 몽타주 재생
         ↓
   몽타주 종료
   → EndAbility → UnderAttackEffect 제거 (태그 소멸)
```

---

## 7. 관련 파일 목록

| 역할 | 파일 |
|------|------|
| 공통 공격 어빌리티 | `ACAbility_Attack.h / .cpp` |
| 적 공격 어빌리티 | `ACEnemyAbility_Attack.h / .cpp` |
| Phase2 진입 어빌리티 | `ACGameplayAbility_AshenKnight_Phase2.h / .cpp` |
| 데미지 계산 | `ACCalculation_DamageTaken.h / .cpp` |
| 어트리뷰트 처리 | `ACAttributeSet.h / .cpp` |
| 무기 충돌 | `ACWeaponBase.h / .cpp` |
| 히트리액트 | `ACEnemyGameplayAbility_HitReact.h / .cpp` |
| 공유 게임플레이 태그 | `ACGameplayTags_Shared.h / .cpp` |
| 적 게임플레이 태그 | `ACGameplayTags_Enemy.h / .cpp` |
| SetByCaller / 캡처 구조체 | `ACStructTypes.h` |
