---
name: asset-modification-wizard
description: "Purpose: safe bulk asset modification. 5-stage safety workflow (Discovery → Impact → Strategy → Dry-Run → Execute) for Blueprint and PCG assets via Remote Control API or .uasset binary modification. 50-200x speedup with Editor open. Not for single-asset analysis (use blueprint-flow or material-analysis). Triggers: 'bulk modify', 'change all Blueprints', 'PCG parameter tuning', 'add property to Blueprints', '일괄 수정', 'BP 대량 변경'."
---

# Asset Modification Wizard

**Version**: 1.0.0 (Issue #151 Phase 4)
**Purpose**: Safe bulk modification of Blueprint and PCG assets with smart execution strategy
**Author**: NarshaMCP Development Team

---

## 🎯 Purpose

Safely modify Blueprint and PCG assets in bulk with:

- **Smart Execution Strategy**: Automatically selects fastest method (Remote Control API or .uasset modification)
- **Safety Checks**: Pre-modification validation and impact analysis
- **Bulk Operations**: Modify hundreds of assets efficiently
- **Rollback Support**: Automatic backup before modifications

**Key Benefits**:

- 50-200x faster when Editor is open (Remote Control API)
- 95% success rate with automatic validation
- Safe bulk operations with dry-run preview
- Cross-domain support (Blueprint + PCG)

---

## 🔍 Auto-Load Trigger Phrases

**Bulk modification queries**:

- "모든 블루프린트에서 MaxHealth 기본값 100으로 바꿔줘" / "Change MaxHealth default to 100 in all Blueprints"
- "BP_Enemy*블루프린트에 MovementSpeed 속성 추가해줘" / "Add MovementSpeed property to BP_Enemy* Blueprints"
- "PCG 그래프에서 Seed 값 전부 1234로 변경" / "Change all Seed values to 1234 in PCG graphs"

**Safe modification queries**:

- "이 블루프린트 수정해도 안전해?" / "Is it safe to modify this Blueprint?"
- "영향 범위 확인하고 수정해줘" / "Check impact and modify"
- "미리보기로 먼저 보여줘" / "Show me preview first"

**Property modification queries**:

- "UPROPERTY 기본값 바꿔줘" / "Change UPROPERTY default value"
- "Component 설정 변경해줘" / "Change Component settings"
- "PCG 파라미터 튜닝해줘" / "Tune PCG parameters"

**Keywords**: `modify`, `change`, `update`, `set`, `add property`, `remove`, `bulk`, `all blueprints`, `PCG`, `수정`, `변경`, `추가`, `제거`, `일괄`, `모든 블루프린트`

---

## 🧭 5-Stage Workflow

### Complete Flow Path

```text
Stage 1: Target Discovery       → Find assets to modify
Stage 2: Impact Analysis        → Assess modification impact and risks
Stage 3: Strategy Selection     → Choose fastest execution method
Stage 4: Dry-Run Preview        → Show what will change (user approval)
Stage 5: Execution & Validation → Apply changes and verify success
```

---

### Stage 1: Target Discovery

**Goal**: Find all assets matching the modification criteria

**Tool**: `ue_search_assets(asset_type="blueprint", pattern="BP_Enemy*")`

**Output**:

```text
🔍 Stage 1: Target Discovery

Found **3 Blueprints** matching "BP_Enemy*":
- BP_Enemy_Melee (2.3 MB)
- BP_Enemy_Ranged (1.8 MB)
- BP_Enemy_Boss (5.2 MB)

Total size: 9.3 MB
```

---

### Stage 2: Impact Analysis

**Goal**: Assess modification impact, identify risks

**Output**:

```text
📊 Stage 2: Impact Analysis

Modification Impact:
- Direct callers: 12 Blueprints reference these enemies
- Child Blueprints: 5 inherit from these classes
- Risk Level: **Low** ✅

Recommendation: Safe to proceed with caution
```

---

### Stage 3: Strategy Selection

**Goal**: Automatically select fastest execution method

**Decision Tree**:

```text
Is Unreal Editor open?
├─ YES → Remote Control API (50-200x faster) ✅
└─ NO  → .uasset modification (safe, slower)
```

**Output**:

```text
⚡ Stage 3: Strategy Selection

Selected Strategy: Remote Control API ✅
- Editor detected on port 30010
- Estimated time: **2-5 seconds** (vs 3-5 minutes for .uasset)
- 50-200x performance improvement
```

---

### Stage 4: Dry-Run Preview

**Goal**: Show exactly what will change before applying

**Output**:

```text
🔍 Stage 4: Dry-Run Preview

Proposed Changes (Preview Only):

| Blueprint | Property | Old Value | New Value | Status |
|-----------|----------|-----------|-----------|--------|
| BP_Enemy_Melee | MaxHealth | 50 | 100 | ✅ Will change |
| BP_Enemy_Ranged | MaxHealth | 30 | 100 | ✅ Will change |
| BP_Enemy_Boss | MaxHealth | 500 | 100 | ⚠️ Will change |

⚠️ Warnings:
- BP_Enemy_Boss: Significant decrease (500 → 100) may affect gameplay

**Do you want to proceed with these changes?**
```

**User Confirmation Required**: Wait for approval before Stage 5.

---

### Stage 5: Execution & Validation

**Goal**: Apply modifications and verify success

**Output (after user confirms)**:

```yaml
✅ Stage 5: Execution Complete

Modifications Applied Successfully!

Execution Summary:
- Strategy: Remote Control API
- Execution time: **3.2 seconds**
- Success rate: **100%** (3/3)

Validation:
- ✅ All assets verified (integrity check passed)
- ✅ Backup created: /Game/.backup/2025-10-22_143022/
- ✅ New values verified in assets
```

---

## 📊 Performance Benchmarks

### Remote Control API vs .uasset Modification

| Operation | .uasset Method | Remote Control API | Speedup |
|-----------|----------------|-------------------|---------|
| Single Blueprint property | 5-10s | 0.1-0.2s | **50x** |
| 10 Blueprint properties | 50-100s | 0.5-1s | **100x** |
| 100 Blueprint properties | 500-1000s | 2-5s | **200x** |

**Note**: Remote Control API requires Unreal Editor to be open.

---

## Output Format

```text
=== Asset Modification Report ===

Target: {asset_type} matching "{pattern}"
Discovered: {N} assets

--- Impact Analysis ---
| Asset | Direct Refs | BP Refs | Risk |
|-------|------------|---------|------|
| {name} | {count} | {count} | LOW/MED/HIGH |

--- Strategy ---
Selected: {Remote Control API | .uasset Modification}
Reason: {justification}

--- Dry-Run Results ---
{N} assets would be modified
{N} references would update automatically
Estimated time: {X}s

--- Execution ---
Modified: {N}/{total} assets
Validated: {pass}/{total} passed
Rollback available: YES
```

## Error Recovery

| Error | Cause | Recovery |
|-------|-------|----------|
| `No assets found matching pattern` | Asset type or pattern mismatch | Verify asset type with `ue_search_assets`; use wildcard `*` pattern to broaden search |
| `Remote Control API unavailable` | Unreal Editor not running | Start Editor first, or fall back to `.uasset` binary modification strategy |
| `Validation failed after modification` | Modified asset breaks references | Auto-rollback via Stage 5 validation; inspect reference chain with `ue_manage_blueprint(get_structure)` |

---

## 📚 Related Files

For detailed information, see:

- **EXAMPLES.md** - Complete usage examples and conversation scenarios
- **ADVANCED.md** - Smart strategy selection, safety features, integration
- **REFERENCE.md** - Complete tool parameters, execution strategies, safety guidelines

---

**Status**: ✅ Production Ready (Issue #151 Phase 4)
**Version**: 1.0.1
**Date**: 2025-10-22
