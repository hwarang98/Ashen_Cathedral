---
name: ue-dev-cycle
description: "Purpose: UE C++ development loop automation (analyze → implement → build → fix → test → summary). Single command for code-build-test cycle with auto error fixing. Triggers: 'dev cycle', 'dev loop', '개발 루프', '개발 사이클', '빌드 루프', 'implement and build'."
argument-hint: ["feature description (e.g., 'AMyCharacter에 Health 추가', 'GAS 공격 어빌리티 시스템', 'create melee combat character')"]
---

# UE Dev Cycle — Development Loop Automation

**Version**: 1.0.0
**Issue**: #6730
**Purpose**: Automate the UE C++ developer loop: analyze → implement → build → fix → test → summary

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the 6-phase workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Initialize** — parse feature description, probe Editor availability
2. **Analyze** — discover project context, existing classes, target paths
3. **Implement** — generate C++ code via `ue_generate_code`
4. **Build + Fix** — build via `ue_build_pipeline`, auto-fix errors (max 3 iterations)
5. **Test** — generate automation test, execute if Editor available
6. **Summarize** — output Dev Cycle Report with generated files and next steps

---

## Auto-Trigger Phrases

### Korean
- "개발 루프 돌려줘", "개발 사이클 실행"
- "코드 빌드 테스트 자동화", "빌드 루프"
- "구현하고 빌드해줘", "코드 짜고 테스트"
- "이 기능 구현하고 빌드까지"

### English
- "Run dev cycle", "Development cycle"
- "Code build test", "Build loop"
- "Implement and build", "Implement and test"
- "Build this feature end to end"

### Should NOT Activate
- "만들어줘" (no build/test intent) -> `/ue-scaffold` (scaffolding only)
- "빌드 에러 해결해줘" -> `/ue-debug` (error diagnosis only)
- "테스트 생성해줘" -> `/ue-test-gen` (test generation only)
- "풀사이클", "full cycle" -> `/full-cycle` (internal issue lifecycle)
- "라이브 코딩 실패" -> `/ue-livecoding-fix` (hot reload fix only)

---

## STEP 0: INITIALIZE

```python
# 1. Parse feature description from $ARGUMENTS[0]
# REQUIRED: If empty, ask user: "What feature should I implement?"
feature_description = $ARGUMENTS[0]

# 2. Probe Editor availability
health = ue_check_health()
editor_available = health.editor.connected  # true/false
project_root = health.project_root

# 3. Parse options (optional)
# --skip-test     : Skip Phase 5 (test generation/execution)
# --build-only    : Skip Phase 2 (code already written, just build+fix)
# --from N        : Start from Phase N (resume interrupted cycle)
```

**Decision Tree**:
- `editor_available == true` -> Full pipeline (Live Coding + test execution)
- `editor_available == false` -> Reduced pipeline (UBT build + test generation only)
- `--build-only` -> Skip Phase 2, start at Phase 3

---

## Phase 1: ANALYZE

**Goal**: Gather project context to inform code generation.

```python
# Step 0 Discovery (lightweight pre-exploration)
# Extract key terms from feature description
ue_grep(query="<base class or keyword from description>", scope="source")
# If matches found:
ue_read(identifier="<discovered class>")

# Verify base class exists in PDB index
ue_analyze_symbols(operation="search_symbols", query="<base class>")

# Discover project structure
ue_grep(query="Build.cs", scope="source")
```

**Outputs**:
- `project_name`: from `ue_check_health`
- `existing_classes`: related classes found via `ue_grep`
- `source_dir`: project Source/ path
- `build_cs_path`: path to relevant Build.cs
- `base_class_verified`: true/false from symbol search

**Feature Complexity Auto-Detection**:

| Complexity | Signal | Phase 2 Behavior |
|------------|--------|-------------------|
| Simple | Single property/function add | Manual edit guidance, skip `derive_class` |
| Normal | Single class derivation | 1x `derive_class` call |
| Complex | GAS/Component system | 1x `derive_class` + features |
| Multi-class | 3+ classes mentioned | Multiple sequential `derive_class` calls |

---

## Phase 2: IMPLEMENT

**Goal**: Generate C++ code from the feature description.

### Step 2a: Architecture Suggestion (Dry-Run)

```python
suggestion = ue_generate_code(operation="suggest_class", params={
    "description": feature_description,
    "project_context": "<from Phase 1 analysis>"
})
# Returns: recommended class name, base class, features, file paths
```

### Step 2b: Pre-Generation Conflict Check

```python
# Check if target files already exist
Glob(pattern="Source/**/<ClassName>.h")
Glob(pattern="Source/**/<ClassName>.cpp")
# If exists: WARN and skip (do not overwrite)
```

### Step 2c: Generate Code

```python
# For each class in the suggestion:
result = ue_generate_code(operation="derive_class", params={
    "base_class": "<from suggestion>",
    "class_name": "<from suggestion>",
    "output_dir": "<from suggestion>",
    "features": ["<auto-detected features>"]
})
# IMPORTANT: Sequential calls only — never parallel ue_generate_code
```

### Step 2d: Dependency Check

```python
ue_fix_errors(operation="dependency_check")
# Catches missing Build.cs modules before build
# If missing modules found: Add to Build.cs automatically
```

**Simple Feature Path** (UPROPERTY add, config change):
- Skip Steps 2a-2c
- Use `Read` + `Edit` to modify existing file directly
- Proceed to Phase 3

---

## Phase 3+4: BUILD-FIX Loop

**Goal**: Build the project and auto-fix any errors. Interleaved loop.

```python
max_iterations = 3
iteration = 0
build_success = false
error_history = []

while iteration < max_iterations and not build_success:
    iteration += 1

    # ── BUILD ──
    if editor_available:
        build_result = ue_build_pipeline(operation="build_editor")
    else:
        # Fallback: UBT via Bash (ue_fix_errors may timeout on large projects)
        # Build.bat LyraEditor Win64 Development <project>.uproject -WaitMutex
        build_result = Bash("Build.bat <Target> Win64 Development <project>.uproject")
        # Parse stdout for "Result: Succeeded" or "error C" lines

    if build_result.success:
        build_success = true
        break

    # ── FIX ──
    if editor_available:
        errors = ue_fix_errors(operation="smart")
    else:
        # Editor offline: parse UBT stdout directly (see Known Limitation #6)
        # ue_fix_errors(scan_build_log) reads Editor log, not UBT output
        # Instead: read Bash build output for "error C", "fatal error" lines
        errors = parse_build_errors_from_bash_output(build_result.stdout)

    # Stagnation check: same errors as previous iteration
    current_errors = extract_error_signatures(errors)
    if current_errors == error_history[-1] if error_history else []:
        # STAGNATION: same errors repeating, manual intervention needed
        break

    error_history.append(current_errors)

    # Apply suggested fixes
    for fix in errors.suggestions:
        if fix.confidence >= 0.7:
            # Apply fix via Edit tool
            Edit(file_path=fix.file, old_string=fix.old, new_string=fix.new)

    # Loop back to BUILD
```

**Exit Conditions**:
1. `SUCCESS` — build passes cleanly
2. `MAX_ITERATIONS` — 3 attempts exhausted
3. `STAGNATION` — same errors repeat (can't auto-fix)

---

## Phase 5: TEST

**Goal**: Generate and optionally execute UE Automation Tests.

```python
# Skip if --skip-test flag set
if skip_test:
    test_status = "SKIPPED"
    goto Phase 6

# Step 5a: Generate test code
test_result = ue_generate_code(operation="generate_test", params={
    "test_target": "<generated class>::<key function>",
    "test_type": "bdd_spec",
    "output_dir": "Source/<Module>/Tests"
})

# Step 5b: Write test file
Write(file_path=test_result.cpp_path, content=test_result.cpp_code)

# Step 5c: Execute test (Editor ON only)
if editor_available:
    # Rebuild to include new test file
    ue_build_pipeline(operation="build_editor")

    # Run the generated test
    ue_editor_automation(operation="run_automation_tests",
        test_name=test_result.pretty_name)
    # Check results
    test_status = "PASS" or "FAIL"
else:
    test_status = "GENERATED (execution requires Editor)"
```

**If test execution fails**:
- Report failing assertions in Phase 6 summary
- Do NOT iterate test fixes (that's `/ue-automation-test-builder`'s job)

---

## Phase 6: SUMMARY

**Goal**: Output comprehensive Dev Cycle Report.

```python
# Verify generated classes exist in PDB index
for class_name in generated_classes:
    ue_analyze_symbols(operation="search_symbols", query=class_name)
```

**Output the report** (see Output Format below).

---

## Output Format

```text
==============================================================
 Dev Cycle Report
==============================================================

Feature: <feature description>
Editor:  <Connected / Offline>

Phase 1 ANALYZE:     OK (project: <name>, <N> related files)
Phase 2 IMPLEMENT:   OK (<N> files generated, <M> deps added)
Phase 3+4 BUILD-FIX: <PASS/FAIL> (iterations: <N>, errors fixed: <M>)
Phase 5 TEST:        <PASS/GENERATED/SKIPPED/FAIL>

--- Generated Files ---
  [x] Source/MyGame/Characters/MyHero.h
  [x] Source/MyGame/Characters/MyHero.cpp
  [x] Source/MyGame/Tests/MyHeroSpec.cpp

--- Build.cs Dependencies Added ---
  - "GameplayAbilities"
  - "GameplayTags"

--- Symbol Verification ---
  AMyHero: FOUND in PDB index
  UMyHeroAttributeSet: FOUND in PDB index

--- Remaining Issues ---
  (none)

--- Next Steps ---
  1. Open Editor and verify in Content Browser
  2. Create Blueprint derived from AMyHero
  3. Set up Input Mapping Context in Project Settings
  4. Test in PIE (Play In Editor)
==============================================================
```

---

## Error Recovery

| Phase | Error | Cause | Fallback |
|-------|-------|-------|----------|
| 0 | `ue_check_health` fails | MCP server not running | Report: "Run `ue_check_health()` to verify MCP connection" |
| 1 | No project context | Project path not configured | Use `ue_check_health()` project_root field |
| 1 | Base class not found in PDB | Invalid class name or PDB not loaded | `ue_analyze_symbols(search_symbols)` with partial name |
| 2 | `suggest_class` returns empty | Ambiguous description | Ask user to clarify feature type |
| 2 | `derive_class` fails | Missing output directory | Create directory, retry |
| 2 | Target file already exists | File conflict | WARN and skip (do not overwrite) |
| 3 | Build timeout | Large project (>5 min) | Report partial result, suggest incremental build |
| 3 | UBT not found | Engine path not configured | Check `UECODEGEN_ENGINE_DIR` in `.env` |
| 4 | `ue_fix_errors` returns no suggestions | Unknown error pattern | Present raw error for manual diagnosis |
| 4 | Auto-fix makes error worse | Wrong fix applied | Revert via `Edit`, stop iteration |
| 4 | 3 iterations exhausted | Complex cascading errors | Report remaining errors, suggest `/ue-debug` |
| 5 | `generate_test` fails | Unsupported class pattern | Skip test, report in summary |
| 5 | Test execution fails | Editor not running | Report: test code generated, run manually |
| 5 | New build fails after test added | Test file has compile errors | Remove test file, report issue |

---

## Anti-Patterns

| DO NOT | DO INSTEAD |
|--------|------------|
| Call `ue_generate_code` in parallel | Always sequential (connection reset risk) |
| Skip Phase 1 analysis | Discovery prevents wrong base class selection |
| Skip `dependency_check` after codegen | Missing modules = guaranteed build failure |
| Iterate build-fix more than 3 times | Report remaining errors for manual fix |
| Run `run_automation_tests` without Editor | Generate test code only, skip execution |
| Overwrite existing files silently | WARN and skip if target files exist |
| Use `gh`, `cargo test`, or CI tools | This skill is for UE developers, not NarshaMCP development |

---

## Evaluation Criteria

### Difficulty Scenarios (from Issue #6730)

**Easy**: UPROPERTY 1개 추가
- Input: "AMyCharacter에 float MaxHealth 프로퍼티 추가해줘"
- Expected: Phase 2 uses `Read` + `Edit` (no `derive_class`), Phase 3+4 builds, Phase 6 verifies via `ue_analyze_symbols`
- Pass: Build success + property exists in PDB

**Normal**: ACharacter 파생 클래스 생성
- Input: "AMyHero 캐릭터 클래스 만들어줘, BeginPlay에서 로그 출력"
- Expected: Phase 2 generates 2 files, Phase 3+4 builds in 1-2 iterations
- Pass: Build success + `ue_analyze_symbols(search_symbols, "AMyHero")` returns result

**Hard**: GAS Ability 시스템
- Input: "GAS 기반 공격 어빌리티 시스템 만들어줘 (ASC + GA_Attack)"
- Expected: Phase 2 generates 4-5 files with `features: ["gas_setup"]`, Phase 3+4 may iterate 2x
- Pass: Build success + test passes + 2+ class symbols found

**Expert**: 멀티클래스 무기 시스템
- Input: "무기 시스템 만들어줘 (AWeaponBase + AMeleeWeapon + ARangedWeapon + 테스트)"
- Expected: Phase 2 calls `derive_class` 3+ times sequentially, Phase 3+4 iterates 2-3x
- Pass: Build success + 4+ class symbols found + test passes

### Activation Test Cases

**Positive (5)** — Should activate:
1. "개발 루프 돌려줘, AMyHero 캐릭터" -> Activate
2. "Dev cycle: create GAS combat character" -> Activate
3. "이 기능 구현하고 빌드까지 해줘" -> Activate
4. "코드 빌드 테스트 자동화" -> Activate
5. "Implement and build a weapon system" -> Activate

**Negative (5)** — Should NOT activate:
1. "캐릭터 만들어줘" -> `/ue-scaffold` (no build/test intent)
2. "빌드 에러 해결해줘" -> `/ue-debug` (error fix only)
3. "풀사이클 6730" -> `/full-cycle` (internal issue lifecycle)
4. "테스트 생성해줘" -> `/ue-test-gen` (test only)
5. "핫 리로드 에러" -> `/ue-livecoding-fix` (hot reload only)

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| Easy scenario pass rate | >95% |
| Normal scenario pass rate | >90% |
| Hard scenario pass rate | >80% |
| Expert scenario pass rate | >70% |
| Build-fix convergence (3 iters) | >85% |
| Time to complete (Normal) | <120s |

---

## Known Limitations

1. **Editor dependency**: Phase 5 test execution requires Editor. Without Editor, tests are generated but not run.
2. **Build time**: Large projects may take 2-5+ minutes per build iteration. Total cycle time scales with project size.
3. **Context window**: Expert scenarios (8-10+ files) may approach context limits. Use `--skip-test` to reduce pressure.
4. **New .cpp files**: Require full UBT build (Live Coding cannot hot-reload new translation units).
5. **GAS complexity**: Hard/Expert scenarios may produce build errors that `ue_fix_errors` cannot auto-resolve. Manual intervention may be needed.
6. **UBT stdout parsing**: `ue_fix_errors(scan_build_log)` reads Editor log only, not UBT stdout. In Editor-offline mode, Claude reads Bash output directly to identify and fix errors.
7. **Test flags**: `ue_generate_code(generate_test)` may generate invalid flag combinations (e.g., SmokeFilter+CriticalPriority). The build-fix loop auto-corrects this on iteration 2.
8. **BDD Spec registration**: BDD Spec tests may not appear in Editor automation test list immediately after rebuild. `run_automation_tests` may timeout; manual refresh or Editor restart needed.

---

**Status**: Phase 1 MVP
**Related**: Issue #6730, `/ue-scaffold` (scaffolding only), `/ue-test-gen` (test only), `/ue-debug` (error diagnosis only)
