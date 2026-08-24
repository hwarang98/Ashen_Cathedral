---
name: ue-validate
description: "Purpose: Automated asset quality validation using UE's FDataValidationContext. Validates assets against project rules, checks references, detects invalid states, and reports issues with priority. Answers 'Is this asset valid?' with Pass/Fail report. Triggers: 'validate', 'data validation', '에셋 검증', '유효성 검사', 'asset quality', 'check assets', '밸리데이션'."
argument-hint: ["(no args = validate all project assets)", "--path /Game/Characters (specific directory)", "--usecase PreSubmit (validation context)"]
---

# UE Validate -- Asset Quality Validation

**Version**: 1.0.0
**Issue**: #5300
**Purpose**: Run UE's FDataValidationContext-based validation on project assets with priority-ranked issue reporting

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the validation workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Parse scope** from argument (all assets, specific path, or specific assets)
2. **Collect asset list** using `ue_search_assets` or accept user-provided paths
3. **Execute validation** via `ue_editor_assets(operation="validate_asset")`
4. **Aggregate issues** and categorize by severity (Error/Warning)
5. **Rank by reference count** using `ue_editor_assets(operation="find_asset_references")`
6. **Generate validation report** with prioritized action items

---

## Auto-Trigger Phrases

### Korean
- "에셋 검증해줘", "유효성 검사"
- "에셋 품질 확인", "데이터 유효성"
- "에셋 밸리데이션", "밸리데이트"
- "제출 전 검증", "커밋 전 확인"
- "이 에셋 유효해?"

### English
- "Validate assets", "Run data validation"
- "Check asset quality", "Asset validation"
- "Pre-submit check", "Validate before commit"
- "Are these assets valid?", "Check for validation errors"
- "Run IsDataValid"

---

## Validation Workflow

### Step 1: Collect Assets

```python
ToolSearch("select:mcp__narshamcp__ue_search_assets")

# Option A: Specific path from user
assets = ue_search_assets(params={"query": "<user_path>/*", "asset_type": "blueprint"})

# Option B: Full project scan (default)
bp_assets = ue_search_assets(params={"query": "BP_*", "asset_type": "blueprint"})
mat_assets = ue_search_assets(params={"query": "M_*", "asset_type": "material"})
# Combine all asset paths into a list (max 50 per validation call)
```

### Step 2: Validate Assets

```python
ToolSearch("select:mcp__narshamcp__ue_editor_assets")

result = ue_editor_assets(
    operation="validate_asset",
    asset_paths=["/Game/BP_Player", "/Game/M_Rock", ...],
    validate_usecase="Manual",    # or "PreSubmit" for commit checks
    include_warnings=True
)
# Returns: num_requested, num_checked, num_valid, num_invalid, num_warnings, num_skipped
```

**Usecase Options**:
| Usecase | When to Use |
|---------|-------------|
| `Manual` | Default, general-purpose validation |
| `Save` | Validating before saving assets |
| `PreSubmit` | Pre-commit/pre-submit quality gate |
| `Commandlet` | CI/CD pipeline validation |
| `Script` | Scripted/automated validation |

### Step 3: Reference Analysis (Priority Ranking)

For each invalid asset, determine fix priority based on reference count:

```python
# For each invalid asset in results
for invalid in result.assets where status == "Invalid":
    refs = ue_editor_assets(
        operation="find_asset_references",
        asset_path=invalid.asset_path
    )
    # More references = higher priority to fix
```

### Step 4: Generate Report

---

## Output Format

```text
=== Asset Validation Report ===

--- Summary ---
Validation Usecase: Manual
Total Requested: 45
Checked: 43 | Valid: 38 | Invalid: 3 | Warnings: 2 | Skipped: 2

--- Critical Issues (Errors) ---
  Priority  Asset                    Refs  Issue
  1         /Game/BP_Enemy           23    Missing parent class reference
  2         /Game/M_BrokenMat        12    Invalid material expression node
  3         /Game/NS_Explosion       5     Missing required Niagara emitter

--- Warnings ---
  Priority  Asset                    Refs  Issue
  1         /Game/BP_Item            8     Deprecated function usage in graph
  2         /Game/WBP_HUD            3     Unbound widget property

--- Skipped (2) ---
  /Game/DT_Loot — No IsDataValid override
  /Game/Curve_Damage — No IsDataValid override

--- Recommendations (Priority Ordered) ---
  1. [CRITICAL] Fix BP_Enemy (23 downstream refs affected)
  2. [CRITICAL] Fix M_BrokenMat (12 refs affected)
  3. [CRITICAL] Fix NS_Explosion (5 refs affected)
  4. [WARNING]  Review BP_Item deprecated functions (8 refs)
  5. [WARNING]  Review WBP_HUD bindings (3 refs)
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| EditorValidatorSubsystem unavailable | DataValidation plugin not enabled | Report: "Enable DataValidation plugin in your .uproject file" |
| Editor not connected | No live Editor instance | Report: "Asset validation requires a running Editor. Start Unreal Editor first." |
| Asset not found | Invalid path | Skip asset, include in "Skipped" section with error |
| Batch too large (>50 assets) | Too many assets | Auto-chunk into batches of 50, aggregate results |
| Timeout | Complex validators take too long | Report partial results, suggest reducing batch size |
| UE < 5.4 | ValidateAssetsWithSettings unavailable | Falls back to per-asset IsDataValid() automatically |

---

## Evaluation Criteria

### Activation Test Cases

**Positive (5)** - Should activate:
1. "에셋 검증해줘" -> Activate (full project validation)
2. "Validate /Game/Characters assets" -> Activate (scoped validation)
3. "/ue-validate --usecase PreSubmit" -> Activate (pre-submit mode)
4. "이 에셋들 유효해?" -> Activate
5. "Run data validation on blueprints" -> Activate

**Negative (3)** - Should NOT activate:
1. "프로젝트 진단해줘" -> Use `/ue-audit` skill (project-wide health, not asset validation)
2. "빌드 에러 해결해줘" -> Use `/ue-debug` skill (compilation errors, not data validation)
3. "에셋 검색해줘" -> Use `ue_search_assets` directly (search, not validate)

### Disambiguation from `/ue-audit`

| Query Contains | Route To | Reason |
|----------------|----------|--------|
| "검증", "유효성", "validate", "valid" | `/ue-validate` | Asset correctness checking |
| "진단", "감사", "audit", "health", "건강" | `/ue-audit` | Project-wide health scoring |

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| Activation accuracy | >95% |
| Validation detection rate | >90% of UE-reported issues |
| Priority ranking accuracy | >80% correlation with actual impact |
| Full project validation time | <60 seconds (50 assets) |

---

**Status**: Beta
**Related**: Issue #5300 (FDataValidationContext Integration), #5296 (Project Health Report)
