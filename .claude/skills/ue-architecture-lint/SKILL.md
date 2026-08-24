---
name: ue-architecture-lint
description: "Static architecture violation scanner for AI-generated UE C++ code. 10 rules (ARCH-001~010): inheritance depth, God Class, missing components, circular deps, Runtime/Editor xref, excessive deps, replication mismatch, GAS bypass, Tick abuse, missing UPROPERTY. Triggers: '아키텍처 린트', 'architecture lint', '구조 검사', 'design check', '코드 구조 분석'."
compatibility: "Python 3.10+. MCP server NOT required (standalone script). MCP optional for --fix mode."
argument-hint: "(no args = full scan), --scope staged|all|module:<Name>, --fix, --threshold ARCH-XXX:param=val, --disable ARCH-XXX"
metadata:
  version: "1.0.0"
  author: "Next-Stage-Inc"
  license: "MIT"
  issue: "#7590"
---

# UE Architecture Lint — AI-Generated Code Architecture Validator

**Version**: 1.0.0
**Issue**: #7590 (based on #7580 community data — 14 critical/high "AI code hides architectural violations")
**Purpose**: Detect 10 architecture anti-patterns (ARCH-001~010) in UE C++ code using 6 existing MCP tools

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the arch_lint operation automatically**.
> Do NOT just display documentation — call the MCP tool!

### Execution Protocol

1. **Call MCP tool**:
   ```python
   ue_fix_errors(operation="arch_lint", scope="all")
   ```
   > Scope options: `"all"` (default), `"staged"`, `"module:Combat"`
   > Disable rules: `disabled_rules=["ARCH-003", "ARCH-010"]`
2. **Read the JSON response**: violations array with rule, severity, file, line, description
3. **Explain findings**: Group by severity (ERROR > WARNING > INFO), suggest fixes
4. **If `--fix` requested**: For ARCH-007, use built-in `Read` + `Edit` to add missing DOREPLIFETIME

### Fallback: Python Script

If MCP server is not available, fall back to the standalone Python script:
```bash
python scripts/quality/ue_arch_lint.py --project "$PROJECT_PATH" $ARGUMENTS
```

### MCP is Single Source of Truth

The Rust implementation in `ue_fix_errors(operation="arch_lint")` is the primary path.
Python script (`scripts/quality/ue_arch_lint.py`) is the standalone fallback.

---

## Auto-Trigger Phrases

### Korean
- "아키텍처 린트", "구조 검사", "아키텍처 검증"
- "코드 구조 분석", "설계 검사", "아키텍처 위반"
- "ARCH 규칙 검사", "구조 린트"

### English
- "architecture lint", "arch lint", "architecture check"
- "structure check", "design check", "architecture violations"
- "check ARCH rules", "structural analysis"

### Disambiguation
- `/ue-architecture-lint` = **Structural design rules** (ARCH-001~010: inheritance, components, module deps, replication)
- `/ue-perf-lint` = **Performance anti-patterns** (PERF-001~023: Tick allocations, replication perf)
- `/ue-pre-commit` = **Pre-commit impact analysis** ("what breaks if I commit this?")
- `/ue-audit` = **Project-wide health check** (server health, config, assets, code quality, message log)

---

## Threshold Configuration

### Default Thresholds

모든 규칙에는 기본 임계값이 있으며, 프로젝트별로 오버라이드할 수 있습니다.

| Rule | Parameter | Default | Description |
|------|-----------|---------|-------------|
| ARCH-001 | `max_inheritance_depth` | 3 | 엔진 베이스 클래스 제외, 프로젝트 레벨 상속 깊이 |
| ARCH-002 | `max_methods` | 30 | 클래스당 최대 메서드 수 |
| ARCH-002 | `max_lines` | 1500 | 헤더 파일 최대 줄 수 |
| ARCH-003 | `min_components` | 1 | Actor 서브클래스 최소 컴포넌트 수 |
| ARCH-006 | `max_public_deps` | 10 | Build.cs PublicDependencyModuleNames 최대 개수 |
| ARCH-008 | `health_patterns` | `["Health\\s*[-+]=", "SetHealth", "->Health\\s*="]` | GAS 바이패스 감지 패턴 |
| ARCH-009 | `tick_violation_threshold` | 1 | Tick 안티패턴 최소 감지 수 |

### Project Config File: `.ue-arch-lint.json`

프로젝트 루트에 `.ue-arch-lint.json` 파일을 생성하면 기본값을 오버라이드합니다.

```json
{
  "version": "1.0",
  "thresholds": {
    "ARCH-001": { "max_inheritance_depth": 4 },
    "ARCH-002": { "max_methods": 50, "max_lines": 2000 },
    "ARCH-006": { "max_public_deps": 24 }
  },
  "severity_overrides": {
    "ARCH-003": "info"
  },
  "disabled_rules": [],
  "exclude_paths": [
    "Source/ThirdParty/**"
  ],
  "exclude_classes": [
    "ALyraTaggedActor",
    "AModularCharacter",
    "AModularPawn"
  ]
}
```

**필드 설명:**

| Field | Type | Description |
|-------|------|-------------|
| `thresholds` | object | 규칙별 임계값 오버라이드 (미지정 시 기본값 사용) |
| `severity_overrides` | object | 규칙별 심각도 변경 (`error` / `warning` / `info`) |
| `disabled_rules` | array | 비활성화할 규칙 ID 목록 |
| `exclude_paths` | array | 스캔에서 제외할 경로 패턴 (glob) |
| `exclude_classes` | array | 스캔에서 제외할 클래스명 목록 |

**Config 미존재 시**: 모든 기본값이 적용됩니다. 파일이 없어도 정상 동작합니다.

### CLI Override

`--threshold` 인자로 일회성 오버라이드도 가능합니다:

```bash
/ue-architecture-lint --scope all --threshold ARCH-006:max_public_deps=20
/ue-architecture-lint --scope staged --disable ARCH-003,ARCH-010
```

---

## Rule Reference (10 rules)

### Inheritance & Structure (3 rules)

| Rule | Pattern | Severity | Detection |
|------|---------|----------|-----------|
| ARCH-001 | Inheritance depth > `max_inheritance_depth` (default: 3) | Warning | `ue_analyze_symbols(trace_hierarchy)` — count depth |
| ARCH-002 | God Class: `max_methods`+ methods (default: 30) OR `max_lines`+ lines (default: 1500) | Warning | `ue_analyze_symbols(get_methods)` + file line count |
| ARCH-003 | Actor subclass with < `min_components` components (default: 1) | Warning | `ue_grep("CreateDefaultSubobject")` — 0 hits in Actor .cpp |

### Module & Dependencies (3 rules)

| Rule | Pattern | Severity | Detection |
|------|---------|----------|-----------|
| ARCH-004 | Circular module dependency | Error | Build.cs `PublicDependencyModuleNames` 파싱 → A↔B 양방향 참조 감지 |
| ARCH-005 | Runtime module includes Editor-only headers | Error | `ue_grep` Build.cs type + include pattern analysis |
| ARCH-006 | > `max_public_deps` PublicDependencyModuleNames (default: 10) | Warning | `ue_read` Build.cs — count dependencies |

### UE Patterns (4 rules)

| Rule | Pattern | Severity | Detection |
|------|---------|----------|-----------|
| ARCH-007 | UPROPERTY(Replicated) without DOREPLIFETIME registration | Error | `ue_diff(replication_audit)` + `ue_analyze_source(cross_check)` |
| ARCH-008 | Direct Health/Damage manipulation in GAS project (bypassing GameplayEffects) | Warning | `ue_grep` with configurable `health_patterns` + GAS module presence check |
| ARCH-009 | Tick abuse (heavy operations in Tick) | Warning | `ue_grep("::Tick(")` + Tick 본문 내 NewObject/SpawnActor/LoadObject/GetAllActorsOfClass 패턴 매칭 |
| ARCH-010 | Member variables/functions without UPROPERTY/UFUNCTION that should be BP-exposed | Info | `ue_analyze_source(extract_pattern)` + `ue_grep` for raw declarations |

### Suppression

- `// NOLINT(ARCH-XXX)` on the same line suppresses a specific rule
- `// NOLINT` suppresses all ARCH rules for that line

---

## 5-Phase Workflow

### Phase 0: Parse Arguments & Load Config

```text
1. Parse $ARGUMENTS:
   - (empty) → --scope all (full project scan)
   - --scope staged → git diff --cached --name-only | filter *.h *.cpp
   - --scope all → scan all Source/**/*.h and Source/**/*.cpp
   - --scope module:Combat → scan Source/**/Combat/**
   - --fix → enable auto-fix for ARCH-007, ARCH-009
   - --threshold ARCH-006:max_public_deps=20 → one-off override
   - --disable ARCH-003,ARCH-010 → skip specific rules

2. Load config (priority: CLI > project file > defaults):
   a. Read defaults from "Threshold Configuration" table above
   b. If .ue-arch-lint.json exists in project root → merge overrides
   c. If --threshold or --disable CLI args → apply on top
   d. Log effective thresholds in report header
```

### Phase 1: Scope Discovery (Step 0 — lightweight pre-assessment)

Determine which files and classes to scan:

```python
# For --scope staged:
# Use git diff to find changed files
git diff --cached --name-only -- '*.h' '*.cpp'
# OR if no staged changes, use recent commits:
ue_analyze_symbols(operation="detect_changes", scope="staged")

# For --scope all:
ue_grep(params={"query": "UCLASS(", "domain": "source", "limit": 200})

# For --scope module:Name:
ue_grep(params={"query": "UCLASS(", "domain": "source", "path_filter": "Name"})
```

Collect candidate classes and their file paths. If 0 classes found, report "No UE classes in scope" and exit.

### Phase 2: Data Collection (6 MCP tools, parallelize where possible)

Run these data collection calls. Group independent calls together:

**Group A** (class-level analysis):
```python
# ARCH-001: Inheritance hierarchy for each candidate class
ue_analyze_symbols(operation="trace_hierarchy", class_name="<ClassName>", direction="up")

# ARCH-002: Method count
ue_analyze_symbols(operation="smart", class_name="<ClassName>")
# → get_methods result includes method list; count them

# ARCH-003: Component usage check
ue_grep(params={"query": "CreateDefaultSubobject", "path_filter": "<ClassName>.cpp"})
```

**Group B** (module-level analysis):
```python
# ARCH-004: Module dependency graph
ue_analyze_source(operation="map_dependencies")

# ARCH-005/006: Build.cs analysis
ue_grep(params={"query": "PublicDependencyModuleNames", "domain": "source", "file_filter": "*.Build.cs"})
# Then ue_read each Build.cs to check module type and count deps
```

**Group C** (pattern-level analysis):
```python
# ARCH-007: Replication mismatch
ue_diff(operation="replication_audit", class_name="<ClassName>")
# AND/OR:
ue_analyze_source(operation="cross_check", class_name="<ClassName>")

# ARCH-008: GAS bypass detection
# First check if GAS exists in project:
ue_grep(params={"query": "GameplayAbilitiesModule\\|GameplayAbilities", "domain": "source", "file_filter": "*.Build.cs"})
# If GAS found, check for direct Health manipulation:
ue_grep(params={"query": "Health\\s*[-+]=\\|SetHealth\\|->Health\\s*=", "domain": "source"})

# ARCH-009: Tick abuse - direct pattern matching in Tick body
ue_grep(params={"query": "::Tick(", "domain": "source"})
# Then for each Tick implementation, check for: NewObject, SpawnActor, LoadObject,
# FindObject, GetAllActorsOfClass, GetAllActorsWithTag inside function body

# ARCH-010: Missing UPROPERTY/UFUNCTION
ue_analyze_source(operation="extract_pattern", specifier_type="UPROPERTY", class_filter="<ClassName>")
# Compare against raw member declarations in header
```

### Phase 3: Rule Evaluation

For each ARCH rule, evaluate collected data:

```text
For each class in scope:
  ARCH-001: hierarchy_depth > 3 → WARNING
  ARCH-002: method_count > 30 OR file_lines > 1500 → WARNING
  ARCH-003: is_actor_subclass AND component_count == 0 → WARNING
  ARCH-004: dependency_cycles.length > 0 → ERROR
  ARCH-005: runtime_module includes editor_header → ERROR
  ARCH-006: public_deps_count > 10 → WARNING
  ARCH-007: replicated_without_registration.length > 0 → ERROR
  ARCH-008: gas_project AND direct_health_manipulation → WARNING
  ARCH-009: perf_lint_tick_violations.length > 0 → WARNING
  ARCH-010: undeclared_bp_members found → INFO

  Check NOLINT suppression before adding to report
```

### Phase 4: Report Generation

Generate severity-ranked markdown report:

```markdown
## Architecture Lint Report

**Scanned**: N classes (M files) | **Scope**: staged/all/module:X
**Violations**: V total | **Errors**: E | **Warnings**: W | **Info**: I

### Errors (E items)

| # | File:Line | Rule | Description | Suggestion |
|---|-----------|------|-------------|------------|
| 1 | AMyChar.h:15 | ARCH-007 | UPROPERTY(Replicated) Health without DOREPLIFETIME | Add DOREPLIFETIME(AMyChar, Health) to GetLifetimeReplicatedProps |

### Warnings (W items)

| # | File:Line | Rule | Description | Suggestion |
|---|-----------|------|-------------|------------|
| 1 | AMyChar.h:1 | ARCH-002 | God Class: 45 methods | Extract to components (Movement, Combat, Inventory) |

### Info (I items)

| # | File:Line | Rule | Description | Suggestion |
|---|-----------|------|-------------|------------|
| 1 | AMyChar.h:30 | ARCH-010 | float Speed not UPROPERTY | Add UPROPERTY(EditAnywhere) if BP access needed |

### Suppressed (S items)
- AMyChar.cpp:89: ARCH-008 direct Health (NOLINT)

### Summary
- Architecture Score: X/10 (10 = no violations)
- Top recommendation: [most impactful fix]
```

### Phase 5: Auto-Fix (--fix only)

If `--fix` flag is set, attempt automatic fixes for supported rules:

| Rule | Auto-Fix | Method |
|------|----------|--------|
| ARCH-007 | Add missing DOREPLIFETIME registration | Edit .cpp GetLifetimeReplicatedProps function |
| ARCH-009 | Suggest Timer conversion | Generate timer-based replacement code snippet |
| Others | Manual fix guide only | Display suggestion in report |

For ARCH-007 auto-fix:
```python
# Read the .cpp file, find GetLifetimeReplicatedProps, add missing DOREPLIFETIME lines
ue_read(identifier="<ClassName>.cpp")
# Then use built-in Edit tool to insert the missing registration
```

---

## MCP Tools Used

| Tool | Phase | Purpose |
|------|-------|---------|
| `ue_analyze_symbols` | 2A | Hierarchy depth (ARCH-001), method count (ARCH-002), change detection |
| `ue_analyze_source` | 2B, 2C | Module dependencies (ARCH-004), replication cross-check (ARCH-007), UPROPERTY extraction (ARCH-010) |
| `ue_diff` | 2C | Replication audit (ARCH-007) |
| `ue_grep` | 1, 2A-C | File discovery, component check (ARCH-003), Build.cs analysis (ARCH-005/006), GAS/Health patterns (ARCH-008) |
| `ue_read` | 2B | Build.cs detailed reading (ARCH-005/006), file line counting (ARCH-002) |
| `/ue-perf-lint` skill | 2C | Tick abuse detection reuse (ARCH-009) |

---

## Limitations

1. **Static analysis only** — cannot detect runtime-only architecture violations
2. **ARCH-003 heuristic** — checks CreateDefaultSubobject calls; manual component creation patterns may be missed
3. **ARCH-008 GAS bypass** — pattern-based; custom health wrapper functions may not be detected
4. **ARCH-010 BP exposure** — cannot determine intent; flags as Info severity only
5. **Module-level rules** (ARCH-004/005/006) require Build.cs files to exist in project

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| `ue_analyze_symbols` returns empty | PDB not indexed | Skip ARCH-001/002, use `ue_grep` fallback |
| `ue_analyze_source(map_dependencies)` fails | No Build.cs in scope | Skip ARCH-004, report "module analysis unavailable" |
| `ue_diff(replication_audit)` returns empty | No replicated properties | Skip ARCH-007, report "no replication found" |
| GAS module not found | Non-GAS project | Skip ARCH-008 entirely |
| `/ue-perf-lint` unavailable | Skill not loaded | Fallback: `ue_grep("::Tick(")` + manual pattern check for ARCH-009 |
| Too many classes (>50) | Large project | Limit to --scope staged or top 50 by file size |

---

## Output Format

```text
================================================================
 Architecture Lint Report
================================================================
 Scanned:    N classes (M files)
 Scope:      staged / all / module:Combat
 Violations: V total (E error, W warning, I info)

 [ERROR]   AMyChar.h:15      ARCH-007  Replicated without DOREPLIFETIME
 [ERROR]   Combat.Build.cs:1  ARCH-004  Circular dep: Combat ↔ AI
 [WARNING] AMyChar.h:1        ARCH-002  God Class: 45 methods
 [WARNING] AEnemy.h:1         ARCH-001  Inheritance depth: 4
 [INFO]    AMyChar.h:30       ARCH-010  float Speed not UPROPERTY

 Suppressed: S items (NOLINT)
 Score: 6/10
 Top fix: Split AMyChar into components (ARCH-002)
================================================================
```

---

## Standalone Script (Claude 없이 실행)

MCP 서버나 Claude 없이 Python 스크립트로 직접 실행할 수 있습니다:

```bash
# 1. 초기 설정 — 프로젝트 분석 후 .ue-arch-lint.json 자동 생성
python scripts/quality/ue_arch_lint.py --init --project "C:/MyProject"

# 2. 전체 스캔
python scripts/quality/ue_arch_lint.py --project "C:/MyProject"

# 3. 커밋 전 변경분만
python scripts/quality/ue_arch_lint.py --scope staged --project "C:/MyProject"

# 4. 특정 모듈만
python scripts/quality/ue_arch_lint.py --scope module:Combat --project "C:/MyProject"

# 5. CI용 JSON 출력
python scripts/quality/ue_arch_lint.py --json --project "C:/MyProject"

# 6. 특정 룰 비활성화
python scripts/quality/ue_arch_lint.py --disable ARCH-003,ARCH-010
```

**`--init` 동작**: 프로젝트의 Build.cs를 분석해서 현재 최대 의존성 수 + 여유분 2를 `ARCH-006` 기본값으로 설정하고, GAS 미사용 프로젝트면 `ARCH-008`을 자동 비활성화합니다.

**Exit code**: 0 = 에러 없음 (경고 허용), 1 = 에러 발견, 2 = 스크립트 오류

**Script path**: `scripts/quality/ue_arch_lint.py`

> **Release Package Note**: 현재 릴리즈 패키지(`package_release.py`)에는 이 스크립트가 미포함.
> `copy_scripts()`의 `skill_scripts` 리스트에 등록 필요 — 별도 이슈로 추적 예정.

---

## Related Skills

- [`/ue-perf-lint`](../ue-perf-lint/SKILL.md) — Performance anti-patterns (PERF-001~023). ARCH-009 reuses its results.
- [`/ue-pre-commit`](../ue-pre-commit/SKILL.md) — Pre-commit safety analysis (complementary: run architecture-lint before pre-commit)
- [`/ue-audit`](../ue-audit/SKILL.md) — Project-wide health audit (broader scope)
- [`/ue-diff`](../ue-diff/SKILL.md) — Cross-check and mismatch detection (ARCH-007 reuses replication_audit)

---

## MCP Tool Examples

```python
# Inheritance depth check (ARCH-001)
ue_analyze_symbols(operation="trace_hierarchy", class_name="AMyCharacter", direction="up")

# Replication audit (ARCH-007)
ue_diff(operation="replication_audit", class_name="AMyCharacter")

# Module dependency map (ARCH-004)
ue_analyze_source(operation="map_dependencies")

# GAS bypass detection (ARCH-008)
ue_grep(params={"query": "Health -= ", "domain": "source"})
```
