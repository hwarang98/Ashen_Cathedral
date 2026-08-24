# Blueprint Flow Workflow - Complete Reference

Complete API reference for Blueprint flow analysis with hybrid Method A/B execution.

---

## Table of Contents

1. [3-Phase Workflow Details](#1-3-phase-workflow-details)
2. [MCP Tool Parameters](#2-mcp-tool-parameters)
3. [Workflow Definition](#3-workflow-definition)
4. [GAS Routing Logic](#4-gas-routing-logic)
5. [Performance Metrics](#5-performance-metrics)
6. [Error Handling](#6-error-handling)
7. [Output Formats](#7-output-formats)
8. [External Links](#8-external-links)

---

## 1. 3-Phase Workflow Details

### Phase 1: Structure Analysis

**Purpose**: Get Blueprint graph structure before tracing

**Tool**: `ue_analyze_blueprint(operation="get_structure")`

**Output**: Entry points, nodes, connections, components

### Phase 2: Execution Tracing (Hybrid)

#### Method A: Step-by-step (Baseline)

**Step 1: trace_from_input**

```python
ue_analyze_blueprint(
    operation="trace_from_input",
    params={"key": "{TARGET}"}
)
```

**Step 2: find_input_events**

```python
ue_analyze_blueprint(
    operation="find_input_events",
    params={"action_name": "IA_Attack"}
)
```

**Step 3: trace_execution_flow**

```python
ue_analyze_blueprint(
    operation="trace_execution_flow",
    params={
        "start_node": "K2Node_InputAction_123",
        "blueprint_name": "BP_PlayerCharacter",
        "max_depth": 10
    }
)
```

**Step 4: search_animations (optional)**

```python
ue_analyze_blueprint(
    operation="search_animations",
    params={"query": "Attack_Combo1"}
)
```

#### Method B: Workflow Shortcut

```python
ue_run_workflow(
    workflow="input-to-action",
    target="LeftMouseButton"
)
```

### Phase 3: Result Review

**Markdown Format**:

```markdown
Complete Execution Flow: {TARGET}

Input Mapping
   {Key} → {InputAction} ({Context})

Event Binding
   {InputAction} → {Blueprint}::{Event Node}

Blueprint Logic ({N} steps)
   1. {Step 1}
   2. {Step 2}
   ...

Animation (if applicable)
   {Montage Name} (Length: {Duration}s)
   - Frame {X}: {AnimNotify} ({Description})
```

---

## 2. MCP Tool Parameters

### ue_analyze_blueprint

**Operation**: `get_structure`

```python
{"operation": "get_structure", "params": {"blueprint_name": str, "project_root": str (optional)}}
```

**Operation**: `trace_from_input`

```python
{"operation": "trace_from_input", "params": {"key": str, "project_root": str (optional)}}
```

**Operation**: `find_input_events`

```python
{"operation": "find_input_events", "params": {"action_name": str, "blueprint_name": str (optional)}}
```

**Operation**: `trace_execution_flow`

```python
{"operation": "trace_execution_flow", "params": {
    "start_node": str, "blueprint_name": str, "max_depth": int (default: 10), "end_node": str (optional)
}}
```

**Operation**: `search_animations`

```python
{"operation": "search_animations", "params": {"query": str}}
```

**Operation**: `analyze_execution_depth`

```python
{"operation": "analyze_execution_depth", "params": {"blueprint_name": str}}
```

### ue_trace_execution

| Operation | Purpose | Parameters |
|-----------|---------|------------|
| `trace_execution_flow` | Trace from start node | `blueprint_name`, `start_node`, `max_depth` |
| `trace_ability_flow` | Trace GAS ability | `ability_name`, `blueprint_name` |

### ue_run_workflow

```python
{"workflow": str, "target": str}
```

**Available Workflows**:

- `input-to-action`: Hardware key → Execution flow
- `ability-tag-trace`: GameplayTag → Abilities/Blueprints
- `quick-inspect`: Class → Methods/Properties
- `animation-notify-impact`: AnimNotify → Gameplay impact

### ue_manage_blueprint

| Operation | Purpose | Parameters |
|-----------|---------|------------|
| `get_structure` | Get Blueprint overview | `blueprint_name` |
| `search_blueprints` | Find by pattern | `query`, `parent_class` |
| `get_functions` | List all functions | `blueprint_name` |
| `get_variables` | List all variables | `blueprint_name` |
| `visualize_structure` | Generate diagram | `blueprint_name` |

---

## 3. Workflow Definition

**Location**: `docs/user-guides/workflows/input-to-action.md`

**Metadata**:

```yaml
workflow_id: input-to-action
name: Input to Action Flow Tracer
category: gameplay
version: 1.0.0
estimated_time: 12-18 seconds
tags: [input, enhanced-input, blueprint, execution-flow]
token_usage: ~800 tokens
trigger_keywords:
  korean: ["키 누르면", "입력 추적", "E키", "버튼 눌렀을 때"]
  english: ["what happens when", "key press", "input trace", "button action"]
```

**Variables**: `{TARGET}` - Hardware key from user query

**Steps**: 3 (trace_from_input → find_input_events → trace_execution_flow)

---

## 4. GAS Routing Logic (Issue #3876)

### When to Apply

- Query contains: "어빌리티", "ability", "GA_", "쿨다운", "발사 속도", "cooldown", "fire rate"
- Lyra-style projects with InputTag → Ability chains

### Routing Table

```python
GAS_ROUTING = {
    "ability_activation": ("ue_manage_gameplay", "trace_abilities"),  # FIRST CHOICE
    "ability_trigger": ("ue_manage_gameplay", "trace_abilities"),
    "fire_rate": ("ue_manage_gameplay", "trace_abilities"),
    "cooldown": ("ue_manage_gameplay", "trace_abilities"),
    "tag_search": ("ue_manage_gameplay", "search_tags"),
}
```

### Priority

1. `ue_manage_gameplay(trace_abilities)` — FIRST for Ability queries
2. `ue_trace_execution(trace_ability_flow)` — if trace_abilities insufficient
3. `ue_analyze_blueprint(get_structure)` — for Blueprint-level structure

---

## 5. Performance Metrics

### Method A (Baseline)

| Metric | Value |
|--------|-------|
| **Total Time** | 4.2s |
| **Total Tokens** | 8K |
| **MCP Calls** | 4 |
| **Success Rate** | 90% |

**Step Breakdown**:

| Step | Avg Time | Avg Tokens |
|------|----------|------------|
| trace_from_input | 1.05s | 1,843 |
| find_input_events | 1.12s | 2,156 |
| trace_execution_flow | 1.87s | 3,421 |
| search_animations | 0.19s | 722 |

### Method B (Workflow)

| Metric | Value |
|--------|-------|
| **Total Time** | 1.8s |
| **Total Tokens** | 3K |
| **MCP Calls** | 1 |
| **Success Rate** | 90% |

### Improvement

- **Time**: 57% faster
- **Tokens**: 62% reduction
- **Decisions**: 75% fewer

---

## 6. Error Handling

### Common Errors

**Key not mapped**:

```json
{"success": false, "error": "Key 'Q' has no InputAction mapping.", "suggestion": "Check InputMappingContext in Project Settings"}
```

**No event bindings**:

```json
{"success": false, "error": "InputAction 'IA_Attack' found but no Blueprint uses it.", "suggestion": "Use ue_analyze_symbols to find C++ handlers"}
```

**Workflow timeout**:

```json
{"success": false, "error": "Workflow Step 3 timeout", "suggestion": "Use Method A with max_depth=5"}
```

### Fallback Strategies

**If Method B fails** → Fallback to Method A

```python
try:
    result = ue_run_workflow("input-to-action", target)
except WorkflowTimeout:
    result = manual_trace_step_by_step(target)
```

**If Step 3 times out** → Reduce max_depth

```python
try:
    flow = ue_analyze_blueprint(operation="trace_execution_flow", max_depth=10)
except Timeout:
    flow = ue_analyze_blueprint(operation="trace_execution_flow", max_depth=5)
```

---

## 7. Output Formats

### Mermaid Diagram Template

```mermaid
graph TD
    A[{Key}] --> B[{InputAction}]
    B --> C[{Blueprint} Event]
    C --> D{{Condition}}
    D -->|Yes| E[{Ability}]
    E --> F[{Montage}]
    F --> G[{AnimNotify}]
    D -->|No| H[Ignore]

    style A fill:#f9f
    style E fill:#bbf
    style G fill:#bfb
```

### JSON Structure

```json
{
  "blueprint_name": "BP_Player",
  "functions": [],
  "variables": [],
  "components": []
}
```

### Entry Point Detection

| Entry Point | Description |
|-------------|-------------|
| `Event BeginPlay` | Actor initialization |
| `Event Tick` | Per-frame update |
| `InputAction.*` | Player input |
| `Event Construct` | Widget construction |

---

## 8. External Links

### Issues

- Issue #1715 - Blueprint Flow Workflow
- Issue #262 - Workflow-Skill integration POC
- Issue #3876 - GAS Tool Selection
- Issue #107 - Delegate tracking
- Issue #29 - Real-time updates

### Unreal Engine Documentation

- [Enhanced Input System](https://docs.unrealengine.com/5.0/enhanced-input-in-unreal-engine/)
- [Blueprint Execution Flow](https://docs.unrealengine.com/5.0/blueprint-execution-flow-in-unreal-engine/)
- [Gameplay Ability System](https://docs.unrealengine.com/5.0/gameplay-ability-system-in-unreal-engine/)

---

**Last Updated**: 2026-02-13
