# Asset Modification Wizard - Complete Reference

Complete workflow details, tool parameters, and safety guidelines.

---

## Complete Workflow Details

### Stage 1: Target Discovery (Asset Search)

**Purpose**: Find all assets matching the modification criteria.

**Tool Call**:

```python
ue_search_assets(
    asset_type="blueprint",  # or "pcg", "animation", etc.
    pattern="BP_Enemy*",     # Wildcard pattern
    project_root="<auto-detected>"
)
```

**Parameters**:

- `asset_type` (required): Type of asset (`"blueprint"`, `"pcg"`, `"animation"`, `"statetree"`, `"behaviortree"`)
- `pattern` (required): Search pattern (supports wildcards `*`)
- `project_root` (optional): Project root path (auto-detected)

**Expected Output**:

```json
{
    "results": [
        {
            "name": "BP_Enemy_Melee",
            "path": "/Game/Blueprints/Enemies/BP_Enemy_Melee.uasset",
            "size_mb": 2.3,
            "last_modified": "2025-10-20T12:34:56Z"
        }
    ],
    "total_count": 3,
    "total_size_mb": 9.3
}
```

**Response Template**:

```text
🔍 Stage 1: Target Discovery

Found **[N] [Type]** matching "[Pattern]":
- [Asset1] ([Size] MB)
- [Asset2] ([Size] MB)
...

Total size: [Total] MB

Proceeding to impact analysis...
```

---

### Stage 2: Impact Analysis (Dependency Check)

**Purpose**: Assess modification impact, identify risks, and check dependencies.

**Tool Call** (Internal):

```python
# Internal function, called automatically
analyze_impact(target_assets)

# Returns:
{
    "direct_callers": 12,         # Blueprints referencing these assets
    "child_blueprints": 5,        # Inherited classes
    "active_in_levels": 3,        # Levels using these assets
    "risk_level": "medium",       # low/medium/high
    "potential_risks": [...]      # List of specific risks
}
```

**Response Template**:

```text
📊 Stage 2: Impact Analysis

Modification Impact:
- Direct callers: [N] Blueprints reference these assets
- Child Blueprints: [M] inherit from these classes
- Risk Level: **[Low/Medium/High]** [✅/⚠️/❌]

Potential Risks:
[List of risks if any]

Dependencies:
[List of required properties/components]

Recommendation: [Safe to proceed / Proceed with caution / High risk]

Proceeding to strategy selection...
```

---

### Stage 3: Strategy Selection (Automatic)

**Purpose**: Automatically select fastest and safest execution method.

**Decision Algorithm**:

```python
def select_strategy():
    if is_editor_running() and has_remote_control_api():
        if modification_requires_editor():
            return "remote_control_api"  # Required
        else:
            return "remote_control_api"  # Faster (50-200x)
    else:
        return "uasset_modification"  # Fallback
```

**Response Template**:

```text
⚡ Stage 3: Strategy Selection

Selected Strategy: [Remote Control API / .uasset Modification] [✅/⚠️]

[If Remote Control API]:
- Editor detected on port [Port]
- Estimated time: **[2-5] seconds** (vs [3-5] minutes for .uasset)
- [50-200x] performance improvement

[If .uasset Modification]:
- Editor not detected
- Estimated time: [3-5] minutes
- Safe fallback method
- ℹ️ Tip: Open Editor for 50-200x speedup

Capabilities:
[List of supported operations]

Proceeding to dry-run preview...
```

---

### Stage 4: Dry-Run Preview (User Approval Required)

**Purpose**: Show exactly what will change before applying modifications.

**Tool Call** (Internal):

```python
# Generate preview (NO actual modifications)
preview = generate_preview(target_assets, modifications)

# Returns:
{
    "changes": [
        {
            "asset": "BP_Enemy_Melee",
            "property": "MaxHealth",
            "old_value": 50,
            "new_value": 100,
            "status": "will_change"
        }
    ],
    "warnings": [...],
    "estimated_time": "2-5 seconds"
}
```

**Response Template**:

```text
🔍 Stage 4: Dry-Run Preview

Proposed Changes (Preview Only - Not Applied Yet):

| [Asset Type] | Property | Old Value | New Value | Status |
|--------------|----------|-----------|-----------|--------|
| [Asset1] | [Prop] | [Old] | [New] | ✅ Will change |
...

⚠️ Warnings:
[List of warnings if any]

Summary:
- Total changes: [N]
- Estimated time: [Time]
- Strategy: [Strategy]

**Do you want to proceed with these changes?**
Options:
- "Yes" / "예" / "Apply" → Proceed to Stage 5 (Execution)
- "No" / "아니오" / "Cancel" → Cancel modification
- "Exclude [AssetName]" → Modify subset
```

**IMPORTANT**: Wait for user confirmation before proceeding to Stage 5.

---

### Stage 5: Execution & Validation (After User Approval)

**Purpose**: Apply modifications and verify success.

**Tool Call**:

```python
ue_modify_assets(
    asset_type="blueprint",
    operation="modify",
    params={
        "assets": target_assets,
        "modifications": changes,
        "strategy": selected_strategy,
        "create_backup": True
    }
)
```

**Parameters**:

- `asset_type` (required): Type of asset
- `operation` (required): `"modify"` or `"rollback"`
- `params` (required): Dictionary with:
  - `assets`: List of asset paths
  - `modifications`: List of changes
  - `strategy`: `"remote_control_api"` or `"uasset_modification"`
  - `create_backup`: `True` (always recommended)

**Response Template**:

```yaml
✅ Stage 5: Execution Complete

Modifications Applied Successfully!

| [Asset Type] | Property | New Value | Status |
|--------------|----------|-----------|--------|
| [Asset1] | [Prop] | [New] | ✅ Success |
...

Execution Summary:
- Strategy: [Strategy]
- Execution time: **[Time]**
- Success rate: **[N]%** ([Success]/[Total])
- Failed: [M]

Validation:
- ✅ All assets verified (integrity check passed)
- ✅ Backup created: [Backup Path]
- ✅ New values verified in assets

[If failures]:
❌ Failed Assets:
[List of failed assets with reasons]

Next Steps:
[Recommendations]
```

---

## Tool Parameter Reference

### `ue_search_assets`

**Purpose**: Find assets matching pattern

**Parameters**:

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `asset_type` | string | Yes | Asset type (`"blueprint"`, `"pcg"`, etc.) |
| `pattern` | string | Yes | Search pattern (supports `*` wildcard) |
| `project_root` | string | No | Project root (auto-detected) |

---

### `ue_modify_assets`

**Purpose**: Modify or rollback assets

**Parameters**:

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `asset_type` | string | Yes | Asset type |
| `operation` | string | Yes | `"modify"` or `"rollback"` |
| `params` | dict | Yes | Operation parameters |

**params dict**:

```python
{
    "assets": ["<asset_path1>", "<asset_path2>", ...],  # List of asset paths
    "modifications": [                                   # List of changes
        {"property": "MaxHealth", "new_value": 100},
        {"property": "MovementSpeed", "new_value": 600.0}
    ],
    "strategy": "remote_control_api",  # or "uasset_modification"
    "create_backup": True              # Always True (recommended)
}
```

---

## Safety Guidelines

### Pre-Modification Safety Checklist

- [ ] **Backup created**: Automatic, always enabled
- [ ] **Impact analyzed**: Understand modification scope
- [ ] **Dry-run preview reviewed**: Know what will change
- [ ] **User approval obtained**: Wait for "Yes" confirmation
- [ ] **Dependencies validated**: Required properties/components exist

---

### Execution Safety Checklist

- [ ] **Atomic operations**: All-or-nothing per asset
- [ ] **Progress tracked**: Monitor bulk operations
- [ ] **Automatic fallback**: Handle Editor disconnect
- [ ] **Integrity verified**: Check asset health after modification

---

### Post-Modification Safety Checklist

- [ ] **Validation passed**: New values verified in assets
- [ ] **Backup available**: Can rollback if needed
- [ ] **Modification history saved**: Track what changed
- [ ] **Testing recommended**: Verify gameplay impact

---

## Rollback Support

### Rollback Command

**Purpose**: Restore assets from backup

**Tool Call**:

```python
ue_modify_assets(
    asset_type="blueprint",
    operation="rollback",
    params={
        "backup_path": "/Game/.backup/2025-10-22_143022/"
    }
)
```

**Effect**:

- Restores original .uasset files from backup
- Reverts all property values
- Restores original graph structure (if modified)

**Time**: ~1-2 minutes (copy files from backup)

---

## External Links

**Unreal Engine Documentation**:

- [Remote Control API](https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-control-api-in-unreal-engine)
- [Blueprint Scripting](https://dev.epicgames.com/documentation/en-us/unreal-engine/blueprints-visual-scripting-in-unreal-engine)
- [PCG (Procedural Content Generation)](https://dev.epicgames.com/documentation/en-us/unreal-engine/procedural-content-generation-overview)

**NarshaMCP Documentation**:

- [Policy Tools Cheatsheet](../../../docs/POLICY_TOOLS_OVERVIEW.md)
- [Remote Control API Guide](../../../docs/REMOTE_CONTROL_API.md)

**Related Issues**:

- [Issue #151: Asset Modification Wizard](https://github.com/Next-Stage-Inc/ue-code-mcp/issues/151)
- [Issue #118: Remote Control API](https://github.com/Next-Stage-Inc/ue-code-mcp/issues/118)
- [Issue #124: WebSocket Editor Communication](https://github.com/Next-Stage-Inc/ue-code-mcp/issues/124)

---

**Version**: 1.0.0
**Status**: ✅ Production Ready
**Date**: 2025-10-22
