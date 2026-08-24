---
name: ue-explain
description: "Purpose: Universal UE system explainer with auto-domain routing. Answers 'How does X work?' for any UE entity (C++ class, Blueprint, Material, Niagara, GAS, Config, AI, PCG, Sequencer). Auto-detects target type from name prefix and routes to optimal MCP tools. Triggers: 'explain', 'how does', 'what is', '설명해', '어떻게 동작', '구조 분석', 'show me how', 'tell me about'."
argument-hint: ["TargetName (e.g., ACharacter, BP_Player, M_Rock, NS_Fire, GA_Attack, r.Shadow.MaxResolution)"]
---

# UE Explain — Universal System Explainer

**Version**: 1.0.0
**Issue**: #4800
**Purpose**: Answer "How does X work?" for **any** UE entity with auto-domain routing

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Extract target** from user query or argument
2. **Auto-detect domain** using the routing matrix below
3. **Execute MCP tools** for that domain
4. **Generate structured explanation** using the output format

---

## Auto-Trigger Phrases

### Korean
- "X가 어떻게 동작해?", "X 설명해줘"
- "X 구조 분석해줘", "X가 뭐야?"
- "X 내부 구조 보여줘", "X 동작 원리"

### English
- "How does X work?", "Explain X"
- "What is X?", "Tell me about X"
- "Show me the structure of X", "How X works"

---

## Domain Auto-Routing Matrix

Detect target type from name patterns and route to the correct MCP tools.

### Step 1: Identify Target Type

```python
ROUTING_RULES = {
    # Prefix-based detection (ordered by specificity)
    "BP_":   "blueprint",    # BP_PlayerController, BP_Enemy
    "GA_":   "ability",      # GA_Attack, GA_Jump
    "GE_":   "ability",      # GE_ApplyDamage (GameplayEffect)
    "NS_":   "niagara",      # NS_Fire, NS_Explosion
    "M_":    "material",     # M_Rock, M_Water
    "MI_":   "material",     # MI_RockInstance (Material Instance)
    "ST_":   "ai",           # ST_EnemyAI (StateTree)
    "BT_":   "ai",           # BT_EnemyAI (BehaviorTree)
    "PCG_":  "pcg",          # PCG_Forest
    "LS_":   "sequencer",    # LS_IntroCinematic
    "DT_":   "datatable",    # DT_WeaponStats (DataTable)
    "WBP_":  "blueprint",    # WBP_MainMenu (Widget Blueprint)
    "ABP_":  "blueprint",    # ABP_Character (AnimBlueprint)

    # Pattern-based detection
    "r.":    "config",       # r.Shadow.MaxResolution (CVar)
    "gc.":   "config",       # gc.MaxObjectsNotConsideredByGC
    "net.":  "config",       # net.MaxClientRate
    "fx.":   "config",       # fx.Niagara.* config

    # UE C++ class prefixes (single letter + uppercase)
    "A":     "cpp_class",    # ACharacter, AActor, APawn
    "U":     "cpp_class",    # UObject, UGameplayAbility
    "F":     "cpp_class",    # FVector, FName, FHitResult
    "E":     "cpp_class",    # ECollisionChannel (enum)
    "I":     "cpp_class",    # IAbilitySystemInterface (interface)
}

# Fallback: if no prefix matches, use ue_search_assets to discover type
```

### Step 2: Route to MCP Tools

| Domain | Primary Tool | Operations | Secondary Tool |
|--------|-------------|------------|----------------|
| `cpp_class` | `ue_analyze_symbols` | `search_symbols` + `get_methods` + `trace_hierarchy` + `find_virtual_overrides` | — |
| `blueprint` | `ue_manage_blueprint` | `get_structure` | `ue_trace_execution(trace_execution_flow)` |
| `material` | `ue_manage_material` | `get_nodes` + `get_hierarchy` + `analyze_performance` | — |
| `niagara` | `ue_manage_niagara` | `get_structure` + `get_parameters` | — |
| `ability` | `ue_manage_gameplay` | `trace_abilities` | `ue_trace_execution(trace_ability_flow)` |
| `config` | `ue_analyze_config` | `search_config` + `explain_config` | — |
| `ai` | `ue_manage_ai` | `get_structure` + `find_transitions` | — |
| `pcg` | `ue_manage_pcg` | `get_structure` + `trace_flow` | — |
| `sequencer` | `ue_sequencer_structure` | `get_structure` + `list_tracks` | `ue_sequencer_tracks(list_tracks)` |
| `datatable` | `ue_search_assets` | search by name | `ue_manage_blueprint(get_structure)` |
| `fallback` | `ue_search_assets` | broad search | Route to specific domain after discovery |

### Step 3: Load and Call MCP Tools

Use `ToolSearch` to load the required MCP tool, then call it:

```python
# Example: C++ class
ToolSearch("select:mcp__narshamcp__ue_analyze_symbols")

# 1. Search for the symbol
ue_analyze_symbols(operation="search_symbols", params={"query": "ACharacter", "symbol_type": "class"})

# 2. Get methods/members
ue_analyze_symbols(operation="get_methods", params={"class_name": "ACharacter"})

# 3. Get inheritance hierarchy
ue_analyze_symbols(operation="trace_hierarchy", params={"class_name": "ACharacter"})

# 4. Get virtual method overrides (shows which subclasses override methods)
ue_analyze_symbols(operation="find_virtual_overrides", params={"class_name": "ACharacter"})
```

```python
# Example: Blueprint
ToolSearch("select:mcp__narshamcp__ue_manage_blueprint")

# 1. Get structure (returns node count and hint)
result = ue_manage_blueprint(operation="get_structure", params={"blueprint_name": "BP_PlayerController"})

# 2. If get_structure returns execution graph hint, follow up with get_raw_nodes for details
#    (get_structure often returns just node_count — use get_raw_nodes for actual node data)
ue_manage_blueprint(operation="get_raw_nodes", params={"blueprint_name": "BP_PlayerController", "limit": 20})

# 3. For quick variable inspection (alternative to full structure)
ue_manage_blueprint(operation="search_variables", params={"blueprint_name": "BP_PlayerController", "pattern": "*"})

# 4. Optionally trace execution (with Mermaid visualization)
ToolSearch("select:mcp__narshamcp__ue_trace_execution")
ue_trace_execution(operation="trace_execution_flow", params={
    "blueprint_name": "BP_PlayerController",
    "output_format": "mermaid"  # Options: "json" (default), "html" (interactive), "mermaid" (diagram)
})
```

```python
# Example: Config
ToolSearch("select:mcp__narshamcp__ue_analyze_config")

# Note: search_config rejects wildcards ("*"). Use exact key or partial substring.
ue_analyze_config(operation="search_config", params={"key": "r.Shadow.MaxResolution"})
```

---

## Output Format

Present results in this structured format:

```text
=== [Target Name] — [Type] Explained ===

--- Overview ---
[1-2 sentence summary of what this entity is and its primary purpose]

--- Structure ---
[Tree view appropriate to the domain]

For C++ class:
  ACharacter
  +-- Parent: APawn -> AActor -> UObject
  +-- Components: UCharacterMovementComponent, UCapsuleComponent
  +-- Key Methods: Jump(), Crouch(), OnLanded()

For Blueprint:
  BP_PlayerController
  +-- Parent Class: APlayerController
  +-- Variables: Health (float), MaxHealth (float)
  +-- Functions: InitializePlayer, HandleDamage
  +-- Event Graph: 12 nodes

For Material:
  M_Rock
  +-- Parent: M_BaseMaterial
  +-- Parameters: BaseColor, Roughness, Normal
  +-- Nodes: 8 expression nodes

--- How It Works ---
[Step-by-step execution flow or data flow specific to the domain]

--- Key Files ---
[Source file paths with line numbers, or asset paths]
- Source/MyGame/Characters/MyCharacter.h:42
- /Game/Blueprints/BP_PlayerController.uasset

--- Related ---
[Parent class, child classes, related assets, dependencies]
- Parent: APawn
- Children: AMyCharacter, AEnemyCharacter
- Used by: BP_Player, BP_Enemy
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| Symbol not found | PDB not indexed or typo | Try `ue_search_assets` with fuzzy name |
| Blueprint not found | Wrong name or not in project | `ue_search_assets(asset_type="blueprint", pattern="*PartialName*")` |
| Config key not found | Incorrect CVar name | `ue_analyze_config(operation="search_config", params={"key": "Shadow"})` with partial match |
| MCP tool unavailable | Server not connected | Report: "Run `ue_check_health()` to verify MCP connection" |

---

## Evaluation Criteria

### Activation Test Cases

**Positive (5)** - Should activate:
1. "ACharacter 어떻게 동작해?" -> Activate (C++ class)
2. "Explain BP_PlayerController" -> Activate (Blueprint)
3. "M_BasicRock 설명해줘" -> Activate (Material)
4. "How does NS_Fire work?" -> Activate (Niagara)
5. "r.Shadow.MaxResolution 뭐야?" -> Activate (Config)

**Negative (3)** - Should NOT activate:
1. "BP_Player의 실행 흐름 추적해줘" -> Use blueprint-flow skill
2. "GA_Attack 전체 흐름" -> Use cross-domain-flow skill
3. "M_Rock 성능 최적화해줘" -> Use material-analysis skill

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| Domain detection accuracy | >95% |
| MCP tool selection accuracy | 100% (hardcoded routing) |
| Response time (single domain) | <30s |
| Response completeness | All 5 output sections populated |

---

**Status**: Phase 1 MVP
**Related**: Issue #4800 (MCP-First Tool Selection)
