---
name: blueprint-flow
description: "Purpose: Blueprint execution flow. Structure analysis + execution tracing with dual methods (step-by-step or workflow shortcut 57% faster). Input→Animation, BP structure, ability flow. For GAS/InputTag deep chain (5-stage), use cross-domain-flow. Not for creating new BPs (use blueprint-generator). Triggers: 'BP 실행 흐름', 'BP 분석', 'animation trace', '버튼 누르면', 'Blueprint 구조'."
---

# Blueprint Flow Workflow

**Version**: 2.0.0 (Merged from Issue #1715 Phase 2 + Issue #262 Phase 1 POC)
**Priority**: High
**Goal**: Automate multi-tool workflow for Blueprint analysis with optimized execution paths

---

## Purpose

Orchestrates Blueprint analysis for **complete flow understanding**:

1. **Structure Analysis** - Get Blueprint graph structure (entry points, nodes, connections)
2. **Execution Tracing** - Trace execution from input to output
3. **Result Integration** - Merge structure and flow data with Mermaid visualization

**Key Innovation**: Hybrid approach with two execution methods:
- **Method A** (Step-by-step): Manual MCP calls for maximum flexibility (4.2s, 8K tokens)
- **Method B** (Workflow Shortcut): `ue_run_workflow` atomic execution for speed (1.8s, 3K tokens) — **Recommended**

**Performance** (Method B vs A):
- 57% faster (4.2s → 1.8s)
- 62% token reduction (8K → 3K)
- 75% fewer Claude decisions (4 → 1)
- Same 95% accuracy

---

## Trigger Phrases

### Korean

- "Blueprint 구조 분석", "BP 분석"
- "실행 흐름 추적", "실행 흐름 보여줘"
- "BP 내부 흐름", "Blueprint 노드 실행"
- "Blueprint 전체 분석", "BP 실행 흐름"
- "BP 그래프에서 뭐가 실행돼?"
- "E키 누르면 어떤 로직 트리거돼?"

**설정/파라미터 찾기 (Issue #3876 - 일반화)**:
- "어디서 설정", "어떻게 세팅", "설정값 찾기", "파라미터 찾기"
- "값 변경하고 싶", "설정 위치", "어디서 바꾸니"

**플로우/로직 이해**:
- "전체 순서", "로직 흐름", "어떻게 작동", "실행 흐름"
- "각 단계마다", "프로세스 흐름", "시퀀스"

**구조/구현 탐색**:
- "어디서 구현", "구조 파악", "연결 관계", "의존성"
- "어빌리티 추적", "어빌리티 흐름", "GA_ 트리거"

### English

- "Blueprint structure analysis", "BP analysis"
- "execution flow trace", "show execution flow"
- "BP internal flow", "Blueprint node execution"
- "full Blueprint analysis", "BP execution flow"
- "What runs in this BP graph?"
- "What does E key trigger?"

**Settings/Parameter discovery (Issue #3876 - generalized)**:
- "where to set", "how to configure", "find setting", "find parameter"
- "want to change value", "setting location", "where to modify"

**Flow/Logic understanding**:
- "full sequence", "logic flow", "how does it work", "execution flow"
- "each step", "process flow", "sequence"

**Structure/Implementation exploration**:
- "where implemented", "understand structure", "dependencies"
- "ability trace", "ability flow", "GA_ trigger"

**Keywords**: `BP flow`, `BP trace`, `BP execution`, `blueprint`, `BP 노드`, `BP 이벤트`, `GA_ trigger`, `BP 내부 흐름`, `Blueprint 구조`

---

## Workflow Overview

### 3-Phase Analysis Process

```text
Phase 1: Structure Analysis
│ Tool: ue_analyze_blueprint(operation="get_structure")
│ Output: Graph nodes, connections, entry points
↓
Phase 2: Execution Tracing (Hybrid Method)
│ Method A: Individual MCP calls (4.2s) — for debugging/learning
│ Method B: ue_run_workflow shortcut (1.8s) — recommended default ⭐
│ Output: Execution path, function calls, animations
↓
Phase 3: Result Integration
│ Merge structure + trace data
│ Generate Mermaid visualization
│ Identify key execution paths
```

---

## Phase 2: Method Selection

### Method B: Workflow Shortcut (Default) ⭐

**Use when**: Standard flow tracing (Input → Animation), fast results needed

```python
result = ue_run_workflow(
    workflow="input-to-action",
    target="LeftMouseButton"
)
# → Complete flow in 1.8s, 3K tokens
```

### Method A: Step-by-step (Baseline)

**Use when**: Cross-domain (C++↔BP), delegate tracking, step-by-step learning, debugging

**4 Separate MCP Calls**:

```python
# Step 1: Input Detection
result_1 = ue_analyze_blueprint(operation="trace_from_input", params={"key": "LeftMouseButton"})

# Step 2: Find Input Events
result_2 = ue_analyze_blueprint(operation="find_input_events", params={"action_name": "IA_Attack"})

# Step 3: Execution Flow (with optional visualization)
result_3 = ue_trace_execution(operation="trace_execution_flow", params={
    "blueprint_name": "BP_PlayerCharacter",
    "start_node": "K2Node_InputAction_123",
    "max_depth": 10,
    "output_format": "html"  # Options: "json" (default), "html" (interactive), "mermaid" (diagram)
})
# output_format="html" generates interactive HTML for browser-based debugging
# output_format="mermaid" generates Mermaid diagram for inline visualization

# Step 4: Animation Search (if mentioned in flow)
result_4 = ue_analyze_blueprint(operation="search_animations", params={"query": "Attack_Combo1"})
```

### Decision Algorithm

```python
def choose_method(user_request, flow_type):
    # Priority 1: Non-standard flows require Method A
    if flow_type in ["cross_domain", "delegate", "realtime", "widget"]:
        return "Method A"

    # Priority 2: User wants step-by-step
    if "step-by-step" in user_request or "단계별" in user_request:
        return "Method A"

    # Priority 3: Default to workflow for speed
    return "Method B ⭐"
```

---

## Tool Integration

### Primary Tools Used

| Tool | Operation | Purpose |
|------|-----------|---------|
| `ue_analyze_blueprint` | `get_structure` | Get graph structure (Phase 1) |
| `ue_analyze_blueprint` | `analyze_execution_depth` | Calculate complexity |
| `ue_trace_execution` | `trace_execution_flow` | Trace from entry point |
| `ue_trace_execution` | `trace_ability_flow` | GAS ability tracing |
| `ue_run_workflow` | `input-to-action` | Atomic flow trace (Method B) ⭐ |

### Routing Logic

```python
BLUEPRINT_ROUTING = {
    "structure": ("ue_analyze_blueprint", "get_structure"),
    "depth": ("ue_analyze_blueprint", "analyze_execution_depth"),
    "flow": ("ue_trace_execution", "trace_execution_flow"),
    "ability": ("ue_trace_execution", "trace_ability_flow"),
    "input": ("ue_trace_execution", "trace_from_input"),
}

# Issue #3876: GAS/Ability queries should use trace_abilities FIRST
# This returns InputTag chain for Lyra-style projects
GAS_ROUTING = {
    "ability_activation": ("ue_manage_gameplay", "trace_abilities"),  # FIRST CHOICE
    "ability_trigger": ("ue_manage_gameplay", "trace_abilities"),     # How is GA_X triggered?
    "fire_rate": ("ue_manage_gameplay", "trace_abilities"),           # 총을 쏘는 간격 → trace GA
    "cooldown": ("ue_manage_gameplay", "trace_abilities"),            # 쿨다운 시간 → trace GA
    "tag_search": ("ue_manage_gameplay", "search_tags"),              # What abilities use tag X?
}
```

---

## Quick Example

**User**: "BP_ShooterCharacter의 실행 흐름 분석해줘"

**Phase 1: Structure Analysis**
```python
ue_analyze_blueprint(operation="get_structure", params={"blueprint_name": "BP_ShooterCharacter"})
# → Entry Points: [InputAction IA_Attack, BeginPlay, Tick]
# → Key Nodes: 15 function calls, 3 custom events
```

**Phase 2: Execution Tracing (Method B)** ⭐
```python
result = ue_run_workflow("input-to-action", "LeftMouseButton")
# → Complete flow in 1.8s
```

**Phase 3: Result Integration**
```text
🎯 Complete Execution Flow: LeftMouseButton

📍 Input Mapping
   LeftMouseButton → IA_Attack (IMC_Default)

🔗 Event Binding
   IA_Attack → BP_PlayerCharacter::InputAction_IA_Attack

⚙️ Blueprint Logic (7 steps)
   1. InputAction_IA_Attack (Event)
   2. Branch → IsAttackReady
   3. ActivateAbilityByClass → GA_Attack_Melee
   4. PlayMontage → Attack_Combo1
   5. SendGameplayEvent → Event.Attack.Start
   6. SetTimerByEvent → ComboWindow (0.5s)
   7. PrintString → "Attack Started"

🎬 Animation
   Attack_Combo1 (Length: 1.2s)
   - Frame 15-20: ANS_AttackHit (Apply Damage)
   - Frame 10: AN_FootstepSound (Play Audio)
```

**Total time**: ~2.1s (vs 4.5s with Method A = 53% faster)

---

## Performance Comparison

| Metric | Method A (Baseline) | Method B (Workflow) | Improvement |
|--------|---------------------|---------------------|-------------|
| **Time** | 4.2s | 1.8s | **57% faster** |
| **Tokens** | 8K | 3K | **62% reduction** |
| **MCP Calls** | 4 | 1 | **75% fewer** |
| **Accuracy** | 95% | 95% | Same |

---

## Common Failures

| 에러 유형 | 원인 | 해결 방법 |
|-----------|------|-----------|
| `get_structure` 빈 결과 | Blueprint 이름 오타 또는 에셋 미로드 | `ue_search_assets`로 정확한 Blueprint 이름 확인 후 재시도 |
| `trace_execution_flow` 실패 | Phase 1에서 entry point를 찾지 못함 | Blueprint에 유효한 EventGraph 또는 InputAction이 있는지 확인 |
| GAS 라우팅 오류 | Ability 쿼리를 `ue_search_assets` 대신 `ue_manage_gameplay`로 보내지 않음 | Issue #3876 라우팅 로직 참조, `trace_abilities` 사용 |
| Phase 간 컨텍스트 유실 | Phase 1 결과가 Phase 2로 전달되지 않음 | 각 Phase 출력을 명시적으로 다음 Phase 입력에 전달 |
| 분석 타임아웃 | 대형 Blueprint (500+ 노드) 처리 시간 초과 | `--max-depth` 제한 또는 서브그래프 단위 분석 |
| `ue_run_workflow` 실패 | 등록되지 않은 workflow 이름 사용 | `input-to-action` 등 정확한 workflow 이름 확인 |
| Method B 타임아웃 | 복잡한 cross-domain 플로우 | Method A (step-by-step)로 전환하여 단계별 디버깅 |
| 빈 Ability 플로우 | `trace_ability_flow`에 잘못된 GA_ 클래스명 전달 | `ue_manage_gameplay(search_tags)`로 유효한 Ability 목록 먼저 조회 |

---

## Related Files

- **EXAMPLES.md** - Complete usage examples (Method A vs B comparison, 5+ scenarios)
- **ADVANCED.md** - Cross-domain tracing, delegate tracking, real-time updates, widget events
- **REFERENCE.md** - Complete stage details, Mermaid templates, tool parameters, performance metrics

**Issues**:
- Issue #1715 - Blueprint Flow Workflow (Phase 2)
- Issue #262 - Workflow-Skill integration POC
- Issue #3876 - GAS Tool Selection
- Issue #107 - Delegate tracking

---

## Output Format

```
Blueprint Execution Flow: <Blueprint name>
──────────────────────────────────────────
Method: A (Step-by-step) | B (Workflow Shortcut)
Time: <seconds>

[Input Mapping]
  <Key> → <InputAction> (<IMC>)

[Event Binding]
  <InputAction> → <Blueprint>::<Event>

[Execution Steps] (<N> steps)
  1. <Node> (Event)
  2. <Node> → <condition>
  3. <Node> → <action>
  ...

[Animation] (if applicable)
  <Montage> (Length: <duration>)
  - <Notify> at <time>

Status: COMPLETE | PARTIAL | FAILED
```

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| `get_structure` empty result | Blueprint name typo or asset not loaded | Use `ue_search_assets(asset_type="blueprint")` to find correct name |
| `trace_execution_flow` fails | No valid entry point found in Blueprint | Verify Blueprint has EventGraph or InputAction; try `get_structure` for static analysis |
| `ue_run_workflow` timeout | Complex cross-domain flow exceeds time limit | Switch to Method A (step-by-step) for incremental debugging |
| GAS routing error | Ability query sent to wrong tool | Use `ue_manage_gameplay(trace_abilities)` for InputTag chain resolution per Issue #3876 |

**Status**: Production (Merged v1.0.0 + v2.0.0)
**Last Updated**: 2026-02-13
