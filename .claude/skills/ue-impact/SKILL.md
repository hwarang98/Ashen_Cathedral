---
name: ue-impact
description: "Purpose: Change impact analysis for UE entities. Answers 'What breaks if I change X?' by tracing callers, subclasses, Blueprint references, and config dependencies. Auto-detects target type and runs multi-tool dependency analysis. Triggers: 'impact', 'what breaks', 'change X', 'dependency', '영향 분석', '뭐가 깨져', '변경 영향', '의존성', 'who uses', 'references to', '크로스 레퍼런스', '어떤 BP가 사용', 'cross reference', 'xref', 'references to'."
argument-hint: "TargetName [--xref] (e.g., 'AActor::BeginPlay', 'APawn --xref', 'r.Shadow.MaxResolution', 'BP_Player')"
---

# UE Impact Analysis — Change Impact Assessment

**Version**: 1.1.0
**Issue**: #4800, #5292
**Purpose**: Answer "What breaks if I change X?" with comprehensive dependency analysis

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the 4-phase analysis automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Extract target** from user query or argument
2. **Identify target type** (function, class, config, asset)
3. **Execute all applicable analysis phases**
4. **Generate impact report** with risk level assessment

---

## Auto-Trigger Phrases

### Korean
- "X를 바꾸면 뭐가 깨져?", "X 변경 영향 분석"
- "X 의존성 분석", "X 사용하는 곳 찾아줘"
- "X 리팩토링 전 영향 분석", "X 삭제해도 돼?"
- "X 변경하면 어디 영향?"
- "어떤 BP가 X를 사용해?", "X 크로스 레퍼런스", "X 참조 관계"

### English
- "What breaks if I change X?", "Impact analysis for X"
- "Who uses X?", "Dependencies of X"
- "Is it safe to modify X?", "References to X"
- "What depends on X?", "Refactoring impact of X"
- "Cross reference X", "xref X", "Which BPs call X?"

---

## 4-Phase Analysis Workflow

```text
Phase 1: Identify Target
   │ Detect: function, class, config, or asset
   ▼
Phase 2: Upstream Analysis (Who uses this?)
   │ Callers (functions), Subclasses (classes), Overrides (configs)
   ▼
Phase 3: Cross-Reference (Blueprint/Asset references)
   │ BP references, Asset dependencies
   ▼
Phase 4: Impact Report
   │ Risk level, recommendations
   ▼
[Complete Impact Report]
```

### Phase 1: Identify Target Type

```python
TARGET_TYPES = {
    # Contains :: -> function/method
    "::":           "function",    # AActor::BeginPlay, UObject::PostLoad

    # UE class prefixes
    "A|U|F|E|I":   "class",       # APawn, UGameplayAbility

    # Config prefixes
    "r.|gc.|net.":  "config",      # r.Shadow.MaxResolution

    # Asset prefixes
    "BP_|GA_|GE_|M_|NS_|ST_|BT_": "asset",   # BP_Player, GA_Attack
}
```

### Phase 2: Upstream Analysis

Execute based on target type:

> **Tool Selection Priority**: Use `impact_analysis` first (1 call covers callers + cross-BP + delegates).
> Fall back to `find_callers` / `trace_hierarchy` only when you need finer control (e.g., custom limit, unqualified name matching).

```python
ToolSearch("select:mcp__narshamcp__ue_analyze_symbols")

# RECOMMENDED: One-shot impact analysis (covers callers + cross-BP + delegates)
# Use this for CLASSES and FUNCTIONS — returns comprehensive dependency data in 1 call
impact = ue_analyze_symbols(operation="impact_analysis", params={
    "target": "APawn",  # class or function name
    "depth": 2
})
# Returns: callers, subclasses, blueprint_references, delegate_refs, interface_implementors

# For FUNCTIONS: Find all callers (now includes blueprint_callers automatically)
# IMPORTANT: Use unqualified function name (no Class:: prefix) for best results.
# The tool's substring matching can misfire with qualified names.
callers = ue_analyze_symbols(operation="find_callers", params={
    "function_name": "BeginPlay",  # NOT "AActor::BeginPlay" — use unqualified name
    "limit": 20
})
# NEW: callers["blueprint_callers"] — BP nodes that call this function (auto-included)

# For CLASSES: Find all subclasses + method overrides
hierarchy = ue_analyze_symbols(operation="trace_hierarchy", params={
    "class_name": "APawn"
})
methods = ue_analyze_symbols(operation="get_methods", params={
    "class_name": "APawn"
})

# For CONFIG: Find all overrides across layers
ToolSearch("select:mcp__narshamcp__ue_analyze_config")

overrides = ue_analyze_config(operation="find_overrides", params={
    "option": "r.Shadow.MaxResolution"  # param name is 'option' (CVar name), not 'key'
})
```

### Phase 3: Cross-Reference

```python
ToolSearch("select:mcp__narshamcp__ue_analyze_symbols")

# RECOMMENDED: Use cross-ref operations from ue_analyze_symbols (faster, PDB-indexed)
# These are available if impact_analysis was not used in Phase 2

# Cross-BP dependency analysis (finds BPs that reference this class/function)
cross_bp = ue_analyze_symbols(operation="find_cross_bp_dependencies", params={
    "class_name": "APawn"
})

# Delegate-based dependencies (finds delegate bindings that reference this target)
delegates = ue_analyze_symbols(operation="find_delegate_refs", params={
    "target": "APawn"
})

# Interface implementations (finds classes implementing this interface)
# Use when target is an interface (I-prefix)
implementors = ue_analyze_symbols(operation="find_interface_implementors", params={
    "interface_name": "IAbilitySystemInterface"
})

# FALLBACK: Blueprint graph-level references (requires Editor)
ToolSearch("select:mcp__narshamcp__ue_manage_blueprint")

bp_refs = ue_manage_blueprint(operation="find_bp_references", params={
    "element_name": "APawn"  # param name is 'element_name' (not 'symbol_name')
})

# Find asset references
ToolSearch("select:mcp__narshamcp__ue_search_assets")

asset_refs = ue_search_assets(params={
    "query": "APawn",
    "asset_type": "blueprint"
})
```

### Phase 4: Impact Report

Calculate risk level based on total dependency count (callers + subclasses + BP refs + delegates + interface implementors):

```python
RISK_LEVELS = {
    "LOW":      (0, 5),     # 0-5 dependents
    "MEDIUM":   (6, 15),    # 6-15 dependents
    "HIGH":     (16, 50),   # 16-50 dependents
    "CRITICAL": (51, None), # 51+ dependents
}
# Count ALL dependency types: callers + blueprint_callers + cross_bp + delegates + implementors
```

---

## Output Format

```text
=== Impact Analysis: [Target] ===

--- Risk Level: [HIGH] ---
Total dependents: [N] (callers: X, subclasses: Y, BP refs: Z)

--- Direct Callers ([count]) ---
  1. UGameplayAbility::ActivateAbility (Source/GAS/GameplayAbility.cpp:234)
  2. APlayerController::BeginPlay (Source/Player/PlayerController.cpp:56)
  3. [+N more...]

--- Subclasses ([count]) ---
  APawn
  +-- ACharacter
  |   +-- AMyCharacter
  |   +-- AEnemyCharacter
  +-- AWheeledVehicle
  +-- ADefaultPawn

--- Blueprint References ([count]) ---
  1. BP_PlayerController — Uses in EventGraph (3 nodes)
  2. BP_EnemyBase — Casts to this class
  3. BP_GameMode — Spawns this class

--- Config Dependencies ---
  [Only for config targets]
  - DefaultEngine.ini: r.Shadow.MaxResolution = 2048
  - ProjectSettings.ini: override = 4096

--- Recommendations ---
  1. [Priority 1]: Update 3 subclass overrides of BeginPlay
  2. [Priority 2]: Check BP_PlayerController cast compatibility
  3. [Priority 3]: Consider virtual method contract preservation
  4. Safe to modify: [YES/NO/WITH CAUTION]
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| Symbol not found | PDB not indexed | Report: "PDB index needed. Run ue_check_health()" |
| find_callers wrong matches | Used qualified name "Class::Func" | Use unqualified name "Func" only — tool uses substring matching |
| No callers found | Leaf function or new function | Report: "No callers found - safe to modify" |
| BP references fail | Editor not running | Skip Phase 3, report partial results |
| Config not found | Invalid CVar name | Search with partial key match |

---

## Evaluation Criteria

### Activation Test Cases

**Positive (5)** - Should activate:
1. "AActor::BeginPlay 바꾸면 뭐가 깨져?" -> Activate (function impact)
2. "What depends on APawn?" -> Activate (class impact)
3. "r.Shadow.MaxResolution 변경 영향" -> Activate (config impact)
4. "Is it safe to modify BP_Player?" -> Activate (asset impact)
5. "GA_Attack 리팩토링 전 분석" -> Activate (ability impact)

**Negative (3)** - Should NOT activate:
1. "APawn 어떻게 동작해?" -> Use ue-explain skill
2. "APawn 성능 문제 디버그" -> Use ue-debug skill
3. "APawn 기반 캐릭터 만들어줘" -> Use ue-scaffold skill

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| Caller detection completeness | >90% |
| Subclass chain accuracy | 100% (PDB-based) |
| Risk level accuracy | >85% |
| Response time | <60s for all phases |

---

## v1.1.0: --xref Mode (Issue #5292)

> **Use when**: "어떤 BP가 X를 사용해?", "X xref", "cross reference X", "which BPs call X?"
> **Skips**: Phase 4 risk scoring (returns cross-reference graph only)

### --xref Auto-Detection

Activate `--xref` mode when the query contains ANY of:
- Korean: `크로스 레퍼런스`, `어떤 BP가 사용`, `참조 관계`, `xref`
- English: `cross reference`, `xref`, `who uses`, `which BPs call`, `references to`
- Or the `--xref` flag is explicitly passed

### --xref Workflow (Phase 1-3 only)

```python
ToolSearch("select:mcp__narshamcp__ue_analyze_symbols")

# Phase 1: Extract C++ function or class name from query
# Phase 2: C++→BP reverse lookup
callers = ue_analyze_symbols(operation="find_cpp_to_bp", params={
    "function_name": "ACharacter::Jump",  # C++ function name
    "limit": 50
})
# Returns: blueprint_callers list + total_callers count

# Phase 2b: Full impact with BP section (if class-level query)
impact = ue_analyze_symbols(operation="impact_analysis", params={
    "target": "ACharacter",
    "depth": 2
})
# Returns: affected_blueprints + blueprint_impact.direct_bp_callers (NEW in v1.1.0)

# Phase 3: Generate Mermaid cross-reference graph
```

### --xref Output Format

```mermaid
=== C++↔BP Cross-Reference: [Target] ===

--- Blueprint Callers ([count]) ---
  1. BP_Hero → EventGraph::K2Node_CallFunction_42
  2. BP_Enemy → BeginPlay::K2Node_CallFunction_17
  3. [+N more...]

--- Cross-Reference Graph (Mermaid) ---
graph LR
    Jump["ACharacter::Jump"] --> BP_Hero
    Jump --> BP_Enemy
    BP_Hero --> EventGraph
    BP_Enemy --> BeginPlay

Note: Risk scoring skipped (--xref mode). Use full /ue-impact for risk assessment.
```

---

**Status**: v1.1.0 (Phase 1 MVP + --xref mode)
**Related**: Issue #4800 (MCP-First Tool Selection), Issue #5292 (C++↔BP Cross-Reference Bridge)
