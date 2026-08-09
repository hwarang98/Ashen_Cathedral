# GAS 자연 감소 지연(Decay Delay) 가이드

`UACDataAsset_StartupDataBase`의 `PostureDecayDelayEffectClass` / `GuardDecayDelayEffectClass` 두 프로퍼티가
무엇을 하고, 어떤 경로로 런타임까지 흘러가는지 정리한 문서입니다.

## 개요

체간(Posture)과 가드 게이지(GuardGauge)는 **상시 켜져 있는 주기형 GE**에 의해 계속 자연 감소합니다.
그런데 감소가 정말 "상시"라면, 맞는 도중에도 게이지가 줄어들어 압박이 성립하지 않습니다.

그래서 **"맞은 직후 N초간은 감소를 멈춘다"**는 유예 시간이 필요하고,
그 유예를 만드는 GE가 바로 이 두 프로퍼티가 가리키는 **Decay Delay(= Cooldown) GE**입니다.

핵심 설계는 태그 한 장으로 끝납니다.

- **감소 GE**(상시 Infinite)는 차단 태그를 갖고 있으면 꺼진다 — Ongoing Tag Requirement의 `IgnoreTags`
- **지연 GE**(HasDuration)는 그 차단 태그를 부여한다 — Granted Tags
- 피해를 받을 때마다 지연 GE를 **재적용**하면 Duration이 리셋되어 유예가 갱신된다

즉 이 두 프로퍼티는 **"어떤 GE를 유예 타이머로 쓸 것인가"를 캐릭터별로 지정하는 슬롯**입니다.
C++은 클래스를 모르고, 데이터 자산이 꽂아준 것을 그대로 적용하기만 합니다.

---

## 1. 데이터 흐름 — DataAsset → ASC → AttributeSet

```
[1. 에디터 설정]
   DA_PlayerStartup / DA_AshenKnightStartup / DA_Ordan_Startup ...
   · PostureDecayDelayEffectClass = GE_XXX_PostureDecayCooldown
   · GuardDecayDelayEffectClass   = GE_XXX_GuardDecayCooldown
   · StartUpGameplayEffects       에 GE_XXX_PostureDecay / GE_XXX_GuardDecay (상시 감소 GE)
         ↓
[2. 캐릭터 초기화 시 주입]
   UACDataAsset_StartupDataBase::GiveToAbilitySystemComponent
   → 어빌리티 부여 + StartUpGameplayEffects 적용(= 상시 감소 GE가 이때 켜짐)
   → InASCToGive->PostureDecayDelayEffectClass = PostureDecayDelayEffectClass
   → InASCToGive->GuardDecayDelayEffectClass   = GuardDecayDelayEffectClass
         ↓
[3. ASC가 클래스 레퍼런스를 보관]
   UACAbilitySystemComponent::PostureDecayDelayEffectClass / GuardDecayDelayEffectClass
   (UPROPERTY, 순수 저장소 — ASC는 이걸 스스로 쓰지 않는다)
         ↓
[4. 피해 시 AttributeSet이 꺼내 쓴다]
   UACAttributeSet::HandlePostureDamage / HandleGuardDamage
   → 게이지가 실제로 증가했을 때만 (ReducedDamage > 0)
   → ASC를 UACAbilitySystemComponent로 캐스팅
   → 클래스의 CDO를 얻어 ApplyGameplayEffectToSelf
         ↓
[5. 결과]
   Shared.Status.PostureDecayBlocked / Shared.Status.GuardDecayBlocked 태그가 N초간 부여됨
   → 상시 감소 GE의 Ongoing Tag Requirement에 걸려 감소가 멈춤
   → N초 뒤 지연 GE가 만료 → 태그 제거 → 감소 재개
```

관련 코드 위치:

| 역할 | 파일 |
|---|---|
| 프로퍼티 선언(에디터 노출) | `Public/DataAssets/Startup/ACDataAsset_StartupDataBase.h:47-53` |
| ASC로 주입 | `Private/DataAssets/Startup/ACDataAsset_StartupDataBase.cpp:26-30` |
| ASC 보관 슬롯 | `Public/GameplayAbilitySystem/ACAbilitySystemComponent.h:36-42` |
| Posture 재적용 | `Private/GameplayAbilitySystem/ACAttributeSet.cpp:307-319` |
| Guard 재적용 | `Private/GameplayAbilitySystem/ACAttributeSet.cpp:380-392` |
| 차단 태그 정의 | `Private/GameplayTags/ACGameplayTags_Shared.cpp:32-33` |

---

## 2. 왜 DataAsset → ASC 경유인가

`UACAttributeSet`은 GE 자산을 직접 들고 있을 수 없습니다. AttributeSet은 캐릭터마다 인스턴스가 생기지만
에디터에서 편집 가능한 데이터 자산이 아니고, 플레이어와 각 보스가 **서로 다른 유예 시간·GE**를 써야 하기 때문입니다.

그래서 "에디터에서 지정 가능한 곳(DataAsset) → 캐릭터마다 하나씩 있는 런타임 객체(ASC) → 계산 주체(AttributeSet)"
경로로 클래스 레퍼런스를 전달합니다.
이 패턴은 스태미나에도 동일하게 쓰이며(`StaminaRegenDelayEffectClass`, `ACDataAsset_PlayerStartupData`),
Posture/Guard는 **적도 함께 쓰기 때문에** 플레이어 전용이 아닌 **Base**에 선언되어 있습니다.

---

## 3. 실제 사용 코드

두 곳 모두 형태가 동일합니다.

```cpp
// UACAttributeSet::HandlePostureDamage — ACAttributeSet.cpp:307
// 체간 자연 감소 지연 타이머 리셋 — 실제로 게이지가 증가했을 때만,
// 마지막 피해 시점부터 유예시간 이후 감소가 재개된다.
// GE의 Stacking(Refresh on Successful Application)이 Duration을 자동 리셋하므로 재적용만으로 충분하다.
if (ReducedDamage > 0.f)
{
    if (UACAbilitySystemComponent* ACTargetASC = Cast<UACAbilitySystemComponent>(TargetASC))
    {
        if (ACTargetASC->PostureDecayDelayEffectClass)
        {
            const UGameplayEffect* DecayDelayGE =
                ACTargetASC->PostureDecayDelayEffectClass->GetDefaultObject<UGameplayEffect>();
            ACTargetASC->ApplyGameplayEffectToSelf(DecayDelayGE, 1, ACTargetASC->MakeEffectContext());
        }
    }
}
```

읽을 때 주의할 점 3가지:

1. **`ReducedDamage > 0.f` 가드가 핵심이다.**
   `PostureResistance` / `GuardBreakResistance`로 전부 감쇄되어 게이지가 안 오른 경우까지 유예를 갱신하면,
   저항 수치를 올릴수록 오히려 "감소가 계속 막히는" 역효과가 납니다.
2. **타이머를 직접 관리하지 않는다.**
   `FTimerHandle`이 아니라 GE의 Stacking 설정(Refresh Duration on Successful Application)에 리셋을 위임합니다.
   따라서 지연 GE의 스택 설정이 잘못되면 유예가 갱신되지 않고 첫 피격 기준으로만 동작합니다.
3. **미설정(nullptr)이면 조용히 스킵된다.**
   경고 로그가 없으므로, 캐릭터가 "맞는 도중에도 게이지가 줄어드는" 증상을 보이면
   해당 DA의 이 프로퍼티가 비어 있는지부터 확인해야 합니다.

또한 `Cast<UACAbilitySystemComponent>`가 실패하면(기본 `UAbilitySystemComponent`를 쓰는 액터) 역시 스킵됩니다.

---

## 4. 짝을 이루는 GE 2종

두 프로퍼티는 **단독으로 동작하지 않습니다.** 반드시 상시 감소 GE와 짝을 이뤄야 합니다.

### 감소 GE — `StartUpGameplayEffects`에 넣는 쪽

| | `GE_XXX_PostureDecay` | `GE_XXX_GuardDecay` |
|---|---|---|
| Duration Policy | Infinite | Infinite |
| Period | 주기 실행 | 주기 실행 |
| Modifier | `ACAttributeSet.Posture` AddBase | `ACAttributeSet.GuardGauge` AddBase |
| Magnitude | Attribute Based (`MaxPosture` 캡처, Target) | Attribute Based (`GuardGaugeRegenRate` 캡처, Target) |
| Ongoing Tag Requirement | Ignore Tags: `Shared.Status.PostureDecayBlocked` | Ignore Tags: `Shared.Status.GuardDecayBlocked` |

> 감소량 산식이 서로 다릅니다. Posture는 `MaxPosture` 기반(최대치 비례),
> Guard는 `GuardGaugeRegenRate` 기반(초당 고정량)입니다. 튜닝 지점이 다르니 혼동하지 마세요.

### 지연 GE — 이 문서의 두 프로퍼티에 꽂는 쪽

| | `GE_XXX_PostureDecayCooldown` | `GE_XXX_GuardDecayCooldown` |
|---|---|---|
| Duration Policy | Has Duration (= 유예 시간) | Has Duration |
| Granted Tags | `Shared.Status.PostureDecayBlocked` | `Shared.Status.GuardDecayBlocked` |
| Modifier | 없음 (태그 부여 전용) | 없음 |
| Stacking Type | Aggregate by Target | Aggregate by Target |
| Stack Limit | 1 | 1 |
| Refresh Duration on Successful Application | **필수 체크** | **필수 체크** |

즉 지연 GE는 **속성을 전혀 건드리지 않고 태그만 붙였다 떼는 스위치**입니다.
유예 시간을 조절하려면 이 GE의 Duration Magnitude를 바꾸면 됩니다.

---

## 5. 캐릭터별 실제 설정 현황

| Startup DataAsset | PostureDecayDelayEffectClass | GuardDecayDelayEffectClass |
|---|---|---|
| `DA_PlayerStartup` | `GE_Player_PostureDecayCooldown` | `GE_Player_GuardDecayCooldown` |
| `DA_AshenKnightStartup` | `GE_AshenKnight_PostureDecayCooldown` | 미설정 |
| `DA_Aldren_Startup` | `GE_AshenKnight_PostureDecayCooldown` (AshenKnight 것 공용) | 미설정 |
| `DA_Ordan_Startup` | `GE_Ordan_PostureDecayCooldown` | 확인 필요 — 프로퍼티는 직렬화돼 있으나 Guard 계열 GE 참조가 없음 |

- 적은 `GuardGauge`를 쓰지 않으므로 Guard 쪽이 비어 있는 것이 정상입니다.
- `DA_Ordan_Startup`은 `GuardDecayDelayEffectClass`가 기본값이 아닌 상태로 저장돼 있는데
  Guard GE를 참조하지 않습니다. Posture용 Cooldown GE가 잘못 꽂혔을 가능성이 있으니
  에디터에서 한 번 열어 확인하는 편이 좋습니다(적은 GuardGauge를 안 쓰므로 실피해는 없습니다).

---

## 6. 새 보스에 붙이는 절차

1. 보스 전용 `GE_보스명_PostureDecay` 생성 — Infinite + Period + `Posture` AddBase(음수), Ongoing Ignore Tags에 `Shared.Status.PostureDecayBlocked`
2. 보스 전용 `GE_보스명_PostureDecayCooldown` 생성 — HasDuration(유예 시간) + Granted Tag `Shared.Status.PostureDecayBlocked` + Stack 1 / Refresh on Apply
3. 보스 Startup DA에서
   - `StartUpGameplayEffects`에 1번 GE 추가
   - `PostureDecayDelayEffectClass`에 2번 GE 지정
4. 밸런스만 다르고 곡선이 같다면 AshenKnight 것을 그대로 재사용해도 됩니다(Aldren이 그렇게 하고 있습니다).

---

## 7. 증상별 점검표

| 증상 | 의심 지점 |
|---|---|
| 맞는 도중에도 게이지가 계속 줄어든다 | DA의 `*DecayDelayEffectClass` 미설정 / 감소 GE의 Ongoing Ignore Tags에 차단 태그 누락 |
| 첫 피격 이후 연타해도 유예가 안 늘어난다 | 지연 GE의 Stacking이 Refresh Duration on Successful Application이 아님 |
| 게이지가 한 번 차면 영영 안 줄어든다 | 지연 GE가 Infinite로 설정됨 / 감소 GE가 `StartUpGameplayEffects`에 없음 |
| 저항 수치를 올렸더니 감소가 더 안 된다 | `ReducedDamage > 0.f` 가드가 빠진 코드로 되돌아갔는지 확인 |
| 특정 캐릭터만 동작 안 한다 | ASC가 `UACAbilitySystemComponent` 파생인지 (`Cast` 실패 시 조용히 스킵) |

---

## 관련 문서

- `GAS_GuardBreak_Guide.md` — 가드 게이지 누적·붕괴 전체 흐름
- `GAS_Stamina_Setup_Guide.md` — 동일한 "Delay GE 주입" 패턴의 스태미나 버전
