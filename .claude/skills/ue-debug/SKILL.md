---
name: ue-debug
description: "Purpose: Systematic UE debugging assistant with symptom-to-cause diagnosis. Answers 'Why is X happening?' by classifying symptoms (compile error, runtime crash, unexpected behavior, performance) and routing to optimal diagnostic tools. Medical 4-step model: Symptoms -> Diagnosis -> Root Cause -> Treatment. Triggers: 'debug', 'why is', 'error', 'crash', 'not working', '디버그', '왜 이런', '에러', '크래시', '안돼', 'fix', 'broken', 'bug'."
argument-hint: ["symptom description or error message"]
---

# UE Debug — Systematic Debugging Assistant

**Version**: 1.0.0
**Issue**: #4800
**Purpose**: Answer "Why is X happening?" with systematic diagnosis from symptom to root cause

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the diagnostic workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Extract symptom** from user query or argument
2. **Classify symptom** into one of 5 categories
3. **Execute diagnostic tools** based on classification
4. **Generate debug report** with root cause and fix

---

## Auto-Trigger Phrases

### Korean
- "왜 X가 발생해?", "X 에러 해결해줘"
- "빌드 에러 고쳐줘", "크래시 원인 찾아줘"
- "X가 안돼", "X가 작동 안해"
- "성능이 느려", "FPS 드랍 원인"
- "어빌리티가 안 발동돼"

### English
- "Why is X happening?", "Fix this error"
- "Debug this crash", "Why doesn't X work?"
- "Build error help", "What's causing this?"
- "Performance is slow", "FPS drop cause"
- "Ability not activating"

---

## Symptom Classification & Diagnostic Routing

### Step 1: Classify Symptom

```python
SYMPTOM_CLASSIFIERS = {
    "compile_error": {
        "keywords": ["error C", "LNK", "undeclared", "undefined", "unresolved",
                     "cannot convert", "빌드 에러", "컴파일 에러", "링커 에러",
                     "GENERATED_BODY", "missing", "expected"],
        "tool": "ue_fix_errors",
        "operation": "smart"
    },
    "runtime_crash": {
        "keywords": ["crash", "assert", "ensure", "access violation", "nullptr",
                     "segfault", "fatal error", "크래시", "런타임 에러",
                     "unhandled exception", "stack overflow"],
        "tool": "ue_fix_errors",
        "operation": "crash_analysis"
    },
    "unexpected_behavior": {
        "keywords": ["doesn't work", "not working", "wrong", "안돼", "안 돼",
                     "작동 안", "이상해", "unexpected", "broken", "bug",
                     "not activating", "not triggering", "안 발동"],
        "tool": "ue_trace_execution",  # + ue_analyze_config
        "operation": "smart"
    },
    "performance": {
        "keywords": ["slow", "FPS", "hitch", "lag", "느려", "프레임", "성능",
                     "bottleneck", "frame drop", "GPU", "CPU bound",
                     "memory", "메모리"],
        "tool": "ue_analyze_insights",
        "operation": "diagnose_bottleneck"
    },
    "gas_issue": {
        "keywords": ["ability", "gameplay tag", "gameplay effect", "attribute",
                     "어빌리티", "태그", "이펙트", "GAS",
                     "not granting", "not activating", "blocked by"],
        "tool": "ue_manage_gameplay",  # + ue_analyze_config
        "operation": "trace_abilities"
    }
}
```

### Step 2: Execute Diagnostic Tools

#### Path A: Compile Error
```python
ToolSearch("select:mcp__narshamcp__ue_fix_errors")

# Smart mode auto-parses error and suggests fix
result = ue_fix_errors(mode="smart", params={
    "error_message": "<paste error here>",
    "project_root": "<project_path>"
})
# Returns: error classification, root cause, suggested fix, code changes
```

#### Path B: Runtime Crash
```python
ToolSearch("select:mcp__narshamcp__ue_fix_errors")

# Crash analysis — scans Saved/Crashes/ automatically
result = ue_fix_errors(mode="crash_analysis")
# Optional: specify crash_path for a specific crash directory
# result = ue_fix_errors(mode="crash_analysis", crash_path="<path>/Saved/Crashes/<crash_dir>")

# Alternative: runtime log analysis (scans Saved/Logs/*.log)
result = ue_fix_errors(mode="runtime_analysis")
# Optional: specify log_path for a specific log file

# If crash mentions a specific symbol, also look it up
ToolSearch("select:mcp__narshamcp__ue_analyze_symbols")
symbol_info = ue_analyze_symbols(operation="search_symbols", params={
    "query": "<crash function name>"
})

# Runtime variable inspection (requires Editor running)
ToolSearch("select:mcp__narshamcp__ue_editor_debug")
runtime = ue_editor_debug(operation="watch_variable", params={
    "actor": "<actor name>",
    "variable": "<variable to watch>"
})
```

#### Path C: Unexpected Behavior
```python
# Trace execution to find where behavior diverges
ToolSearch("select:mcp__narshamcp__ue_trace_execution")
trace = ue_trace_execution(operation="smart", params={
    "target": "<blueprint or function name>",
    "output_format": "html"  # Interactive HTML for visual debugging in browser
})
# output_format options: "json" (default), "html" (interactive), "mermaid" (diagram)

# Check if delegate/event binding is broken ("event not firing" debugging)
ToolSearch("select:mcp__narshamcp__ue_analyze_symbols")
delegates = ue_analyze_symbols(operation="find_delegate_refs", params={
    "target": "<event or delegate name>"
})

# Check if config is overriding expected behavior
ToolSearch("select:mcp__narshamcp__ue_analyze_config")
config = ue_analyze_config(operation="search_config", params={
    "key": "<relevant config key>"
})
```

#### Path D: Performance
```python
ToolSearch("select:mcp__narshamcp__ue_analyze_insights")

# Diagnose bottleneck
result = ue_analyze_insights(operation="diagnose_bottleneck", params={
    "trace_path": "<.utrace file if available>"
})

# Or get stats overview
result = ue_analyze_insights(operation="get_stats", params={})
```

#### Path E: GAS Issue
```python
ToolSearch("select:mcp__narshamcp__ue_manage_gameplay")

# Trace ability chain to find where it breaks
result = ue_manage_gameplay(operation="trace_abilities", params={
    "tag": "<ability tag or class name>"
})

# Check if config is blocking
ToolSearch("select:mcp__narshamcp__ue_analyze_config")
config = ue_analyze_config(operation="search_config", params={
    "key": "AbilitySystem"
})
```

---

## Output Format (Medical Model)

```text
=== Debug Report: [Symptom Summary] ===

--- Step 1: Symptoms ---
Classification: [Compile Error / Runtime Crash / Unexpected Behavior / Performance / GAS Issue]
Reported symptom: [User's description]
Error details: [Parsed error message if applicable]

--- Step 2: Diagnosis ---
Tool used: [ue_fix_errors / ue_analyze_insights / etc.]
Findings:
  [Detailed tool results — what was found]

--- Step 3: Root Cause ---
[Clear explanation of WHY this is happening]
Example: "The GENERATED_BODY() macro is missing from line 15 of MyActor.h.
         UE requires this macro in every UCLASS to generate reflection data."

--- Step 4: Fix ---
[Step-by-step fix instructions]

1. Open [file path]
2. Add/modify [specific code change]:
   ```cpp
   // Before:
   class AMyActor : public AActor
   {
       UCLASS()

   // After:
   UCLASS()
   class AMyActor : public AActor
   {
       GENERATED_BODY()
   ```
3. Rebuild project

--- Prevention ---
[How to avoid this in the future]
- [Specific practice or check to add]
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| ue_fix_errors smart fails | No error_message provided | Ask user for full error log text |
| ue_fix_errors preflight fails | Requires build_log_path | Use `dependency_check` or `hotreload_check` as fallback |
| crash_analysis empty | No crash reports in Saved/Crashes/ | Use `runtime_analysis` to scan Saved/Logs/ instead |
| Performance trace unavailable | No .utrace file | Use `ue_analyze_insights(get_stats)` for general overview |
| GAS trace empty | Ability not in project | `ue_search_assets` to find similar abilities |
| Symptom classification ambiguous | Multiple keyword matches | Classify as highest priority match (compile > crash > behavior) |

---

## Evaluation Criteria

### Activation Test Cases

**Positive (5)** - Should activate:
1. "error C2065: 'MyVar': undeclared identifier 해결해줘" -> Activate (compile error)
2. "게임 실행하면 BeginPlay에서 크래시 발생" -> Activate (runtime crash)
3. "GA_Attack 어빌리티가 안 발동돼" -> Activate (GAS issue)
4. "프레임이 30FPS 아래로 떨어져" -> Activate (performance)
5. "Why is my character not moving?" -> Activate (unexpected behavior)

**Negative (3)** - Should NOT activate:
1. "ACharacter 어떻게 동작해?" -> Use ue-explain skill
2. "APawn 변경 영향 분석" -> Use ue-impact skill
3. "캐릭터 클래스 만들어줘" -> Use ue-scaffold skill

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| Symptom classification accuracy | >90% |
| Root cause identification | >80% |
| Fix suggestion accuracy | >75% |
| Compile error auto-fix | >95% (via ue_fix_errors smart mode) |

---

**Status**: Phase 1 MVP
**Related**: Issue #4800 (MCP-First Tool Selection)
