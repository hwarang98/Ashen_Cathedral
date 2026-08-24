---
name: ue-perf-lint
model: opus
description: "Static performance anti-pattern scanner for UE C++ code. Scans Tick/BeginPlay/Constructor for PERF-001~023 rules. Triggers: '성능 린트', 'perf lint', 'anti-pattern check', 'Tick 성능 검사', '코드 성능 검사'."
compatibility: "Requires NarshaMCP MCP server (v0.9.7+)"
argument-hint: "(no args = full scan), --class <name>, --rules PERF-016,PERF-020, --strict"
metadata:
  version: "1.0.0"
  author: "Next-Stage-Inc"
  license: "MIT"
  issue: "#7214"
---

# UE Perf Lint — Static Performance Anti-Pattern Scanner

**Version**: 1.0.0
**Issue**: #7214 (Epic #7207)
**Purpose**: Scan UE C++ source code for 23 performance anti-patterns (PERF-001~023) without requiring runtime data

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the scan workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Scope**: `$ARGUMENTS`
   > If `$ARGUMENTS` is empty, scan all project source files.
   > `--class AMyCharacter` scans a specific class.
   > `--rules PERF-016,PERF-020` scans only specific rules.
   > `--strict` treats any warning as a blocking issue.
2. **Execute the 3-phase workflow** below
3. **Generate severity-ranked report** with file:line, rule ID, message, and suggestion

---

## Auto-Trigger Phrases

### Korean
- "성능 린트", "퍼포먼스 린트", "안티패턴 검사"
- "Tick 성능 검사", "코드 성능 검사"
- "PERF 규칙 검사", "성능 규칙 스캔"

### English
- "perf lint", "performance lint", "anti-pattern check"
- "tick performance scan", "static perf analysis"
- "check PERF rules", "scan performance rules"

### Disambiguation
- `/ue-perf-lint` = **Static code scanning** (no runtime data, no Editor needed)
- `/ue-insights-profiler` = **Runtime profiling** (requires .utrace files or Unreal Insights)
- `/ue-audit` = **Project-wide health check** (broader scope, includes config/assets/build)

---

## Rule Reference (23 rules)

### Phase 1 Rules (PERF-001~015) — Tick/Constructor/BeginPlay patterns

| Rule | Pattern | Severity | Context |
|------|---------|----------|---------|
| PERF-001 | `NewObject<>` in Tick | Warning | Allocates every frame |
| PERF-002 | `SpawnActor<>` in Tick | Warning | Spawns every frame |
| PERF-003 | `LoadObject<>` in Tick | Warning | Sync load every frame |
| PERF-004 | `FindObject<>` in Tick | Warning | String lookup every frame |
| PERF-005 | `GetAllActorsOfClass` in Tick | Warning | Full scene scan every frame |
| PERF-006 | `AddOnScreenDebugMessage` in Tick | Info | Debug-only, remove before ship |
| PERF-007 | `GetAllActorsWithTag` in Tick | Warning | Full scene tag scan every frame |
| PERF-008 | `LoadObject<>` in Constructor | Warning | Blocks game thread |
| PERF-009 | `FObjectFinder` in Constructor | Info | Blocks loading (note cost) |
| PERF-010 | `SpawnActor<>` in BeginPlay | Warning | Adds to level load time |
| PERF-011 | `Cast<>` in Tick | Warning | Dynamic type check every frame |
| PERF-012 | `GetTimerManager` in Tick | Warning | Prefer delegate-based timers |
| PERF-013 | `FlushPersistentDebugLines` in Tick | Error | Devastates frame rate |
| PERF-014 | `bReplicates = true` (any) | Warning | Ensure GetLifetimeReplicatedProps defined |
| PERF-015 | `bCanEverTick = true` (any) | Info | Disable if not needed |

### Phase 2 Rules (PERF-016~023) — Threshold & Replication patterns

| Rule | Pattern | Severity | Threshold |
|------|---------|----------|-----------|
| PERF-016 | Excessive `SpawnActor` in BeginPlay | Warning | >5 calls |
| PERF-017 | Excessive `FObjectFinder` in Constructor | Warning | >3 calls |
| PERF-018 | `LoadObject<>` in BeginPlay | Warning | Sync asset load |
| PERF-019 | Excessive `CreateDefaultSubobject` | Warning | >10 calls |
| PERF-020 | Large replicated TArray in header | Critical | >1024 bytes |
| PERF-021 | Replicated without `ReplicatedUsing` | Warning | Missing OnRep |
| PERF-022 | `bReplicates` without `NetUpdateFrequency` | Warning | Missing rate limit |
| PERF-023 | Excessive Tick property changes with replication | Critical | >5 assignments |

### Suppression
- `// NOLINT(PERF-XXX)` on the same line suppresses a specific rule
- `// NOLINT` suppresses all rules for that line

---

## 3-Phase Workflow

### Phase 0: Parse Arguments

```text
Parse $ARGUMENTS:
- (empty) → full project scan
- --class AMyCharacter → scan only that class
- --rules PERF-016,PERF-020 → filter to specific rules
- --strict → any warning blocks (perf_strict=true)
```

### Phase 1: Discovery — Find Candidate Files

Use `ue_grep` to find files containing performance-relevant patterns:

```python
# Find files with Tick/BeginPlay/Constructor implementations
ue_grep(params={"query": "::Tick(|::BeginPlay(|bReplicates|bCanEverTick", "domain": "source"})

# If --class specified, narrow to that class
ue_grep(params={"query": "class AMyCharacter", "domain": "source"})
```

Collect unique `.h` and `.cpp` file pairs from results.

### Phase 2: Analysis — Run Performance Rules

#### Phase 2A: Replication Rules (PERF-020/021/022) — `ue_analyze_source` (PRIMARY)

For replication-specific rules, **always use `ue_analyze_source` first** (structured macro parsing replaces manual grep):

```python
# PERF-020 (Large replicated TArray) + PERF-021 (Replicated without ReplicatedUsing):
ue_analyze_source(operation="cross_check", class_name="<target_class>")
# → missing_registration[] → PERF-020: UPROPERTY(Replicated) without DOREPLIFETIME
# → matched[].on_rep_function == null → PERF-021: Replicated without ReplicatedUsing
# → matched[].specifiers containing "TArray" + Replicated → PERF-020: large array check

# PERF-022 (bReplicates without NetUpdateFrequency):
ue_analyze_source(operation="extract_pattern", specifier_type="UPROPERTY", class_filter="<target_class>")
# → Any Replicated property found → check class .cpp for NetUpdateFrequency setting
# → Missing NetUpdateFrequency → PERF-022 warning
```

**Fallback** (if `ue_analyze_source` returns error or tool unavailable):
Use `ue_read` + manual pattern matching for PERF-020/021/022.

#### Phase 2B: Non-replication Rules (PERF-001~019, PERF-023) — `ue_read` pattern matching

For each candidate file pair:
1. `ue_read` the header (`.h`) — scan for PERF-014/015/019/023
2. `ue_read` the source (`.cpp`) — scan for function scope patterns
3. Identify function scopes (Tick, BeginPlay, Constructor)
4. Match patterns from rule table (PERF-001~013, PERF-016~018)
5. Check NOLINT suppression
6. Collect warnings with file:line

**Optional**: Use `ue_generate_code(operation="derive_class", run_preflight=true)` to check generated code.

### Phase 3: Report — Severity-Ranked Output

Generate a markdown report sorted by severity (Critical > Warning > Info):

```markdown
## Performance Lint Report

**Scanned**: N files | **Warnings**: M total | **Critical**: C | **Strict mode**: on/off

### Critical (C items)

| # | File:Line | Rule | Pattern | Suggestion |
|---|-----------|------|---------|------------|
| 1 | AMyChar.cpp:45 | PERF-001 | NewObject<> in Tick | Cache in BeginPlay or member variable |
| 2 | AMyChar.h:12 | PERF-020 | TArray<FTransform> Replicated | Use delta compression or reduce frequency |

### Warning (W items)

| # | File:Line | Rule | Pattern | Suggestion |
|---|-----------|------|---------|------------|
| 1 | AMyChar.cpp:67 | PERF-004 | FindComponentByClass in Tick | Cache in BeginPlay |

### Info (I items)

| # | File:Line | Rule | Pattern | Suggestion |
|---|-----------|------|---------|------------|
| 1 | AMyChar.cpp:23 | PERF-015 | bCanEverTick without Tick | Remove bCanEverTick or implement Tick |

### Suppressed (S items)
- AMyChar.cpp:89: PERF-007 DrawDebugLine (NOLINT)

### Summary
- True Positive rate: ≥95% (verified against 43 TP + 20 TN test suite)
- False Positive reduction: NOLINT suppression available per-line
```

---

## MCP Tools Used

| Tool | Phase | Purpose |
|------|-------|---------|
| `ue_grep` | 1 | Find candidate files with perf-relevant patterns |
| `ue_analyze_source` | 2A (primary) | Replication analysis for PERF-020/021/022 (`cross_check`, `extract_pattern`) |
| `ue_read` | 2B | Read source files for non-replication rule pattern analysis |
| `ue_generate_code` | 2B (optional) | Run preflight on generated code |
| `ue_analyze_symbols` | 2B (optional) | Verify class hierarchy for base class detection |

---

## Limitations

1. **Static analysis only** — cannot detect runtime-only patterns (e.g., branch-dependent allocations)
2. **Pattern matching** — may miss patterns inside macros or heavily templated code
3. **Function scope detection** — relies on `void ClassName::FuncName(` patterns; nested lambdas may be misattributed
4. **Header scanning** — PERF-020/021 require header files; if only `.cpp` is available, these rules are skipped

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| `ue_grep` returns empty | No UE source files in project | Display "No C++ source files found" |
| `ue_read` file not found | File path changed since grep | Skip file, continue scanning |
| Class hierarchy unknown | PDB not loaded | Skip base-class-dependent rules |
| Too many files (>100) | Large project | Scan only files with Tick/BeginPlay patterns |

---

## Output Format

```text
═══════════════════════════════════════════
 Performance Lint Report
═══════════════════════════════════════════
 Scanned:  N files
 Warnings: M total (C critical, W warning, I info)
 Strict:   on/off
 Rules:    PERF-001~023 (or filtered subset)

 [Critical] AMyChar.cpp:45  PERF-001  NewObject in Tick
 [Warning]  AMyChar.cpp:67  PERF-004  FindComponentByClass in Tick
 [Info]     AMyChar.cpp:23  PERF-015  bCanEverTick without Tick

 Suppressed: S items (NOLINT)
═══════════════════════════════════════════
```

---

## Related Skills

- [`/ue-insights-profiler`](../ue-insights-profiler/SKILL.md) — Runtime .utrace profiling (complementary)
- [`/ue-audit`](../ue-audit/SKILL.md) — Project-wide health audit (broader scope)
- [`/ue-pre-commit`](../ue-pre-commit/SKILL.md) — Pre-commit safety analysis (includes perf)
- [`/ue-test-gen`](../ue-test-gen/SKILL.md) — Test generation (with `with_perf_tests` option)
## MCP Tool Examples

```python
# Read source file for performance lint
ue_read(identifier="AMyCharacter")
# Generate optimized code suggestion
ue_generate_code(operation="smart", params={"base_class": "ACharacter", "class_name": "AOptimizedChar"})
```
