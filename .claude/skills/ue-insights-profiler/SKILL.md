---
name: ue-insights-profiler
description: "Purpose: Comprehensive UE Insights profiling analysis. Answers 'Why is my game slow?' by analyzing .utrace files, identifying hot functions, detecting performance regressions, and providing optimization suggestions. Orchestrates ue_analyze_insights for CPU/GPU bottleneck identification. Triggers: '성능 분석', '프로파일링', '왜 느려', 'FPS 분석', 'utrace 분석', 'performance profiling', 'bottleneck analysis', 'why is my game slow', 'CPU/GPU analysis', 'hot function', 'trace analysis'."
argument-hint: ["trace file path (.utrace)", "target function or category", "comparison baseline trace"]
---

# UE Insights Profiler -- Performance Analysis Assistant

**Version**: 1.0.0
**Issue**: #6047
**Purpose**: Answer "Why is my game slow?" with systematic profiling from .utrace analysis to actionable optimization suggestions

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the profiling workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Detect analysis mode** from user query (general profiling, regression detection, or targeted function analysis)
2. **Execute ue_analyze_insights** with appropriate operation
3. **Cross-reference** with symbol analysis if hot functions are identified
4. **Generate performance report** with bottleneck identification and optimization suggestions

---

## Auto-Trigger Phrases

### Korean
- "왜 게임이 느려?", "성능 분석해줘"
- "프로파일링 결과 보여줘", "utrace 분석해줘"
- "FPS 드랍 원인 분석", "병목 찾아줘"
- "CPU 바운드인지 GPU 바운드인지 확인"
- "핫 함수 찾아줘", "성능 회귀 감지"
- "프레임 타임 분석", "최적화 제안해줘"
- "Insights 트레이스 비교", "이전 빌드 대비 성능 비교"

### English
- "Why is my game slow?", "Profile my game"
- "Analyze this utrace file", "Show profiling results"
- "Find the bottleneck", "What's causing FPS drops?"
- "Is this CPU or GPU bound?", "Hot function analysis"
- "Detect performance regression", "Compare traces"
- "Frame time breakdown", "Optimization suggestions"
- "Insights trace analysis", "Performance comparison with baseline"

---

## Analysis Mode Classification

### Step 1: Classify Analysis Intent

```python
ANALYSIS_CLASSIFIERS = {
    "general_profiling": {
        "keywords": ["느려", "slow", "성능 분석", "profile", "병목", "bottleneck",
                     "FPS", "프레임", "frame time", "왜 느려", "why slow",
                     "CPU bound", "GPU bound", "CPU 바운드", "GPU 바운드"],
        "operation": "smart_analyze",
        "description": "General performance profiling -- identify top bottlenecks"
    },
    "regression_detection": {
        "keywords": ["회귀", "regression", "비교", "compare", "이전 빌드", "baseline",
                     "slower than before", "이전보다 느려", "성능 저하", "degradation",
                     "delta", "diff"],
        "operation": "compare_traces",
        "description": "Compare two traces to detect performance regressions"
    },
    "hot_function_analysis": {
        "keywords": ["핫 함수", "hot function", "callers", "호출자", "콜 그래프",
                     "call graph", "누가 호출", "expensive function", "비용 높은 함수",
                     "inclusive time", "exclusive time"],
        "operation": "find_callers",
        "description": "Trace call graph for specific expensive functions"
    },
    "optimization_guidance": {
        "keywords": ["최적화", "optimization", "제안", "suggestions", "어떻게 고쳐",
                     "how to fix", "improve performance", "성능 개선", "줄이기"],
        "operation": "get_optimization_suggestions",
        "description": "Get actionable optimization recommendations"
    }
}
```

---

## Workflow

### Step 1: Health Check & Trace Discovery

Verify MCP server connectivity and discover available trace data.

```python
ToolSearch("select:mcp__narshamcp__ue_check_health")

health = ue_check_health()
# Confirm server is running and project path is valid
```

If user provides a `.utrace` file path, use it directly. Otherwise, auto-discover:

```python
ToolSearch("select:mcp__narshamcp__ue_analyze_insights")

# Smart analyze auto-detects available traces
result = ue_analyze_insights(operation="smart_analyze", params={
    "trace_path": "<.utrace file path if provided>"
})
```

**Extract from response**: Available traces, session info, initial performance overview.

### Step 2: Bottleneck Identification

Based on classification from Step 1, execute the appropriate deep analysis.

#### Path A: General Profiling (smart_analyze)

```python
# Full profiling analysis -- identifies CPU/GPU split, top functions, frame breakdown
result = ue_analyze_insights(operation="smart_analyze", params={
    "trace_path": "<.utrace path>",
    "top_n": 20
})
# Returns: frame_time_breakdown, cpu_vs_gpu, hot_functions[], thread_utilization
```

#### Path B: Regression Detection (compare_traces)

```python
# Compare current trace against baseline
result = ue_analyze_insights(operation="compare_traces", params={
    "trace_path": "<current .utrace>",
    "baseline_path": "<baseline .utrace>",
    "threshold_ms": 0.5
})
# Returns: regressions[], improvements[], unchanged[], delta_summary
```

#### Path C: Hot Function Deep Dive (find_callers)

```python
# Trace call graph for a specific expensive function
result = ue_analyze_insights(operation="find_callers", params={
    "function_name": "<expensive function>",
    "trace_path": "<.utrace path>",
    "max_depth": 10
})
# Returns: call_chain[], inclusive_time, exclusive_time, call_count
```

#### Path D: Optimization Suggestions

```python
# Get actionable optimization recommendations
result = ue_analyze_insights(operation="get_optimization_suggestions", params={
    "trace_path": "<.utrace path>",
    "category": "all"  # or "cpu", "gpu", "memory", "rendering", "game_thread"
})
# Returns: suggestions[] with priority, estimated_impact, implementation_steps
```

### Step 3: Cross-Reference with Symbol Analysis

When hot functions are identified, cross-reference with C++ symbol data for deeper context.

```python
ToolSearch("select:mcp__narshamcp__ue_analyze_symbols")

# For each top hot function, get implementation details
for func in hot_functions[:5]:
    symbol = ue_analyze_symbols(operation="smart", params={
        "function_name": func.name
    })
    # Returns: file location, class hierarchy, callers in source

    # If function is in project code, check for known anti-patterns
    if symbol.is_project_code:
        impact = ue_analyze_symbols(operation="impact_analysis", params={
            "function_name": func.name
        })
```

### Step 4: Config-Driven Performance Check (Optional)

Check if performance-related CVars are misconfigured.

```python
ToolSearch("select:mcp__narshamcp__ue_analyze_config")

# Search for performance-related config overrides
config = ue_analyze_config(operation="search_config", params={
    "key": "r.Shadow"  # or other performance-related prefix
})

# Common performance CVars to check
PERF_CVARS = [
    "r.Shadow.MaxResolution",
    "r.Streaming.PoolSize",
    "gc.TimeBetweenPurgingPendingKillObjects",
    "r.ScreenPercentage",
    "r.VSync",
    "tick.AllowAsyncTickDispatch"
]
```

### Step 5: Generate Report

Compile all findings into the structured output format below.

---

## Output Format

```
=== Insights Performance Report ===

--- Environment ---
Project: [project name]
Trace File: [.utrace path or "live session"]
Trace Duration: [X seconds]
Platform: [Win64 / Linux / etc.]

--- Frame Time Summary ---
Average Frame Time: [X.XX ms] ([Y FPS])
P95 Frame Time:     [X.XX ms]
P99 Frame Time:     [X.XX ms]
Budget:             [16.67 ms (60 FPS) / 33.33 ms (30 FPS)]
Status:             [WITHIN BUDGET / OVER BUDGET by X.XX ms]

--- Bottleneck Classification ---
Primary Bottleneck: [CPU Game Thread / CPU Render Thread / GPU / Memory / I/O]
CPU vs GPU Split:   [CPU X.XX ms | GPU X.XX ms]
Limiting Factor:    [specific subsystem or function]

--- Top 10 Hot Functions ---
| # | Function | Inclusive (ms) | Exclusive (ms) | Calls | Thread |
|---|----------|---------------|----------------|-------|--------|
| 1 | [FuncName] | [X.XX] | [X.XX] | [N] | [GameThread] |
| 2 | ... | ... | ... | ... | ... |

--- Regression Analysis (if baseline provided) ---
Regressions (>0.5ms delta):
  [FuncA]: +X.XX ms (was Y.YY ms, now Z.ZZ ms) -- [possible cause]
  [FuncB]: +X.XX ms ...

Improvements:
  [FuncC]: -X.XX ms ...

--- Optimization Suggestions ---
Priority 1 (High Impact):
  [Suggestion with estimated savings and implementation steps]

Priority 2 (Medium Impact):
  [Suggestion ...]

Priority 3 (Low Hanging Fruit):
  [Suggestion ...]

--- CVar Recommendations ---
[CVar]: Current=[value] Recommended=[value] Impact=[description]

--- Next Steps ---
1. [Actionable next step]
2. [Actionable next step]
3. [Actionable next step]
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| No .utrace file found | User hasn't captured a trace yet | Guide user to capture: UnrealInsights -trace=cpu,gpu,frame or in-editor Trace menu |
| smart_analyze returns empty | Trace file corrupted or incompatible version | Verify file with `ue_analyze_insights(operation="get_trace_info")`, suggest re-capture |
| compare_traces baseline missing | Baseline trace path invalid or deleted | Run single-trace analysis with `smart_analyze` instead; recommend saving baselines |
| find_callers function not found | Function name doesn't match trace symbols | Use `ue_analyze_symbols(search_symbols)` to find correct function name, retry |
| get_optimization_suggestions empty | Trace too short or no significant bottlenecks | Report "no significant bottlenecks detected"; suggest longer capture or stress test scenario |
| MCP server connection failed | Server not running or project path wrong | Run `ue_check_health()` to diagnose; verify `.mcp.json` configuration |
| Trace file too large (>1GB) | Extended profiling session | Suggest trimming trace range or using `smart_analyze` with `time_range` parameter |

---

## Tool Disambiguation

| Scenario | This Skill (ue-insights-profiler) | ue-debug (performance path) |
|----------|----------------------------------|---------------------------|
| "Why is my game slow?" | Full profiling workflow with trace analysis | Quick bottleneck check as part of general debugging |
| "Analyze this .utrace file" | Primary handler -- deep trace analysis | Would not activate |
| "FPS drop cause" | Deep analysis with regression detection | Quick diagnosis, may route here |
| Trace comparison | compare_traces with delta analysis | Not supported |
| Optimization suggestions | get_optimization_suggestions with priorities | General performance tips only |

**Rule**: If the query is specifically about profiling, traces, or systematic performance analysis, use this skill. If performance is one symptom among others (e.g., "game crashes and is slow"), use ue-debug which may route to this skill's tools.

---

## Evaluation Criteria

### Activation Test Cases

**Positive (6)** - Should activate:
1. "왜 게임이 느려?" -> Activate (general_profiling)
2. "Analyze this utrace file at D:/Traces/session.utrace" -> Activate (general_profiling)
3. "이전 빌드 대비 성능 비교해줘" -> Activate (regression_detection)
4. "USkeletalMeshComponent::RefreshBoneTransforms 왜 이렇게 비싸?" -> Activate (hot_function_analysis)
5. "What optimization suggestions do you have?" -> Activate (optimization_guidance)
6. "Is this CPU bound or GPU bound?" -> Activate (general_profiling)

**Negative (4)** - Should NOT activate:
1. "빌드 에러 해결해줘" -> Use ue-debug skill (compile_error)
2. "ACharacter 어떻게 동작해?" -> Use ue-explain skill
3. "r.Shadow.MaxResolution 뭐야?" -> Use ue-cvar-explorer skill (single CVar query)
4. "캐릭터 만들어줘" -> Use ue-scaffold skill

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| Bottleneck classification accuracy (CPU vs GPU) | >90% |
| Hot function identification (top 5 match) | >85% |
| Regression detection (>0.5ms delta) | >95% |
| Optimization suggestion relevance | >80% |

---

**Status**: Phase 1 MVP
**Related**: Issue #6047
