---
name: ue-pre-commit
description: "Purpose: Pre-commit safety check analyzing staged C++ changes to map impact on BPs/assets. 5-phase workflow: VCS scan -> function extract -> impact analysis -> cross-ref -> report. Orchestrates git diff + ue_analyze_symbols + ue_fix_errors. Triggers: 'pre-commit', 'commit check', '커밋 전', '푸시 전 검사', 'safe to commit', 'commit impact', '커밋 영향'."
argument-hint: "[--staged | --branch | --range=main...HEAD]"
---

# UE Pre-Commit Impact Analysis -- Commit Risk Assessment

**Version**: 1.0.0
**Issue**: #5299
**Purpose**: Answer "Is it safe to commit?" by analyzing C++ changes and mapping affected BPs/assets
**Requires**: #5292 (C++<->BP cross-reference) for full BP impact mapping

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the 5-phase workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Detect diff scope** from argument or default to `--staged`
2. **Scan changed files** via git diff, filter to C++ files
3. **Extract changed targets** (classes, functions, properties) from diff hunks
4. **Run impact analysis** via MCP tools for each target
5. **Generate consolidated report** with risk level and recommendations

---

## Auto-Trigger Phrases

### Korean
- "커밋 전 분석해줘", "커밋 전 검사"
- "푸시 전 검사", "커밋해도 돼?"
- "이 변경 커밋해도 안전해?", "커밋 영향 분석"
- "스테이지된 파일 영향 분석"
- "커밋 전에 BP 영향 확인"

### English
- "Pre-commit check", "Pre-commit analysis"
- "Is it safe to commit?", "Check before commit"
- "Commit impact analysis", "What does my commit affect?"
- "Push safety check", "Analyze staged changes"
- "Impact before commit"

---

## 5-Phase Workflow

```text
Phase 1: VCS Changed File Scan
   | git diff (staged/branch) -> filter C++ files (.h/.cpp)
   v
Phase 2: Function/Class Extraction
   | Parse diff hunks -> identify changed UCLASS/UFUNCTION/UPROPERTY
   v
Phase 3: Impact Analysis (MCP)
   | ue_analyze_symbols(impact_analysis) + ue_fix_errors(hotreload_check)
   v
Phase 4: Cross-Reference Extension (MCP) [gated on HIGH risk]
   | ue_analyze_symbols(find_cpp_to_bp) for function-level BP mapping
   v
Phase 5: Consolidated Report
   | Risk assessment, affected BPs, hot-reload safety, recommendations
   v
[Pre-Commit Impact Report]
```

### Phase 1: VCS Changed File Scan

```python
# Determine diff scope from argument
# Default: --staged (true pre-commit: only staged files)
# Alt:     --branch (branch changes vs main)
# Alt:     --range=X...Y (custom range)

if scope == "staged":
    files = Bash("git diff --cached --name-only")
elif scope == "branch":
    files = Bash("git diff main...HEAD --name-only")
elif scope.startswith("range="):
    range_spec = scope.split("=")[1]
    files = Bash(f"git diff {range_spec} --name-only")

# Filter to C++ files only
cpp_files = [f for f in files if f.endswith(('.h', '.cpp'))]
headers = [f for f in cpp_files if f.endswith('.h')]
sources = [f for f in cpp_files if f.endswith('.cpp')]

# SHORT-CIRCUIT: No C++ files -> safe to commit
if not cpp_files:
    print("No C++ files changed. Safe to commit (no BP/asset impact).")
    return
```

### Phase 2: Function/Class Extraction

Read the unified diff and extract changed targets:

```python
# Get unified diff with function context
if scope == "staged":
    diff = Bash("git diff --cached -U0")
else:
    diff = Bash("git diff main...HEAD -U0")
```

**Extraction rules** (Claude reads diff output and identifies):

1. **Hunk headers**: `@@ -L,N +L,N @@ ClassName::FunctionName`
   - These identify which function body was changed
   - Extract both class name and function name

2. **UCLASS changes**: Lines with `UCLASS(...)` followed by `class AMyClass`
   - target_type: "class"

3. **UFUNCTION changes**: Lines with `UFUNCTION(...)` followed by function signature
   - target_type: "function"

4. **UPROPERTY changes**: Lines with `UPROPERTY(...)` followed by member declaration
   - target_type: "property"

5. **Function signature changes**: Return type or parameter list modified
   - HIGH risk for BP callers

**Change classification**:

| Category | Risk | Description |
|----------|------|-------------|
| `signature_change` | HIGH | Function return type or parameters changed |
| `class_hierarchy` | HIGH | Base class changed, UCLASS flags modified |
| `property_change` | HIGH | UPROPERTY added/removed/type-changed |
| `function_body` | MEDIUM | Implementation changed, API preserved |
| `new_function` | LOW | New function added (no existing callers) |
| `comment_only` | NONE | Skip analysis entirely |

**Batching**: If more than 10 targets are extracted, deduplicate to unique classes and run class-level `impact_analysis` only (which covers all methods).

### Phase 3: Impact Analysis (MCP)

```python
ToolSearch("select:mcp__narshamcp__ue_analyze_symbols")
ToolSearch("select:mcp__narshamcp__ue_fix_errors")

# 3a: Impact analysis per target (skip NONE category)
all_impacts = []
for target in targets:
    if target.change_category == "NONE":
        continue

    impact = ue_analyze_symbols(operation="impact_analysis", params={
        "target": target.name,   # class name e.g. "AMyCharacter"
        "depth": 2
    })
    # Returns: risk_level, total_affected, affected_blueprints[],
    #   summary{direct_callers, delegate_binders, interface_implementors},
    #   blueprint_impact{direct_bp_callers[]}
    all_impacts.append((target, impact))

# 3b: Hot-reload safety check
hotreload = ue_fix_errors(operation="hotreload_check", params={
    "auto_detect_git": true,
    "staged": true,
    "unstaged": false
})
# Returns: issues[], checked_files, hot_reload_safe
```

### Phase 4: Cross-Reference Extension (Gated)

**Only execute for targets with**:
- `change_category` == HIGH, OR
- Phase 3 `impact_analysis` found affected BPs (total_affected > 0)

```python
ToolSearch("select:mcp__narshamcp__ue_analyze_symbols")

# 4a: Function-level BP caller mapping
# PRIMARY: Use impact_analysis blueprint_impact (already fetched in Phase 3)
# Phase 3's impact_analysis response includes blueprint_impact.direct_bp_callers[]
# when BP callers exist (Issue #5292). Use this data first.
#
# FALLBACK: If blueprint_impact is missing from Phase 3, try find_cpp_to_bp directly.
# Note: find_cpp_to_bp requires MCP binary built after Issue #5292 (PR #5370).
for target in high_risk_targets:
    if target.type == "function":
        # Check if Phase 3 already has BP caller data
        if phase3_impact.get("blueprint_impact", {}).get("direct_bp_callers"):
            bp_callers = phase3_impact["blueprint_impact"]["direct_bp_callers"]
        else:
            # Fallback: direct find_cpp_to_bp call
            bp_callers = ue_analyze_symbols(operation="find_cpp_to_bp", params={
                "function_name": target.name,  # IMPORTANT: unqualified name
                "limit": 30
            })
            # Returns: blueprint_callers[], total_callers
            # If operation not available (older binary), skip with warning

# 4b: Class-level BP dependency graph (when many BPs affected)
for target in high_risk_targets:
    if target.type == "class" and impact.total_affected > 5:
        bp_deps = ue_analyze_symbols(operation="find_cross_bp_dependencies", params={
            "query": target.name
        })
        # Returns: dependencies[], dependents[], has_circular_dependencies
```

### Phase 5: Consolidated Report

Aggregate all results into a single report (see Output Format below).

**Overall risk calculation**:
- Count all affected BPs across all targets (deduplicated)
- Factor in hot-reload safety status
- Factor in change categories (HIGH changes escalate risk)

```python
RISK_LEVELS = {
    "SAFE":    (0, 3),     # 0-3 affected, all LOW/MEDIUM changes
    "CAUTION": (4, 15),    # 4-15 affected, or any HIGH change
    "BLOCKED": (16, None), # 16+ affected, or hot-reload unsafe + HIGH changes
}
```

---

## Output Format

```text
=== Pre-Commit Impact Report ===

--- Scope ---
Mode: Staged files (git diff --cached)
Changed C++ files: 3 (.h: 2, .cpp: 1)
Targets analyzed: 5 (classes: 2, functions: 3)

--- Risk Summary ---
Overall Risk: HIGH
Hot Reload Safe: NO (2 issues found)

--- Changed Files ---
  [H] Source/MyGame/Characters/MyCharacter.h (signature_change)
  [M] Source/MyGame/Characters/MyCharacter.cpp (function_body)
  [H] Source/MyGame/Weapons/WeaponBase.h (property_change)

--- Impact by Target ---

1. AMyCharacter (class, HIGH)
   Affected BPs: 7
   Risk Level: high
   - BP_PlayerCharacter (hop 1) - direct caller
   - BP_EnemyBase (hop 1) - subclass
   - BP_BossEnemy (hop 2) - via BP_EnemyBase
   - BP_Companion (hop 1) - direct caller
   - [+3 more...]
   Direct BP Callers: 4
   Delegate Bindings: 1

2. AMyCharacter::TakeDamage (function, HIGH - signature changed)
   BP Callers: 3
   - BP_PlayerController -> EventGraph::CallFunction_42
   - BP_DamageSystem -> DamageCalculation::CallFunction_17
   - BP_HUD -> HealthUpdate::CallFunction_8

3. UWeaponBase (class, HIGH - UPROPERTY changed)
   Affected BPs: 4
   - BP_Rifle (hop 1) - subclass
   - BP_Pistol (hop 1) - subclass
   - BP_MeleeWeapon (hop 1) - subclass
   - BP_WeaponPickup (hop 2) - references weapon

--- Hot Reload Safety ---
Status: NOT SAFE (2 issues)
  1. [MyCharacter.h] UCLASS layout changed (struct size affected)
     -> Full rebuild recommended
  2. [WeaponBase.h] UPROPERTY added (may invalidate serialized data)
     -> Resave affected Blueprints after rebuild

--- Recommendations ---
  1. [CRITICAL] Rebuild project before testing (hot reload not safe)
  2. [HIGH] Verify BP_PlayerController cast compatibility (TakeDamage signature changed)
  3. [HIGH] Resave BP_Rifle, BP_Pistol, BP_MeleeWeapon (UPROPERTY layout change)
  4. [MEDIUM] Check 3 BP callers of TakeDamage for parameter mismatch
  5. [LOW] 1 function body change only - API preserved, BPs unaffected

--- Verdict ---
PROCEED WITH CAUTION: 11 BPs affected, 2 hot-reload issues.
Recommend: Full rebuild + resave affected BPs before commit.
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| Not a git repo | Wrong directory | Report: "Not a git repository. Run from project root." |
| No staged files | Nothing staged | Report: "No staged files. Use `git add` first, or try `--branch` mode." |
| No C++ files in diff | Non-C++ changes only | Short-circuit: "No C++ files changed. Safe to commit." |
| impact_analysis fails | PDB not indexed | Report partial: "PDB not available. Run `ue_check_health()`. Showing hotreload_check only." |
| Cross-ref index unavailable | BP metadata not loaded | Skip Phase 4, report Phase 3 results with note |
| find_cpp_to_bp unavailable | MCP binary predates Issue #5292 | Use `impact_analysis` response's `blueprint_impact.direct_bp_callers[]` instead |
| hotreload_check fails | No Source directory found | Skip hot-reload check, warn in report |
| Too many targets (>20) | Large refactor | Batch to class-level only, cap at 20 MCP calls |
| git diff timeout | Very large diff | Limit to first 50 files, warn about truncation |

---

## Evaluation Criteria

### Activation Test Cases

**Positive (5)** - Should activate:
1. "커밋 전에 영향 분석해줘" -> Activate (Korean pre-commit)
2. "Is it safe to commit?" -> Activate (English safety check)
3. "Pre-commit check" -> Activate (direct trigger)
4. "What does my staged code affect?" -> Activate (staged analysis)
5. "/ue-pre-commit --branch" -> Activate (branch mode)

**Negative (4)** - Should NOT activate:
1. "APawn 변경하면 뭐가 깨져?" -> Use `/ue-impact` (specific entity, not VCS-driven)
2. "빌드 에러 해결해줘" -> Use `/ue-debug` (error resolution)
3. "커밋 메시지 작성해줘" -> Not this skill (commit message authoring)
4. "git diff 보여줘" -> Not this skill (raw diff viewing)

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| Changed function extraction accuracy | >85% |
| BP impact detection completeness | >90% (via impact_analysis) |
| Hot-reload safety check accuracy | >95% (via ue_fix_errors) |
| Total response time | <90s (5 targets), <180s (20 targets) |
| False positive rate (unnecessary warnings) | <10% |

---

## Related

- **Issue #5299**: Pre-commit impact analysis linking C++ changes to affected BP/assets
- **Issue #5292**: C++<->BP cross-reference bridge (dependency for full BP mapping)
- **Skills**: `/ue-impact` (entity-specific impact), `/ue-livecoding-fix` (hot-reload diagnostics)
- **MCP Tools**: `ue_analyze_symbols(impact_analysis)`, `ue_analyze_symbols(find_cpp_to_bp)`, `ue_fix_errors(hotreload_check)`

---

**Status**: Phase 1 MVP
**Last Updated**: 2026-02-26
