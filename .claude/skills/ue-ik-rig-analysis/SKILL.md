---
name: ue-ik-rig-analysis
description: "Purpose: IK Rig structure analysis and comparison. Answers 'Analyze IK Rig X' by inspecting solvers, goals, chains, and retargeter configurations. Supports A/B rig comparison and complexity audit. Triggers: 'IK Rig 분석', 'IK 리그 구조', 'IK 솔버 분석', 'Analyze IK Rig', 'IK Rig comparison', 'retargeter analysis'."
argument-hint: ["IK Rig name (e.g., IK_Mannequin)", "operation: analyze/compare/audit"]
---

# UE IK Rig Analysis

**Version**: 1.0.0
**Issue**: #6047
**Purpose**: Analyze IK Rig structure, solvers, goals, chains, and retargeter configurations with A/B comparison support

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Extract target** IK Rig name(s) from user query or argument
2. **Detect operation mode** (single analysis, comparison, or audit) from context
3. **Execute MCP tools** via `ue_manage_rigging` with appropriate operations
4. **Generate structured report** using the output format below

---

## Auto-Trigger Phrases

### Korean
- "IK 리그 분석해줘", "IK Rig 구조 보여줘"
- "IK 솔버 분석", "IK 골 확인해줘"
- "IK 리그 비교해줘", "리타게터 분석"
- "IK 체인 구조", "IK 리그 감사"
- "IK 리그 복잡도 분석"

### English
- "Analyze IK Rig X", "Show IK Rig structure"
- "IK Rig solvers", "IK Rig goals"
- "Compare IK Rigs", "Retargeter analysis"
- "IK chain structure", "IK Rig audit"
- "IK Rig complexity analysis"

**Keywords**: `IK Rig`, `IK_`, `Retargeter`, `IK solver`, `IK goal`, `IK chain`, `IK 리그`, `솔버`, `리타게터`, `IK 체인`

---

## Mode Detection

```python
MODE_DETECTION = {
    "analyze":  ["analyze", "structure", "inspect", "분석", "구조", "확인"],
    "compare":  ["compare", "vs", "diff", "비교", "차이"],
    "audit":    ["audit", "batch", "all", "감사", "전체", "복잡도"],
}
# Default: "analyze" if no mode keyword detected
```

---

## Workflow

### Step 1: Discover IK Rigs

If the user provides a specific rig name, skip to Step 2. Otherwise, search for available rigs.

```python
ToolSearch("select:mcp__narshamcp__ue_manage_rigging")

# Smart mode: auto-routes based on params
ue_manage_rigging(operation="smart", query="*")  # → search_rigs

# Or explicit search
ue_manage_rigging(operation="search_rigs", params={"query": "*"})
```

**Extract from response**: List of IK Rig names and paths for subsequent analysis.

### Step 2: Get Rig Structure (Single Analysis)

For a named IK Rig, retrieve the full structure overview.

```python
# Smart mode: auto-detects get_structure from rig_name
ue_manage_rigging(operation="smart", rig_name="IK_Mannequin")  # → get_structure

# Or explicit
ue_manage_rigging(operation="get_structure", params={"rig_name": "IK_Mannequin"})
```

**Extract from response**: Skeleton reference, solver count, goal count, chain count, overall complexity.

### Step 3: Deep-Dive - Solvers, Goals, Chains

Retrieve detailed breakdown of the three core IK Rig components.

```python
# Get solver configurations (FABRIK, LimbIK, CCDIK, etc.)
ue_manage_rigging(operation="get_solvers", params={"rig_name": "IK_Mannequin"})

# Get goal definitions (effectors, root bones, target bones)
ue_manage_rigging(operation="get_goals", params={"rig_name": "IK_Mannequin"})

# Get chain definitions (bone chains for IK solving)
ue_manage_rigging(operation="get_chains", params={"rig_name": "IK_Mannequin"})
```

**Extract from response**:
- **Solvers**: Solver type, target bone, settings (iterations, precision, etc.)
- **Goals**: Goal name, bone association, transform targets
- **Chains**: Chain name, start bone, end bone, bone count

### Step 4: Retargeter Configuration (Optional)

If retargeting is relevant or the user asks about retargeters, inspect the retargeter asset.

```python
# Get retargeter configuration (source/target IK Rigs, chain mappings)
ue_manage_rigging(operation="get_retargeter", params={"rig_name": "RTG_Mannequin"})
```

**Extract from response**: Source/target IK Rig references, chain mapping pairs, pose offsets.

### Step 5: Complexity Audit (Audit Mode)

For batch analysis across all project IK Rigs.

```python
def audit_ik_rigs(pattern="*"):
    # Stage 1: Find all IK Rigs
    rigs = ue_manage_rigging(operation="search_rigs", params={"query": pattern})

    audit_results = []
    for rig in rigs.get("results", []):
        # Stage 2: Analyze each rig for complexity metrics
        analysis = ue_manage_rigging(
            operation="analyze_rig", params={"rig_name": rig["name"]}
        )
        audit_results.append({
            "name": rig["name"],
            "solver_count": analysis.get("solver_count", 0),
            "goal_count": analysis.get("goal_count", 0),
            "chain_count": analysis.get("chain_count", 0),
            "complexity": analysis.get("complexity", "Unknown"),
        })

    # Sort by total component count descending
    audit_results.sort(
        key=lambda x: x["solver_count"] + x["goal_count"] + x["chain_count"],
        reverse=True
    )
    return audit_results
```

### Step 6: A/B Rig Comparison (Compare Mode)

Compare two IK Rigs side-by-side.

```python
def compare_ik_rigs(rig_a, rig_b):
    # Single-call comparison
    comparison = ue_manage_rigging(
        operation="compare_rigs",
        params={"rig_a": rig_a, "rig_b": rig_b}
    )

    # Optionally get detailed structures for deeper analysis
    struct_a = ue_manage_rigging(operation="get_structure", params={"rig_name": rig_a})
    struct_b = ue_manage_rigging(operation="get_structure", params={"rig_name": rig_b})

    return {
        "comparison": comparison,
        "structure_a": struct_a,
        "structure_b": struct_b,
    }
```

---

## Output Format

Present results in this structured format:

```
=== [Rig Name] -- IK Rig Analysis ===

--- Overview ---
[1-2 sentence summary: skeleton, purpose, complexity level]

--- Solvers ---
| # | Solver Type | Target Bone | Iterations | Precision |
|---|------------|-------------|------------|-----------|
| 1 | FABRIK     | hand_r      | 15         | 0.01      |
| 2 | LimbIK    | foot_l      | 10         | 0.001     |

--- Goals ---
| # | Goal Name   | Bone        | Connected Solver |
|---|------------|-------------|------------------|
| 1 | RightHand  | hand_r      | FABRIK_1         |
| 2 | LeftFoot   | foot_l      | LimbIK_1         |

--- Chains ---
| # | Chain Name | Start Bone | End Bone | Bone Count |
|---|-----------|------------|----------|------------|
| 1 | RightArm  | clavicle_r | hand_r   | 4          |
| 2 | LeftLeg   | thigh_l    | foot_l   | 3          |

--- Retargeter (if applicable) ---
Source Rig: IK_Mannequin
Target Rig: IK_CustomCharacter
Chain Mappings: RightArm -> RightArm, LeftLeg -> LeftLeg

--- Complexity Summary ---
Solvers: 4 | Goals: 6 | Chains: 8 | Complexity: Medium
```

### Comparison Output Format

```
=== IK Rig Comparison: [Rig A] vs [Rig B] ===

            | Rig A       | Rig B       | Diff
Solvers     |     3       |     5       |  +2
Goals       |     4       |     6       |  +2
Chains      |     6       |     8       |  +2
Complexity  |   Low       |   Medium    |  -

Solver Types:
  Shared: FABRIK, LimbIK
  Only in A: -
  Only in B: CCDIK, SplineIK

Chain Differences:
  Only in A: Spine
  Only in B: Tail, Spine_Extended
```

### Audit Output Format

```
=== IK Rig Complexity Audit ===

Total: N rigs analyzed
High Complexity: X rigs
Medium: Y rigs
Low: Z rigs

Top Optimization Targets:
1. IK_FullBody (5 solvers, 12 goals, 10 chains) - HIGH
2. IK_Mannequin (3 solvers, 6 goals, 8 chains) - MEDIUM
3. IK_Simple (1 solver, 2 goals, 2 chains) - LOW
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| IK Rig not found | Wrong name or asset not in project | Run `ue_manage_rigging(operation="search_rigs", params={"query": "*"})` to list all available rigs, then retry with correct name |
| Retargeter not found | No retargeter asset for this rig | Report "No retargeter configured for this IK Rig" and skip retargeter step; suggest creating one if needed |
| Metadata not cached | First query after cache clear | Run `ue_search_assets(asset_type="ik_rig")` to populate metadata cache, then retry |
| MCP server not connected | Server process not running | Report: "Run `ue_check_health()` to verify MCP connection" |
| Comparison fails (missing rig) | One of the two rigs does not exist | Search for both rigs first with `search_rigs`, report which one is missing, suggest alternatives |

---

## Evaluation Criteria

### Activation Test Cases

**Positive (5)** - Should activate:
1. "IK_Mannequin 분석해줘" -> Activate (single analysis)
2. "Analyze IK Rig structure" -> Activate (single analysis)
3. "IK_Hand vs IK_Foot 비교해줘" -> Activate (comparison)
4. "프로젝트 IK 리그 전체 감사" -> Activate (audit)
5. "IK 솔버랑 골 확인해줘" -> Activate (deep-dive)

**Negative (3)** - Should NOT activate:
1. "IK_Mannequin 검색해줘" -> Use `ue_manage_rigging(search_rigs)` directly
2. "Control Rig 분석해줘" -> Use `control-rig-workflow` skill (different domain)
3. "캐릭터 리타게팅 설정해줘" -> Use `ue_manage_rigging` directly for write operations

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| Domain detection accuracy | >95% |
| MCP tool selection accuracy | 100% (hardcoded to ue_manage_rigging) |
| Response time (single rig) | <15s |
| Response completeness | All applicable output sections populated |
| Audit batch time | <2 min for 20 rigs |

---

**Status**: Phase 1 MVP
**Related**: Issue #6047
**Last Updated**: 2026-03-07
