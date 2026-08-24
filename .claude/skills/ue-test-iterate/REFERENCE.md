# UE Test Iterate — Reference

Detailed algorithm, UE-specific edge case values, test type guide, and configuration reference.

---

## Ambiguous Activation Cases

- "TakeDamage 테스트 강화해줘" → ue-test-iterate if "반복"/"엣지" mentioned, else `/ue-test-gen`
- "캐릭터 테스트 해줘" → ue-test-iterate if "클린 패스"/"반복" mentioned, else `/ue-test-gen`

---

## Iteration 4-Phase Detail

Each iteration follows 4 strict phases:

### Phase 1: Analyze

1. Select category: `category = CATEGORIES[iteration % 12]`
2. If first iteration for this target:
   - `ue_analyze_symbols(operation="get_methods", class_name="<target>")` → get method signatures
   - `ue_grep(pattern="<target>")` → find source file location
   - `ue_read(file_path="<source_file>")` → understand parameter types, valid ranges
3. If revisiting a category:
   - Use different edge case values than previous visit
   - Prioritize values adjacent to previously-found bugs

### Phase 2: Generate

1. Determine test type based on `--type` flag (default: `bdd_spec`)
2. For each method in target scope:
   - Generate 2-5 edge case test cases for the selected category
   - Use UE assertion macros: `TestEqual`, `TestTrue`, `TestNearlyEqual`, `TestNull`, `TestNotNull`
   - For float comparisons, always use tolerance: `TestNearlyEqual(TEXT("desc"), Value, Expected, KINDA_SMALL_NUMBER)`
3. Write test to `Source/<Module>/Tests/<Target>_EdgeCaseTest.cpp`
4. Include proper test flags: `EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter`

**Test scaffolding options**:
- `ue_generate_code(operation="generate_test", test_target="<Class>::<Method>")` — template-based
- Direct C++ writing — for custom edge case values not in templates

### Phase 3: Execute

**Editor mode** (default):
```python
# Step 1: Compile (if new test file)
ue_editor_automation(operation="execute_console_command",
    command="NarshaMCP.BenchmarkMode 1")  # Prevent latent test stalls

# Step 2: Run tests
ue_editor_automation(operation="run_automation_tests",
    test_name="MyGame.Characters.TakeDamage.EdgeCase",
    timeout_seconds=120,
    wait_for_completion=true)
```

**Headless mode** (`--headless`):
```python
ue_build_pipeline(operation="run_commandlet",
    commandlet_name="AutomationTest",
    commandlet_args=["-test=MyGame.Characters.TakeDamage.EdgeCase"])
```

### Phase 4: Evaluate

Parse the test execution response:

```json
{
  "tests_run": 5,
  "passed": 4,
  "failed": 1,
  "results": [
    {"name": "...", "status": "passed"},
    {"name": "TakeDamage_NullInstigator", "status": "failed"}
  ]
}
```

**Decision tree**:
- All pass → `streak += 1`; check if `streak >= N` → SUCCESS
- Any fail → analyze failure message, fix source code, `streak = 0`
  - After fix: re-run ALL accumulated tests (regression check)
  - If regression detected: revert fix, try alternative approach
  - If same bug pattern 3 times: STUCK exit

---

## UE Test Type Selection Guide

| Type | Macro | Best For | World Required | Multi-Frame |
|------|-------|----------|----------------|-------------|
| `simple` | `IMPLEMENT_SIMPLE_AUTOMATION_TEST` | Pure functions, math, utilities | No | No |
| `bdd_spec` | `BEGIN_DEFINE_SPEC` / `END_DEFINE_SPEC` | Most cases — readable Given/When/Then | Optional | No |
| `complex_latent` | `IMPLEMENT_COMPLEX_AUTOMATION_TEST` | Animations, physics, async operations | Yes | Yes |
| `functional` | `AFunctionalTest` subclass | Level-based, requires placed actors | Yes (level) | Yes |

**Auto-selection heuristic**:
- Target is a static/utility function → `simple`
- Target is a class method, no async → `bdd_spec` (default)
- Target involves `LatentCommand`, animation, physics → `complex_latent`
- Target is a `UGameplayAbility` subclass (GAS) → `complex_latent` (async Activate/End flow)
- Target requires specific level setup → `functional`

---

## UE Edge Case Values

### 1. Boundary Values
| Value | UE Type | Usage |
|-------|---------|-------|
| `0.0f` | `float` | Zero damage, zero speed |
| `MAX_FLT` | `float` | Maximum float value |
| `-MAX_FLT` | `float` | Minimum float value |
| `FVector::ZeroVector` | `FVector` | Zero position/direction |
| `FVector(MAX_FLT)` | `FVector` | Extreme position |
| `FRotator(0)` | `FRotator` | Identity rotation |
| `FRotator(360, 720, -360)` | `FRotator` | Wrap-around angles |
| `NAME_None` | `FName` | Empty name |
| `FGameplayTag()` | `FGameplayTag` | Invalid/empty tag |

### 2. Null/Empty
| Value | UE Type | Usage |
|-------|---------|-------|
| `nullptr` | `UObject*`, `AActor*` | Null pointer dereference |
| `TArray<>()` | `TArray` | Empty array |
| `FString()` | `FString` | Empty string |
| `FText::GetEmpty()` | `FText` | Empty localized text |
| `FName(NAME_None)` | `FName` | None name |
| `FGameplayTagContainer()` | Tag container | Empty tag set |

### 3. Type Boundaries
| Value | Issue | Check |
|-------|-------|-------|
| `static_cast<uint8>(IntVar)` where `IntVar > 255` | Truncation overflow | Enum values past max |
| `(int32)2147483647 + 1` | int32 overflow | Damage/health calc |
| `(float)16777217` | Float precision loss | Large integer as float |
| `StaticCast<T>` | Incorrect cast | Wrong UClass type |

### 4. Unicode/Encoding
| Value | Issue | Check |
|-------|-------|-------|
| `TEXT("한글 이름")` | Korean characters | Player name, asset path |
| `TEXT("🎮⚔️")` | Emoji | Display name |
| `TEXT("\xEF\xBB\xBF")` | BOM marker | Config file parsing |
| `TEXT("\u200B")` | Zero-width space | String comparison |
| `TEXT("ñ")` / `TEXT("ü")` | Latin extended | Localized strings |

### 5. Large Inputs
| Value | Risk | Check |
|-------|------|-------|
| `TArray` with 10,000 elements | Reallocation | Buff/inventory lists |
| `FString` with 1MB | Memory | Log messages, serialization |
| Nested `UStruct` 20 levels deep | Stack overflow | Recursive processing |
| 1000 `SpawnActor` calls | GC pressure | Spawner/pooling systems |

### 6. Concurrent Access
| Scenario | Risk | Check |
|----------|------|-------|
| GameThread + AsyncThread modify same array | Data race | `FScopeLock` usage |
| Double `Destroy()` on same actor | Double-free | Already pending kill |
| Timer callback after owner destroyed | Dangling pointer | Weak pointer / IsValid |
| Replicated property set during GC | Crash | RPC timing |

### 7-12. (Remaining Categories)

Follow same pattern. Key UE-specific values:

- **Error Paths**: Invalid `FSoftObjectPath`, missing `UClass`, failed `LoadObject`
- **Overflow/Underflow**: `int32` MAX+1, `FTimespan::MaxValue()`, negative array index
- **State Transitions**: `Uninitialized → BeginPlay → EndPlay → Destroyed` lifecycle
- **Format Variations**: Windows `\` vs Unix `/` in paths, `/Game/` vs `/Engine/` prefix
- **Resource Exhaustion**: Texture pool overflow, `FMalloc` failure, GC under pressure
- **Regression Guards**: Variations of previously-discovered bugs with different parameters

---

## UE Assertion Macros

| Macro | Usage | Example |
|-------|-------|---------|
| `TestTrue(Desc, Cond)` | Boolean check | `TestTrue(TEXT("Is alive"), Char->IsAlive())` |
| `TestFalse(Desc, Cond)` | Negative check | `TestFalse(TEXT("Not dead"), Char->IsDead())` |
| `TestEqual(Desc, A, B)` | Exact equality | `TestEqual(TEXT("HP"), HP, 100.0f)` |
| `TestNotEqual(Desc, A, B)` | Inequality | `TestNotEqual(TEXT("Changed"), Before, After)` |
| `TestNearlyEqual(Desc, A, B, Tol)` | Float comparison | `TestNearlyEqual(TEXT("Damage"), Val, 50.0f, 0.01f)` |
| `TestNull(Desc, Ptr)` | Null check | `TestNull(TEXT("Cleared"), Ref)` |
| `TestNotNull(Desc, Ptr)` | Non-null | `TestNotNull(TEXT("Spawned"), Actor)` |
| `AddError(Msg)` | Explicit fail | `AddError(TEXT("Should not reach"))` |
| `AddWarning(Msg)` | Non-fatal | `AddWarning(TEXT("Slow path taken"))` |

**Float comparison rule**: NEVER use `TestEqual` for float values. Always use `TestNearlyEqual` with explicit tolerance to prevent flaky tests.

---

## Configuration Reference

### Arguments

| Argument | Type | Default | Description |
|----------|------|---------|-------------|
| Target | string | (required) | Class name, `Class::Function`, or UE test path |
| Clean passes | int | 5 | Consecutive clean passes required (max: 50) |
| `--type` | enum | `bdd_spec` | `simple`, `bdd_spec`, `complex_latent`, `functional` |
| `--categories` | string | auto-rotate | Comma-separated list or `all` |
| `--headless` | flag | false | Use commandlet instead of Editor |
| `--timeout` | duration | `30m` | Maximum wall clock time |

### Derived Limits

| Limit | Formula | Purpose |
|-------|---------|---------|
| Max iterations | `N * 3` | Prevent infinite loops |
| Tests per iteration | 2-9 | Based on target method count |
| Regression suite | All accumulated tests | Run after every bug fix |

---

## Headless Mode

When `--headless` is specified or Editor is not available:

```bash
# UE command (executed via ue_build_pipeline)
UnrealEditor-Cmd.exe <Project>.uproject \
  -ExecCmds="Automation RunTests MyGame.Characters.TakeDamage" \
  -NullRHI -NoSound -NoSplash -Unattended
```

**Trade-offs**:
- No live Editor required
- ~1.5x slower than Editor mode (no hot reload, full startup)
- Cannot test rendering/visual features (NullRHI)
- Full automation test support including latent commands

---

## Streak Counter Logic

```text
streak = 0
total_iterations = 0
bugs_found = []

for each iteration:
    total_iterations += 1

    if wall_clock() > timeout:
        EXIT: TIMEOUT

    if total_iterations > N * 3:
        EXIT: MAX_REACHED

    result = run_tests(category[iteration % 12])

    if result.all_pass:
        streak += 1
        if streak >= N:
            EXIT: SUCCESS
    else:
        bug = analyze_failure(result)
        if bug in bugs_found[-2:]:  # same bug pattern
            consecutive_same += 1
            if consecutive_same >= 3:
                EXIT: STUCK
        else:
            consecutive_same = 0
        bugs_found.append(bug)
        fix(bug)
        regression = run_all_accumulated_tests()
        if regression:
            revert(fix)
            EXIT: REGRESSION (if revert also fails)
        streak = 0  # reset after any bug
```

---

## Test File Organization

Generated tests are placed in the project's test directory:

```text
Source/<Module>/
├── Private/
│   └── MyCharacter.cpp
├── Public/
│   └── MyCharacter.h
└── Tests/                          ← Test files go here
    ├── MyCharacter_EdgeCaseTest.cpp  ← Generated by ue-test-iterate
    └── MyCharacter_Test.cpp          ← Existing tests (not modified)
```

**Naming convention**: `<Target>_EdgeCaseTest.cpp`
- Appends to existing file across iterations (new `Describe` blocks per category)
- Never modifies existing test files

---

## Output Format

```text
=== UE Test-Iterate Results ===

Target: AMyCharacter::TakeDamage
Type: bdd_spec
Goal: 5 consecutive clean passes

[Iter  1] CLEAN  boundary_values      (4 tests, streak: 1/5)
[Iter  2] BUG    null_empty           (streak reset → 0)
  → Fix: add nullptr guard in TakeDamage() line 142
[Iter  3] CLEAN  null_empty           (4 tests, streak: 1/5)
[Iter  4] CLEAN  type_boundaries      (3 tests, streak: 2/5)
[Iter  5] CLEAN  unicode_encoding     (3 tests, streak: 3/5)
[Iter  6] CLEAN  large_inputs         (2 tests, streak: 4/5)
[Iter  7] CLEAN  concurrent_access    (3 tests, streak: 5/5)

Result: SUCCESS (5/5 clean passes)
Total: 7 iterations, 1 bug fixed, 23 edge cases, 8m 45s
Test files: Source/MyGame/Tests/TakeDamage_EdgeCaseTest.cpp
```

---

## Error Recovery

| Error | Cause | Recovery |
|-------|-------|----------|
| Compilation failure after test generation | Missing include or wrong API usage | Run `ue_fix_errors(smart)` to auto-resolve; if persistent, simplify test |
| `run_automation_tests` timeout | Latent test blocked by low FPS | Enable benchmark mode; increase `timeout_seconds`; use `--headless` |
| Same bug found 3 times (STUCK) | Fix doesn't address root cause | Exit with report; suggest manual review of the fix approach |
| Editor not connected | Editor not running or RC API disabled | Fall back to `--headless` mode via commandlet |
| Test path not found | Wrong test name format | Verify dot-separated path matches `IMPLEMENT_*` macro registration |
| `Build.cs` missing test dependency | Module lacks `AutomationController` | Run `ue_grep(pattern="AutomationController", path="Source/<Module>/<Module>.Build.cs")`; if not found, add `PrivateDependencyModuleNames.Add("AutomationController")` guarded by `Target.bBuildDeveloperTools` |

---

## Changelog

### v1.0.0 (2026-03-15)
- Initial release adapted for UE Automation Test framework
- 12-category edge case rotation
- 4 UE test types: simple, bdd_spec, complex_latent, functional
- Editor + headless execution modes
- Auto-fix with regression guard
- Streak-based completion with STUCK/TIMEOUT/MAX safety exits
