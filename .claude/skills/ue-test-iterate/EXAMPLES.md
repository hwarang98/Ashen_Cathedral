# UE Test Iterate — Examples

4 difficulty scenarios matching acceptance criteria, from single-function to multi-system integration.

---

## Example 1: Easy — Single Function, 3 Edge Cases

**Scenario**: Test `CalculateDamage()` with 3 consecutive clean passes.

```bash
/ue-test-iterate CalculateDamage 3
```

### Iteration Log

```text
=== UE Test-Iterate Results ===

Target: CalculateDamage
Type: bdd_spec
Goal: 3 consecutive clean passes

--- Initialization ---
Analyzing target via ue_analyze_symbols(get_methods)...
Found: float UMyDamageLibrary::CalculateDamage(float BaseDamage, float Multiplier, EDamageType Type)
Test output: Source/MyGame/Tests/CalculateDamage_EdgeCaseTest.cpp

[Iter 1] Category: boundary_values
  Generated 3 tests:
    - CalculateDamage_ZeroBaseDamage → PASS
    - CalculateDamage_MaxFloatDamage → PASS
    - CalculateDamage_NegativeMultiplier → PASS
  CLEAN (3 tests, streak: 1/3)

[Iter 2] Category: null_empty
  Generated 3 tests:
    - CalculateDamage_ZeroMultiplier → PASS
    - CalculateDamage_DefaultEnumValue → PASS
    - CalculateDamage_MinFloatDamage → PASS
  CLEAN (3 tests, streak: 2/3)

[Iter 3] Category: type_boundaries
  Generated 3 tests:
    - CalculateDamage_IntToFloatPrecision → PASS
    - CalculateDamage_EnumOutOfRange → PASS
    - CalculateDamage_VerySmallMultiplier → PASS
  CLEAN (3 tests, streak: 3/3)

Result: SUCCESS (3/3 clean passes)
Total: 3 iterations, 0 bugs found, 9 edge cases, 2m 10s
Test file: Source/MyGame/Tests/CalculateDamage_EdgeCaseTest.cpp
```

### Generated Test (BDD Spec)

```cpp
BEGIN_DEFINE_SPEC(FCalculateDamage_EdgeCase, "MyGame.Damage.CalculateDamage.EdgeCase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FCalculateDamage_EdgeCase)

void FCalculateDamage_EdgeCase::Define()
{
    Describe("Boundary Values", [this]()
    {
        It("Should handle zero base damage", [this]()
        {
            float Result = UMyDamageLibrary::CalculateDamage(0.0f, 1.5f, EDamageType::Physical);
            TestEqual(TEXT("Zero damage"), Result, 0.0f);
        });

        It("Should handle MAX_FLT base damage without overflow", [this]()
        {
            float Result = UMyDamageLibrary::CalculateDamage(MAX_FLT, 1.0f, EDamageType::Physical);
            TestTrue(TEXT("No NaN"), !FMath::IsNaN(Result));
            TestTrue(TEXT("No Inf"), FMath::IsFinite(Result));
        });

        It("Should handle negative multiplier", [this]()
        {
            float Result = UMyDamageLibrary::CalculateDamage(100.0f, -1.0f, EDamageType::Fire);
            TestTrue(TEXT("Non-negative result"), Result >= 0.0f);
        });
    });
}
```

---

## Example 2: Normal — Class Methods, 10 Clean Passes (Bug Found)

**Scenario**: Test `AMyCharacter` across all methods, 10 consecutive clean passes.

```bash
/ue-test-iterate AMyCharacter 10
```

### Iteration Log

```text
=== UE Test-Iterate Results ===

Target: AMyCharacter
Type: bdd_spec
Goal: 10 consecutive clean passes

--- Initialization ---
Analyzing target via ue_analyze_symbols(get_methods)...
Found 8 methods: TakeDamage, Heal, ApplyBuff, RemoveBuff, GetHealth,
                  SetMaxHealth, IsAlive, Die
Test output: Source/MyGame/Tests/AMyCharacter_EdgeCaseTest.cpp

[Iter  1] Category: boundary_values
  Generated 5 tests across TakeDamage, Heal, SetMaxHealth:
    - TakeDamage_ZeroDamage → PASS
    - TakeDamage_ExactlyMaxHP → PASS
    - Heal_ZeroAmount → PASS
    - Heal_OverMaxHP → PASS
    - SetMaxHealth_Zero → PASS
  CLEAN (5 tests, streak: 1/10)

[Iter  2] Category: null_empty
  Generated 5 tests:
    - TakeDamage_NullInstigator → FAIL
      → Assertion: Access violation reading nullptr DamageCauser
    - ApplyBuff_EmptyBuffTag → PASS
    - RemoveBuff_NonExistentBuff → PASS
    - TakeDamage_NullDamageType → PASS
    - Die_AlreadyDead → PASS
  BUG FOUND (streak reset → 0)
  → Fix: Add nullptr check for DamageCauser in TakeDamage() at line 142
  → Regression check: re-running all 10 tests... ALL PASS

[Iter  3] Category: null_empty (retry after fix)
  Generated 5 new null/empty tests:
    - TakeDamage_NullInstigator_AfterFix → PASS
    - TakeDamage_NullController → PASS
    - ApplyBuff_NullAbilitySpec → PASS
    - Heal_NullSource → PASS
    - GetHealth_BeforeBeginPlay → PASS
  CLEAN (5 tests, streak: 1/10)

[Iter  4] Category: type_boundaries
  CLEAN (4 tests, streak: 2/10)

[Iter  5] Category: unicode_encoding
  CLEAN (3 tests, streak: 3/10)

[Iter  6] Category: large_inputs
  CLEAN (3 tests, streak: 4/10)

[Iter  7] Category: concurrent_access
  CLEAN (4 tests, streak: 5/10)

[Iter  8] Category: error_paths
  CLEAN (4 tests, streak: 6/10)

[Iter  9] Category: overflow_underflow
  CLEAN (3 tests, streak: 7/10)

[Iter 10] Category: state_transitions
  CLEAN (4 tests, streak: 8/10)

[Iter 11] Category: format_variations
  CLEAN (3 tests, streak: 9/10)

[Iter 12] Category: resource_exhaustion
  CLEAN (2 tests, streak: 10/10)

Result: SUCCESS (10/10 clean passes)
Total: 12 iterations, 1 bug fixed, 50 edge cases, 14m 30s
Bug fixed: nullptr DamageCauser access in TakeDamage() (line 142)
Test file: Source/MyGame/Tests/AMyCharacter_EdgeCaseTest.cpp
```

---

## Example 3: Hard — GAS Ability Flow, 5 Clean Passes

**Scenario**: Test `GA_Attack` ability through Activate → ApplyEffect → End flow.

```bash
/ue-test-iterate GA_Attack --clean-passes 5
```

### Iteration Log

```text
=== UE Test-Iterate Results ===

Target: GA_Attack (GameplayAbility)
Type: bdd_spec
Goal: 5 consecutive clean passes

--- Initialization ---
Analyzing target via ue_analyze_symbols + ue_manage_gameplay...
Found ability: UGA_Attack (inherits UGameplayAbility)
Key methods: ActivateAbility, ApplyGameplayEffectToTarget,
             EndAbility, CanActivateAbility, CommitAbility
Associated tags: Ability.Attack, Ability.Attack.Melee
Test output: Source/MyGame/Tests/GA_Attack_EdgeCaseTest.cpp

[Iter 1] Category: boundary_values
  Generated 9 tests (3 per phase: Activate, ApplyEffect, End):
  Activate:
    - ActivateAbility_ZeroCooldown → PASS
    - ActivateAbility_MaxLevelAbility → PASS
    - ActivateAbility_MinCostValues → PASS
  ApplyEffect:
    - ApplyEffect_ZeroDamage → PASS
    - ApplyEffect_MaxStackCount → PASS
    - ApplyEffect_ZeroDuration → PASS
  End:
    - EndAbility_Cancelled → PASS
    - EndAbility_WhileApplyingEffect → PASS
    - EndAbility_AlreadyEnded → PASS
  CLEAN (9 tests, streak: 1/5)

[Iter 2] Category: null_empty
  Generated 6 tests:
    - ActivateAbility_NullTargetData → FAIL
      → Crash in GetTargetActor() — expects valid TargetDataHandle
  BUG FOUND (streak reset → 0)
  → Fix: Guard GetTargetActor() with TargetDataHandle.IsValid() check
  → Regression check: 15 tests... ALL PASS

[Iter 3] Category: null_empty (retry)
  CLEAN (6 tests, streak: 1/5)

[Iter 4] Category: state_transitions
  Generated 5 tests:
    - ActivateAbility_WhileOnCooldown → PASS
    - ActivateAbility_DuringAnotherAbility → PASS
    - EndAbility_BeforeCommit → PASS
    - ApplyEffect_AfterOwnerDied → PASS
    - ActivateAbility_DuringEndAbility → PASS
  CLEAN (5 tests, streak: 2/5)

[Iter 5] Category: error_paths
  CLEAN (4 tests, streak: 3/5)

[Iter 6] Category: overflow_underflow
  CLEAN (3 tests, streak: 4/5)

[Iter 7] Category: concurrent_access
  CLEAN (4 tests, streak: 5/5)

Result: SUCCESS (5/5 clean passes)
Total: 7 iterations, 1 bug fixed, 40 edge cases, 12m 20s
Bug fixed: Missing TargetDataHandle validity check in GA_Attack
Test file: Source/MyGame/Tests/GA_Attack_EdgeCaseTest.cpp
```

---

## Example 4: Expert — Multi-System Integration, 10 Clean Passes

**Scenario**: Full 12-category rotation on `AMyCharacter` with 10 consecutive clean passes.

```bash
/ue-test-iterate AMyCharacter --clean-passes 10 --categories all
```

### Iteration Log

```text
=== UE Test-Iterate Results ===

Target: AMyCharacter (full class + GAS integration)
Type: bdd_spec
Goal: 10 consecutive clean passes
Categories: all 12 (forced full rotation)

--- Initialization ---
Analyzing target via ue_analyze_symbols(get_methods)...
Found 12 methods + 3 GAS abilities + 2 input actions
Test output: Source/MyGame/Tests/AMyCharacter_FullEdgeCase.cpp

[Iter  1] boundary_values       → CLEAN (6 tests, streak: 1/10)
[Iter  2] null_empty            → BUG: nullptr in GetAbilitySystemComponent()
  → Fix: early return guard in BeginPlay()
  → Regression: ALL PASS (streak reset → 0)
[Iter  3] null_empty            → CLEAN (6 tests, streak: 1/10)
[Iter  4] type_boundaries       → CLEAN (4 tests, streak: 2/10)
[Iter  5] unicode_encoding      → CLEAN (3 tests, streak: 3/10)
[Iter  6] large_inputs          → BUG: TArray reallocation crash with 10K buffs
  → Fix: add capacity check in ApplyBuff()
  → Regression: ALL PASS (streak reset → 0)
[Iter  7] large_inputs          → CLEAN (3 tests, streak: 1/10)
[Iter  8] concurrent_access     → CLEAN (4 tests, streak: 2/10)
[Iter  9] error_paths           → CLEAN (5 tests, streak: 3/10)
[Iter 10] overflow_underflow    → CLEAN (3 tests, streak: 4/10)
[Iter 11] state_transitions     → CLEAN (5 tests, streak: 5/10)
[Iter 12] format_variations     → CLEAN (3 tests, streak: 6/10)
[Iter 13] resource_exhaustion   → CLEAN (2 tests, streak: 7/10)
[Iter 14] regression_guards     → CLEAN (4 tests, streak: 8/10)
[Iter 15] boundary_values       → CLEAN (5 tests, streak: 9/10)
[Iter 16] null_empty            → CLEAN (4 tests, streak: 10/10)

Result: SUCCESS (10/10 clean passes)
Total: 16 iterations, 2 bugs fixed, 67 edge cases, 25m 15s
Bugs fixed:
  1. nullptr in GetAbilitySystemComponent() — BeginPlay guard
  2. TArray reallocation crash — capacity check in ApplyBuff()
Test file: Source/MyGame/Tests/AMyCharacter_FullEdgeCase.cpp
```

### Summary Table

| Scenario | Target | Clean Passes | Iterations | Bugs | Edge Cases | Time |
|----------|--------|-------------|-----------|------|------------|------|
| Easy | CalculateDamage | 3/3 | 3 | 0 | 9 | ~2min |
| Normal | AMyCharacter | 10/10 | 12 | 1 | 50 | ~15min |
| Hard | GA_Attack | 5/5 | 7 | 1 | 40 | ~12min |
| Expert | AMyCharacter (all) | 10/10 | 16 | 2 | 67 | ~25min |

---

## Example 5: Headless Mode (No Editor)

```bash
/ue-test-iterate AMyCharacter::GetHealth 3 --headless
```

```text
=== UE Test-Iterate Results ===

Target: AMyCharacter::GetHealth
Type: bdd_spec
Mode: headless (commandlet)
Goal: 3 consecutive clean passes

--- Execution via commandlet ---
Command: ue_build_pipeline(run_commandlet,
    commandlet_name="AutomationTest",
    commandlet_args=["-test=MyGame.Characters.GetHealth"])

[Iter 1] CLEAN  boundary_values    (3 tests, streak: 1/3)
[Iter 2] CLEAN  null_empty         (3 tests, streak: 2/3)
[Iter 3] CLEAN  type_boundaries    (3 tests, streak: 3/3)

Result: SUCCESS (3/3 clean passes)
Total: 3 iterations, 0 bugs, 9 edge cases, 3m 40s
Note: Headless mode ~1.5x slower than Editor mode (no hot reload)
```

---

## Example 6: STUCK Exit (Same Bug 3 Times)

```bash
/ue-test-iterate AMyCharacter::ProcessInput 5
```

```text
[Iter 1] CLEAN  boundary_values    (4 tests, streak: 1/5)
[Iter 2] BUG    null_empty         (streak reset → 0)
  → Fix: add null check for InputComponent
[Iter 3] BUG    null_empty         (streak reset → 0)
  → Same bug pattern: InputComponent still null in different path
  → Fix: move InputComponent init to earlier lifecycle
[Iter 4] BUG    null_empty         (streak reset → 0)
  → Same bug pattern detected 3rd time

Result: STUCK (same bug pattern 3 consecutive times)
Total: 4 iterations, 0 bugs found, 12 edge cases, 4m 50s
Recommendation: Manual review of InputComponent initialization order.
The issue may involve engine lifecycle timing that requires architectural change.
```
