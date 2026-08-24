---
name: ue-execution-trace
description: "Purpose: Trace Blueprint/GAS execution flows using ue_trace_execution. Answers 'What happens when X fires?', 'Trace the ability flow', 'Follow input to action'. Supports BP flow, ability chains, input routing, gameplay tag propagation, and widget events. Triggers: '실행 추적', '실행 플로우', '어빌리티 플로우', '입력 추적', 'trace execution', 'execution flow', 'ability flow', 'trace from input', 'what happens when'."
argument-hint: ["target blueprint or ability", "--input (from input)", "--ability (ability flow)", "--tag (tag propagation)"]
---

# UE Execution Trace

**Version**: 1.1.0
**Issue**: #6098
**Purpose**: Trace Blueprint and GAS execution flows end-to-end

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements
1. **Parse target** from user query (Blueprint, ability, input action, or tag)
2. **Detect trace type**: execution flow, ability flow, input trace, or tag propagation
3. **Execute MCP trace calls** with appropriate parameters
4. **Generate flow report** with step-by-step execution chain

---

## Auto-Trigger Phrases

### Korean
- "실행 추적해줘", "실행 플로우 분석"
- "어빌리티 플로우 추적", "GAS 플로우"
- "입력 추적", "키 입력부터 추적"
- "태그 전파 추적", "이벤트 추적"
- "위젯 이벤트 추적"

### English
- "Trace execution flow", "What happens when X fires"
- "Trace ability flow", "GAS flow analysis"
- "Trace from input", "Follow input to action"
- "Trace gameplay tag", "Tag propagation"
- "Trace widget events"

---

## Workflow

### Step 1: Target Discovery

Identify trace target and available Blueprints:

```python
# List available Blueprints for tracing
ue_trace_execution(operation="list_blueprints", params={})

# Get execution trace stats
ue_trace_execution(operation="get_stats", params={})
```

### Step 2: Route to Trace Type

Based on user intent, select the appropriate trace operation:

#### 2A: Blueprint Execution Flow (default)

```python
ue_trace_execution(operation="trace_execution_flow", params={
    "blueprint_name": "<BP_Name>",
    "start_node": "<optional_start_node>",  # e.g., "BeginPlay", "EventTick"
    "end_node": "<optional_end_node>",      # Specific path finding between two nodes
    "output_format": "json",                # json (default), html (interactive), mermaid (diagram)
    "max_depth": 10                         # Trace depth limit (default 10)
})
```

#### 2B: Ability Flow (GAS)

```python
ue_trace_execution(operation="trace_ability_flow", params={
    "ability_name": "<GA_Name>"  # e.g., "GA_Weapon_Fire"
})
```

#### 2C: Input Chain

```python
ue_trace_execution(operation="trace_from_input", params={
    "input_action": "<IA_Name>"  # e.g., "IA_Jump", "IA_Fire"
})
```

#### 2D: Gameplay Tag Propagation

```python
# List available tags first
ue_trace_execution(operation="list_tags", params={})

# Trace tag usage
ue_trace_execution(operation="trace_gameplay_tag", params={
    "tag": "<Tag.Name>"  # e.g., "Ability.Attack.Melee"
})
```

#### 2E: Widget Events

```python
ue_trace_execution(operation="trace_widget_events", params={
    "widget_name": "<WBP_Name>"
})
```

### Step 3: Deep Dive (optional)

For complex flows, combine with symbol analysis:

```python
# If C++ functions are involved, trace into native code
ue_analyze_symbols(operation="find_callers", params={
    "function_name": "<native_function>"
})
```

---

## Output Format

```text
=== Execution Trace Report ===

Target: <target_name>
Trace Type: Execution Flow | Ability Flow | Input Chain | Tag | Widget
Entry Point: <start_node_or_event>

--- Execution Chain ---
1. [Event] BeginPlay
   → 2. [Function] InitializeAbilities()
      → 3. [Function] GrantAbility(GA_Attack)
         → 4. [BlueprintCall] SetupInputBindings
            → 5. [InputAction] IA_Attack → Ability.Attack tag

--- Key Nodes ---
| Step | Type | Name | Details |
|------|------|------|---------|
| 1 | Event | BeginPlay | Entry point |
| 2 | Function | InitializeAbilities | Grants 3 abilities |
| 3 | Ability | GA_Attack | Triggered by InputTag |

--- Branch Points ---
- Step 3: Branches to GA_Attack (melee) or GA_RangedAttack (ranged)
- Step 5: Conditional on IsLocallyControlled

--- Observations ---
- Total nodes in chain: N
- Max depth: M
- Cross-BP calls: K
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| Blueprint not found | Name mismatch or not indexed | `ue_trace_execution(operation="list_blueprints")` to find correct name |
| Empty trace result | No execution path from start node | Try different start node or use `trace_execution_flow` without start_node for full graph |
| Trace returns start_node only (no path) | No `end_node` specified; tool returns reachable nodes at depth 1 only | Provide `end_node` for specific path finding (primary fix). `max_depth` increase only helps when paths exist but are truncated at the depth limit |
| Ability flow returns null fields | GAS metadata (GameplayEffects, Tags, Montages) not resolvable from binary asset alone | Use `ue_manage_gameplay(operation="trace_abilities")` for tag-ability bindings; `ue_manage_blueprint(operation="get_structure")` for BP-level detail |
| Ability not found | GAS not configured or name wrong | `ue_manage_gameplay(operation="search_tags")` to find ability bindings |
| Input action not found | Enhanced Input not configured | `ue_trace_execution(operation="get_stats")` to check available input actions |
| Tag not found | Tag name typo | `ue_trace_execution(operation="list_tags")` to list available tags |
| Widget not found | Widget Blueprint name mismatch | `ue_trace_execution(operation="list_blueprints")` and filter by WBP_ prefix |

---

## Activation Test Cases

**Positive (5)** - Should activate:
1. "BP_Player 실행 추적해줘" -> execution flow
2. "GA_Weapon_Fire 어빌리티 플로우" -> ability flow
3. "IA_Jump 입력부터 추적" -> input chain
4. "Ability.Attack 태그 전파 추적" -> tag propagation
5. "What happens when BeginPlay fires?" -> execution flow

**Negative (3)** - Should NOT activate:
1. "BP_Player 구조 설명해" -> ue-explain skill
2. "APawn 변경하면 뭐가 깨져?" -> ue-impact skill
3. "에러 해결해줘" -> ue-debug skill

---

**Status**: v1.1.0
**Related**: Issue #6098
