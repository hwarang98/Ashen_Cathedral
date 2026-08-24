# UE Diff — Reference

**Version**: 1.2.0
**Tool**: `ue_diff`
**Operations**: 7 (smart, replication_audit, diff_hierarchy, compare_configs, compare_config_chain, compare_assets, compare_revision)

---

## Smart Routing Priority

| Priority | Condition | Routes To |
|----------|-----------|-----------|
| 0 | `revision_a` or `revision_b` present | `compare_revision` |
| 1 | `asset_a` or `asset_b` present | `compare_assets` |
| 2 | `config_files` present | `compare_config_chain` |
| 3 | `config_a` or `config_b` present | `compare_configs` |
| 4 | `base_class` present | `diff_hierarchy` |
| 5 | `class_name` with replication hint | `replication_audit` |
| 6 | `class_name` without replication hint | `diff_hierarchy` |

---

## Parameters

### Common
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `operation` | string | `smart` | Operation to execute |
| `limit` | integer | 50 | Max results per response |

### replication_audit
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `class_name` | string | Yes | Target class (e.g., `ALyraPlayerState`) |

### diff_hierarchy
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `class_name` | string | Yes | Project class |
| `base_class` | string | No | Engine base class (auto-detected from PDB if omitted) |

### compare_configs
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `config_a` | string | Yes | First config file path or name |
| `config_b` | string | Yes | Second config file path or name |
| `section` | string | No | Filter to specific INI section |

### compare_config_chain
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `config_files` | string | Yes | Comma-separated config files in override order |
| `section` | string | No | Filter to specific INI section |

### compare_assets
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `asset_a` | string | Yes | First asset — file path or asset name |
| `asset_b` | string | Yes | Second asset — file path or asset name |
| `asset_type` | string | No | Type hint (auto-detect if omitted): blueprint, material, niagara, statetree, datatable, gameplay_effect, gameplay_ability, behavior_tree, generic |

### compare_revision
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `revision_a` | string | Yes | Base revision (SVN number, Git hash, P4 changelist) |
| `revision_b` | string | No | Target revision (default: working copy / HEAD) |
| `vcs_type` | string | No | VCS type: `svn`, `git`, `p4` (auto-detected if omitted) |
| `filter` | string | No | File pattern filter (e.g., `*.uasset`, `*.cpp`) |

---

## Response Schemas

### compare_assets
```json
{
  "operation": "compare_assets",
  "success": true,
  "asset_type": "NiagaraSystem",
  "asset_a": { "path": "...", "name_count": 245, "import_count": 32, "export_count": 18 },
  "asset_b": { "path": "...", "name_count": 251, "import_count": 34, "export_count": 19 },
  "summary": {
    "total_changes": 42,
    "typed_changes": 30,
    "generic_changes": 12,
    "added": 15,
    "removed": 10,
    "modified": 17,
    "diff_method": "typed+generic",
    "truncated": false
  },
  "changes": [
    { "path": "typed.emitters[0].name", "type": "modified", "old": "Sparks", "new": "Flames" },
    { "path": "names.ExplosionForce", "type": "added", "new": "ExplosionForce" }
  ],
  "total_time_ms": 45.2
}
```

### compare_revision
```json
{
  "operation": "compare_revision",
  "success": true,
  "vcs_type": "svn",
  "revision_a": "5948",
  "revision_b": "5950",
  "summary": {
    "total_files_changed": 15,
    "files_processed": 15,
    "uasset_files": 3,
    "text_files": 8,
    "config_files": 2,
    "skipped_files": 2,
    "truncated": false
  },
  "files": [
    {
      "path": "Content/Characters/BP_Hero.uasset",
      "status": "modified",
      "file_type": "uasset",
      "diff_method": "typed+generic",
      "summary": { "total_changes": 12, "typed_changes": 8, "generic_changes": 4 }
    },
    {
      "path": "Source/MyGame/MyCharacter.cpp",
      "status": "modified",
      "file_type": "source",
      "diff_method": "text",
      "summary": { "lines_added": 45, "lines_removed": 12 }
    }
  ],
  "total_time_ms": 2345.6
}
```

---

## Typed Diff — 8 Core Asset Types

| Type | AssetType Variants | Semantic Fields |
|------|-------------------|-----------------|
| Blueprint | BlueprintGeneratedClass, AnimBlueprint, AnimationBlueprint, WidgetBlueprint | nodes, variables, functions, interfaces, connections |
| Material | Material, MaterialInstanceConstant | parameters, connections, nodes |
| Niagara | NiagaraSystem, NiagaraEmitter | emitters, parameters, renderers |
| DataTable | DataTable | rows, properties |
| StateTree | StateTree | tasks, conditions, transitions |
| GameplayEffect | GameplayEffect | modifiers, duration, requirements |
| GameplayAbility | GameplayAbility | tags, cost, cooldown, instancing |
| BehaviorTree | BehaviorTree | tasks, decorators, services, composites |

Unsupported types fall back to **generic diff** (names, imports, exports comparison).

---

## Error Messages

| Error | Cause | Recovery |
|-------|-------|----------|
| `Asset file not found: {path}` | File doesn't exist at path | Check path, use `ue_glob` to find |
| `Failed to parse .uasset: {path}` | Corrupt or unsupported format | Verify file is valid .uasset |
| `asset_a is required` | Missing parameter | Provide both asset_a and asset_b |
| `No VCS detected at project root` | No .svn/.git/.p4config | Provide vcs_type explicitly |
| `VCS command failed` | SVN/Git not installed or error | Check VCS installation |
| `Header file not found` | Class doesn't exist | Check class name spelling |
| `Cannot auto-detect base class` | PDB not loaded | Provide base_class explicitly |
