---
name: ue-diff
model: opus
description: "Purpose: Cross-check and mismatch detection for UE projects. Finds gaps between declarations and registrations, class hierarchy deltas, config override cascades, and binary asset diffs. Answers 'What's missing?', 'What changed vs base?', 'How do configs differ?', 'How do assets differ?'. Triggers: 'replication audit', 'config diff', 'config chain', 'diff hierarchy', 'class delta', 'compare configs', 'override cascade', 'compare assets', 'asset diff', 'uasset diff', '리플리케이션 감사', '설정 비교', '계층 비교', '클래스 차이', '설정 체인', '에셋 비교', '에셋 차이'."
compatibility: "Requires NarshaMCP MCP server (v0.9.7+) with Unreal Engine 5.x C++ project"
argument-hint: "TargetName, ConfigFiles, or AssetPaths (e.g., 'ALyraPlayerState', 'DefaultEngine.ini vs WindowsEngine.ini', 'ALyraCharacter vs ACharacter', 'BP_Player_v1.uasset vs BP_Player_v2.uasset')"
metadata:
  version: "1.0.0"
  author: "Next-Stage-Inc"
  license: "MIT"
---

# UE Diff — Cross-Check & Mismatch Detection

**Version**: 1.2.0
**Issue**: #7315, #7422
**Purpose**: Find what's MISSING — gaps between declarations and registrations, hierarchy deltas, config override cascades, binary asset diffs

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the analysis automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Parse `$ARGUMENTS`** to determine operation and target
2. **Auto-route** based on input pattern (see routing table below)
3. **Execute** the appropriate `ue_diff` operation
4. **Generate report** with findings

> If `$ARGUMENTS` is empty, ask the user what they want to diff/audit.

---

## CRITICAL RULES — Call Count Minimization

> **MANDATORY**: This skill MUST complete in **2-4 MCP/tool calls** (Bash excluded).
> Optimal: 2 calls. Maximum: 4 calls.

| # | Rule |
|---|------|
| 1 | **ToolSearch 1x only** — fetch `ue_diff` schema once at start |
| 2 | **smart mode first** — let `ue_diff(smart)` auto-route when possible |
| 3 | **No redundant exploration** — `ue_diff` already handles file discovery internally |
| 4 | **compare_config_chain for 3+ files** — don't call compare_configs repeatedly |

### Target Call Sequences

```text
Optimal (2 calls):                    Standard (3 calls):
1. ToolSearch (1x)                    1. ToolSearch (1x)
2. ue_diff(smart, ...)               2. ue_grep (context, optional)
                                      3. ue_diff(operation, ...)
```

---

## Input Pattern Routing

| Input Pattern | Operation | Example |
|---|---|---|
| Two .uasset file paths or asset names | `compare_assets` | "BP_Player_v1.uasset vs BP_Player_v2.uasset" |
| "asset diff" / "에셋 비교" keyword | `compare_assets` | "NS_Confetti vs NS_Explosion 에셋 비교" |
| Class name only (Actor/Character/PlayerState/GameState/Controller/Pawn/GameMode/PlayerController/ReplicatedComponent) | `replication_audit` | "ALyraPlayerState" |
| Class name + "vs" + base class | `diff_hierarchy` | "ALyraCharacter vs ACharacter" |
| Class name + base_class keyword | `diff_hierarchy` | "ALyraCharacter 계층 비교" |
| Two config file names | `compare_configs` | "DefaultEngine.ini vs WindowsEngine.ini" |
| Three+ config file names or "chain"/"cascade" keyword | `compare_config_chain` | "DefaultEngine → Windows → Editor 설정 체인" |
| Ambiguous class name (no replication hint) | `diff_hierarchy` (auto-detect base) | "ULyraExperienceManagerComponent" |

### Parsing Rules

1. **Extract target names** from `$ARGUMENTS`:
   - Split on "vs", "와", "비교", "compared to"
   - Config files: match `*.ini` patterns
   - Classes: match `A`, `U`, `F` prefixed names
2. **Detect operation keywords**:
   - "asset", "uasset", "에셋 비교", "에셋 차이", ".uasset" → `compare_assets`
   - "replication", "replicated", "DOREPLIFETIME", "push model" → `replication_audit`
   - "hierarchy", "delta", "override", "계층", "차이" → `diff_hierarchy`
   - "config", "ini", "설정" → `compare_configs` or `compare_config_chain`
   - "chain", "cascade", "체인", "3개 이상 파일" → `compare_config_chain`

---

## Operation Details

### 1. replication_audit

**Purpose**: Find declared-but-not-registered UPROPERTY(Replicated) and push model coverage gaps.

```python
ue_diff(operation="replication_audit", class_name="ALyraPlayerState")
```

**Report Format**:
```text
## Replication Audit: ALyraPlayerState

**Declared**: 6 replicated properties
**Registered**: 6 DOREPLIFETIME entries
**Mismatches**: 0

### Push Model Coverage: 5/6 (83%)
| Property | Push Model |
|----------|-----------|
| MyTeamID | Applied |
| PawnData | Applied |
| ...      | ...     |
| StatTags | MISSING  |

**Recommendation**: Add MARK_PROPERTY_DIRTY for StatTags to enable push model replication.
```

### 2. diff_hierarchy

**Purpose**: Show what a project class adds/changes vs its engine base class.

```python
ue_diff(operation="diff_hierarchy", class_name="ALyraCharacter", base_class="ACharacter")
```

**Report Format**:
```text
## Hierarchy Diff: ALyraCharacter vs ACharacter

**Added Methods** (50): DestroyDueToDeath, GetAbilitySystemComponent, ...
**Overridden Methods** (9): BeginPlay, PossessedBy, OnMovementModeChanged, ...
**Added Properties** (8): CameraComponent, HealthComponent, PawnExtComponent, ...
**Interface Changes**: +IAbilitySystemInterface, +IGameplayCueInterface, +IGameplayTagAssetInterface, +ILyraTeamAgentInterface
**Child Classes**: 1
```

### 3. compare_configs

**Purpose**: Diff two config files section by section.

```python
ue_diff(operation="compare_configs", config_a="DefaultEngine.ini", config_b="WindowsEngine.ini")
```

### 5. compare_assets

**Purpose**: Compare two .uasset binary files and return structured diff (names, imports, exports).

```python
ue_diff(operation="compare_assets", asset_a="NS_Confetti.uasset", asset_b="NS_Explosion.uasset")
```

**Report Format**:
```text
## Asset Diff: NS_Confetti vs NS_Explosion

**Asset Type**: NiagaraSystem (auto-detected)
**Asset A**: 245 names, 32 imports, 18 exports
**Asset B**: 251 names, 34 imports, 19 exports

### Changes (12 total: +5 added, -2 removed, ~5 modified)
| Path | Type | Old | New |
|------|------|-----|-----|
| names.ExplosionForce | added | — | ExplosionForce |
| imports.NiagaraModule/NewEmitter | added | — | NiagaraModule/NewEmitter |
| exports.NS_Explosion.serial_size | modified | 1024 | 2048 |
```

### 6. compare_revision

**Purpose**: Compare all changed files between two VCS revisions (SVN/Git/P4).

```python
ue_diff(operation="compare_revision", revision_a="5948", revision_b="5950")
```

**Report Format**:
```text
## Revision Diff: r5948 → r5950

**VCS**: SVN (auto-detected)
**Files Changed**: 15 (3 .uasset, 8 source, 2 config, 2 skipped)

### .uasset Files (typed+generic diff)
| File | Status | Changes |
|------|--------|---------|
| Content/Characters/BP_Hero.uasset | modified | 12 changes (8 typed, 4 generic) |

### Source Files (text diff)
| File | Status | +Lines | -Lines |
|------|--------|--------|--------|
| Source/MyGame/MyCharacter.cpp | modified | +45 | -12 |
```

### 7. compare_config_chain

**Purpose**: Trace override cascade across 3+ config files with final resolved values.

```python
ue_diff(operation="compare_config_chain", config_files="DefaultEngine.ini,WindowsEngine.ini,WindowsEditor.ini")
```

**Report Format**:
```text
## Config Chain: DefaultEngine → WindowsEngine → WindowsEditor

**Files**: 3 analyzed
**Override Steps**: N sections modified across chain
**Final Resolved Values**: [key sections with final values]
```

---

## Smart Mode

When the input is ambiguous, use smart mode — it auto-routes based on parameters:

```python
# Class with replication hints → replication_audit
ue_diff(operation="smart", class_name="ALyraPlayerState")

# Class + base_class → diff_hierarchy
ue_diff(operation="smart", class_name="ALyraCharacter", base_class="ACharacter")

# config_a + config_b → compare_configs
ue_diff(operation="smart", config_a="DefaultEngine.ini", config_b="WindowsEngine.ini")

# config_files → compare_config_chain
ue_diff(operation="smart", config_files="DefaultEngine.ini,WindowsEngine.ini,WindowsEditor.ini")

# asset_a + asset_b → compare_assets
ue_diff(operation="smart", asset_a="NS_Confetti.uasset", asset_b="NS_Explosion.uasset")
```

---

## Error Handling

| Error | Cause | Recovery |
|-------|-------|----------|
| "Header file not found" | Class doesn't exist in Source/ | Check class name spelling, try without A/U prefix |
| "Cannot auto-detect base class" | PDB not loaded | Provide base_class explicitly |
| "Config file not found" | Wrong filename | Use `ue_glob("*.ini")` to discover available configs |
| "Failed to parse .uasset" | Corrupt or unsupported asset | Verify file is valid .uasset (not .umap or text asset) |
| "asset_a is required" | Missing asset path | Provide both asset_a and asset_b parameters |
| `has_symbol_index: false` | PDB index not ready | Wait for indexing or use replication_audit (file-based, no PDB needed) |

---

## Related

- Issue #7315 — ue_diff tool implementation
- Issue #7422 — compare_assets operation (binary .uasset diff)
- Issue #7423 — `/ue-vcs-diff` skill (VCS integration, depends on #7422)
- `/ue-pre-commit` — Uses replication_audit for pre-commit checks
- `/ue-audit` — Project-wide health audit (could integrate config diff)
- `/ue-impact` — Change impact analysis (complementary: impact shows what breaks, diff shows what's different)
## MCP Tool Examples

```python
# Search for patterns during diff analysis
ue_grep(query="UPROPERTY(Replicated)", domain="source")
```
