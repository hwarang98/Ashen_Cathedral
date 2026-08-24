---
name: ue-cvar-explorer
description: "Purpose: Comprehensive CVar/config exploration with engine metadata (ECVF flags, scalability group mapping, change impact prediction). Searches, explains, and traces CVar relationships. Triggers: 'CVar', 'config search', '설정 검색', '그래픽 설정', '렌더링 옵션', '퀄리티 설정', 'scalability', 'quality settings'."
argument-hint: ["CVar name or search pattern (e.g., r.Shadow.MaxResolution, sg.ShadowQuality, r.Shadow*)"]
---

# UE CVar Explorer

**Version**: 1.0.0
**Issue**: #5295
**Purpose**: Deep CVar exploration with ECVF flags, scalability group mapping, config relationships, and change impact prediction.

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Flow

1. **Extract target** CVar name or search pattern from user query
2. **Determine query type** and execute appropriate MCP call
3. **Format structured report** from the response

---

## Auto-Trigger Phrases

### Korean
- "CVar 찾아줘", "CVar X 뭐야", "설정 검색"
- "그래픽 설정 분석", "렌더링 옵션 설명"
- "퀄리티 설정 영향 분석", "스케일러빌리티 그룹"
- "r.Shadow 관련 설정 전부 보여줘"
- "성능 설정 탐색", "설정값 변경 영향"

### English
- "Explore CVar X", "What is r.Shadow.MaxResolution?"
- "CVar search Shadow", "Find config settings"
- "Graphics quality settings", "Rendering options"
- "config search", "graphics settings"
- "Scalability group mapping", "Quality level values"
- "What happens if I change r.Shadow.MaxResolution?"

---

## Workflow

### Step 1: Detect Query Type

| Input Pattern | Query Type | MCP Operation |
|--------------|-----------|---------------|
| Specific CVar name (e.g., `r.Shadow.MaxResolution`) | Single CVar exploration | `explore_cvar` |
| Scalability group (e.g., `sg.ShadowQuality`) | Group exploration | `explore_cvar` (shows all controlled CVars) |
| Wildcard pattern (e.g., `r.Shadow*`) | Search first | `search_config` then `explore_cvar` on top results |
| Natural language (e.g., "shadow quality") | Keyword search | `search_config` with extracted keywords |

### Step 2: Execute MCP Tools

**For specific CVar or scalability group:**
```python
result = ue_analyze_config(
    operation="explore_cvar",
    option="r.Shadow.MaxResolution",
    explore=true
)
```

**For pattern search:**
```python
# Step A: Search
search = ue_analyze_config(operation="search_config", query="r.Shadow*")

# Step B: Explore top results
for cvar in search.results[:5]:
    detail = ue_analyze_config(
        operation="explore_cvar",
        option=cvar.key,
        explore=true
    )
```

### Step 3: Format Report

## Output Format

Present results in structured format:

```text
=== CVar Explorer: r.Shadow.MaxResolution ===

--- Identity ---
Name:     r.Shadow.MaxResolution
Type:     TAutoConsoleVariable<int32>
Default:  2048
Flags:    [RenderThreadSafe, Scalability]
Category: Rendering

--- Current Project Values ---
File                     Section                Value
DefaultEngine.ini        [ConsoleVariables]     2048

--- Engine Source ---
File: Source/Runtime/Renderer/Private/ShadowSetup.cpp:42
Help: "Max square dimensions (in texels) for shadow depth rendering"

--- Scalability Group ---
Controlled by: sg.ShadowQuality
  Low (0):         512
  Medium (1):      1024
  High (2):        1024
  Epic (3):        2048
  Cinematic (Cine): 4096

--- Change Impact ---
WARNING: Direct modification will be overridden when sg.ShadowQuality
preset changes.
Recommendation: Modify sg.ShadowQuality instead.
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| CVar not in registry | Engine path not set or CVar unknown | Show config file occurrences only |
| No scalability mapping | CVar not in any sg.* group | Report "Not controlled by scalability preset" |
| explore_cvar unavailable | Older MCP server version | Fall back to `explain_config` + `find_overrides` separately |
| No config files found | Project path incorrect | Suggest checking project_root configuration |

---

## Examples

### Example 1: Specific CVar
**User**: "r.Shadow.MaxResolution 뭐야?"
**Action**: `ue_analyze_config(operation="explore_cvar", option="r.Shadow.MaxResolution", explore=true)`

### Example 2: Scalability Group
**User**: "sg.ShadowQuality가 제어하는 설정들?"
**Action**: `ue_analyze_config(operation="explore_cvar", option="sg.ShadowQuality", explore=true)`

### Example 3: Pattern Search
**User**: "Shadow 관련 CVar 전부"
**Action**: `ue_analyze_config(operation="search_config", query="r.Shadow*")`
Then explore top results individually.

### Example 4: Impact Analysis
**User**: "r.ScreenPercentage 변경하면 영향?"
**Action**: `ue_analyze_config(operation="explore_cvar", option="r.ScreenPercentage", explore=true)`
Focus on `change_impact` and `relationships` in the response.
