---
name: ue-audit
description: "Purpose: Comprehensive UE project health audit with 6-point diagnostic. Checks server health, config consistency, asset quality, performance, code quality, and message log diagnostics. Answers 'Is this system healthy?' with scored report and prioritized recommendations. Triggers: 'audit', 'health check', 'project health', '감사', '건강 검진', '프로젝트 상태', 'system check', 'quality check', 'validate project', '경고 요약', '메시지 로그', 'message log', 'auto fix warnings'."
argument-hint: ["(no args = full audit)", "--config (config only)", "--performance (perf only)", "--assets (assets only)", "--messages (message log only)"]
---

# UE Audit — System Health Audit

**Version**: 1.1.0
**Issue**: #4800, #5296, #5458
**Purpose**: Answer "Is this system/project healthy?" with comprehensive 6-point diagnostic

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the audit workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Parse scope** from argument (full audit or specific category)
2. **Execute all applicable audit checks** sequentially
3. **Score each category** (Pass/Warning/Fail)
4. **Generate audit report** with overall score and recommendations

---

## Auto-Trigger Phrases

### Korean
- "프로젝트 건강 검진", "시스템 감사"
- "프로젝트 상태 확인", "전체 점검"
- "빌드 전 검증", "품질 확인"
- "서버 상태 확인", "MCP 헬스체크"
- "메시지 로그 확인", "경고 요약", "자동 수정"
- "자동 수정 가능한 경고", "MessageLog 진단"

### English
- "Audit the project", "Health check"
- "Is the project healthy?", "System status"
- "Pre-build validation", "Quality check"
- "Check server status", "Project diagnostic"
- "Message log", "Warning summary", "Auto fix warnings"
- "MessageLog warnings", "Show fixable warnings"

---

## 6-Point Audit Framework

### Scope Selection

```python
AUDIT_SCOPES = {
    "full":        [1, 2, 3, 4, 5, 6],  # Default: all 6 checks
    "--health":    [1],                   # Server health only
    "--config":    [2],                   # Config audit only
    "--assets":    [3],                   # Asset quality only
    "--perf":      [4],                   # Performance only
    "--code":      [5],                   # Code quality only
    "--messages":  [6],                   # Message log diagnostics only
}
```

### Check 1: Server Health

```python
health = ue_check_health()
# Checks:
# - MCP server connectivity
# - PDB index status (loaded, file count, last updated)
# - Project path validation
# - Engine version detection
```

**Scoring**:
- Pass: All checks green
- Warning: PDB outdated (>24 hours) or partial index
- Fail: Server unreachable or project path invalid

### Check 2: Configuration Audit

```python
# Search for common problematic configs
config_issues = []

# Check for deprecated CVars
# Note: search_config rejects "*" and empty strings. Use exact key or partial substring.
deprecated_check = ue_analyze_config(operation="search_config", params={
    "key": "r.AllowOcclusionQueries"  # Exact key or partial substring (e.g., "r.Shadow")
})

# Check for conflicting overrides
override_check = ue_analyze_config(operation="find_overrides", params={
    "option": "r.Shadow"  # Use 'option', not 'key'
})
```

**Scoring**:
- Pass: No deprecated CVars, no conflicting overrides
- Warning: Deprecated CVars found or minor conflicts
- Fail: Critical config conflicts that affect gameplay

### Check 3: Asset Quality

```python
# Check naming conventions
# Note: query="*" may return 0 if metadata cache not fully loaded. Use specific prefixes.
bp_assets = ue_search_assets(params={"query": "BP_*", "asset_type": "blueprint"})
# Flag: BPs not starting with BP_, Materials not starting with M_, etc.

# Check for common issues
material_assets = ue_search_assets(params={"query": "M_*", "asset_type": "material"})
niagara_assets = ue_search_assets(params={"query": "NS_*", "asset_type": "niagara"})

# Cross-BP dependency health (detect circular BP dependencies)
ToolSearch("select:mcp__narshamcp__ue_analyze_symbols")
cross_bp = ue_analyze_symbols(operation="find_cross_bp_dependencies", params={
    "class_name": "<main game class>"  # e.g., project's primary character or game mode
})
```

**Naming Convention Rules**:
| Type | Expected Prefix |
|------|----------------|
| Blueprint | `BP_` |
| Material | `M_` |
| Material Instance | `MI_` |
| Niagara System | `NS_` |
| GameplayAbility | `GA_` |
| GameplayEffect | `GE_` |
| StateTree | `ST_` |
| BehaviorTree | `BT_` |
| Animation Montage | `AM_` |
| Widget Blueprint | `WBP_` |

**Scoring**:
- Pass: >90% naming convention compliance
- Warning: 70-90% compliance
- Fail: <70% compliance

### Check 4: Performance Check

```python
ToolSearch("select:mcp__narshamcp__ue_analyze_insights")

# Get performance overview if trace data available
perf_stats = ue_analyze_insights(operation="get_stats", params={})

# Check for known hot functions
hot_functions = ue_analyze_insights(operation="get_hot_functions", params={})
```

**Scoring**:
- Pass: No critical hot functions, frame budget within limits
- Warning: 1-3 hot functions or occasional frame drops
- Fail: Critical bottlenecks or consistent frame drops

**Note**: This check requires .utrace data. If no trace data is available, report as "Skipped - No trace data. Run Unreal Insights to capture."

### Check 5: Code Quality

```python
ToolSearch("select:mcp__narshamcp__ue_fix_errors")

# Option A: If a build log exists, run preflight scan
preflight = ue_fix_errors(mode="preflight", build_log_path="<path>/Saved/Logs/Build.log")

# Option B (recommended): Dependency check — no build log needed
dep_check = ue_fix_errors(mode="dependency_check")
# Returns: missing modules, circular deps, unused deps

# Option C: Hot-reload risk check — no build log needed
hotreload = ue_fix_errors(mode="hotreload_check", auto_detect_git=True)
# Returns: files changed that may cause hot-reload issues

# Option D: Dead UFUNCTION detection (finds unused BlueprintCallable functions)
ToolSearch("select:mcp__narshamcp__ue_analyze_symbols")
ufunction_stats = ue_analyze_symbols(operation="ufunction_usage", params={})
# Returns: total UFUNCTIONs, called count, unused count, usage percentage
```

**Important**: `preflight` mode requires `build_log_path`. If no build log is available,
use `dependency_check` (always works) or `hotreload_check` (requires git) as fallback.

**Scoring**:
- Pass: No errors, <5 warnings
- Warning: No errors, 5-20 warnings or minor dependency issues
- Fail: Any errors, missing critical dependencies, or circular deps

### Check 6: Message Log Diagnostics (Issue #5296, #5458)

```python
ToolSearch("select:mcp__narshamcp__ue_editor_debug")

# Step 1: Get category overview (no category = all categories with counts)
overview = ue_editor_debug(operation="get_message_log")
# Returns: category listing with message counts and severity breakdown

# Step 2: For categories with errors/warnings, get detailed messages
for category in categories_with_issues:
    details = ue_editor_debug(operation="get_message_log", params={
        "log_category": category,      # e.g., "MapCheck", "BlueprintLog", "AssetCheck"
        "severity_filter": "Warning",  # "All", "Info", "Warning", "Error"
        "max_messages": 50,
        "include_tokens": True         # Decompose into typed tokens (Text, Fix, Asset, etc.)
    })

# Step 3: Identify fixable messages (those with Fix tokens)
# Fix tokens appear in token decomposition with type="Fix"

# Step 4: Preview fixes with dry_run before executing
for fixable_message in messages_with_fix_tokens:
    preview = ue_editor_debug(operation="execute_message_fix", params={
        "log_category": category,
        "message_index": fixable_message.index,
        "fix_token_index": 0,          # Which fix token (default: first)
        "dry_run": True                 # Preview only — do NOT execute without user confirmation
    })

# Step 5: Execute fix (only with user confirmation) and re-query to verify
fix_result = ue_editor_debug(operation="execute_message_fix", params={
    "log_category": category,
    "message_index": fixable_message.index,
    "dry_run": False
})
# Re-query category to verify fix was applied
```

**Known Categories**: MapCheck, BlueprintLog, AssetCheck, PIE, LoadErrors, LightingResults, DataValidation, PackagingResults, SlateStyleLog, SourceControl, HotReload, LiveCoding, AssetTools

**SAFETY**: `execute_message_fix` is always called with `dry_run=True` in audit mode.
Actual fix execution should only happen when the user explicitly requests it after reviewing the audit report.

**Scoring**:
- Pass: No errors across all categories
- Warning: Warnings only, or fixable errors (Fix tokens available)
- Fail: Unfixable errors (no Fix tokens) in critical categories (MapCheck, PIE, LoadErrors)
- Skipped: Editor offline or not connected

**Note**: This check requires a running UE Editor with Remote Control enabled. If Editor is not connected, report as "Skipped - Editor not connected. Launch Editor with Remote Control plugin enabled."

---

## Output Format

```text
=== System Audit Report ===

--- Overall Score: [8.5/10] ---
Checks: 6 total | 3 Pass | 1 Warning | 2 Skipped

--- 1. Server Health: PASS ---
  MCP Server: Connected (uptime: 2h 15m)
  PDB Index: Loaded (12,450 symbols, updated 30m ago)
  Project Path: D:/MyProject (validated)
  Engine: UE 5.4.3

--- 2. Configuration: WARNING ---
  Deprecated CVars found: 1
    - r.AllowOcclusionQueries (deprecated in 5.3, use r.OcclusionQueries)
  Config conflicts: 0
  Total config keys checked: 15

--- 3. Asset Quality: PASS ---
  Total assets scanned: 245
  Naming compliance: 94% (231/245)
  Violations:
    - Enemy_AI (should be BP_EnemyAI)
    - RockMaterial (should be M_Rock)

--- 4. Performance: SKIPPED ---
  No .utrace data available.
  Run: UnrealEditor > Tools > Unreal Insights to capture trace.

--- 5. Code Quality: PASS ---
  Preflight result: Clean
  Errors: 0
  Warnings: 3
    - Unused #include in MyCharacter.h
    - Potential null dereference in WeaponSystem.cpp:145
    - Missing UPROPERTY() on replicated field

--- 6. Message Log: WARNING ---
  Categories scanned: 8
  Total messages: 23
    MapCheck: 5 warnings, 2 fixable (Fix tokens)
    BlueprintLog: 3 errors, 0 fixable
    AssetCheck: 15 info
  Fixable issues: 2 (dry_run previewed)

--- Recommendations (Priority Ordered) ---
  1. [Config] Replace deprecated r.AllowOcclusionQueries with r.OcclusionQueries
  2. [MsgLog] Fix 2 MapCheck warnings via auto-fix tokens
  3. [MsgLog] Investigate 3 BlueprintLog errors (no auto-fix available)
  4. [Asset] Rename 14 assets to follow naming conventions
  5. [Code] Fix 3 code warnings before next build
  6. [Perf] Capture performance trace for bottleneck analysis
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| Server health fails | MCP not connected | Report connection status, stop audit |
| Config search fails | No project configured | Use ue_check_health first to verify |
| Asset search empty | No assets in project | Report: "No assets found - is project path correct?" |
| Performance unavailable | No .utrace data | Skip with note, suggest how to capture |
| Preflight fails | No build_log_path | Use `dependency_check` or `hotreload_check` instead (no build log needed) |
| Message log empty | Editor not connected | Skip with note: "Launch Editor with Remote Control plugin enabled" |
| Fix token execution fails | Token expired or Editor state changed | Re-query messages, report stale state |

---

## Evaluation Criteria

### Activation Test Cases

**Positive (7)** - Should activate:
1. "프로젝트 상태 확인해줘" -> Activate (full audit)
2. "Health check" -> Activate (full audit)
3. "/ue-audit --config" -> Activate (config only)
4. "서버 연결 확인" -> Activate (health only)
5. "빌드 전에 검증해줘" -> Activate (code quality focus)
6. "경고 요약 보여줘" -> Activate (MessageLog diagnostics focus)
7. "/ue-audit --messages" -> Activate (MessageLog only)

**Negative (3)** - Should NOT activate:
1. "빌드 에러 해결해줘" -> Use ue-debug skill
2. "APawn 구조 설명해" -> Use ue-explain skill
3. "새 기능 만들어줘" -> Use ue-scaffold skill

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| Health check accuracy | 100% |
| Config issue detection | >85% |
| Naming violation detection | >90% |
| Full audit time | <2 minutes |
| MessageLog detection rate | >90% when Editor online |

---

**Status**: Phase 3 (Issue #5458: MessageLog integration)
**Related**: Issue #4800 (MCP-First Tool Selection), Issue #5296 (MessageLog operations), Issue #5458 (MessageLog audit integration)
