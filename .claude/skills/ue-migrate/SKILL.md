---
name: ue-migrate
model: opus
description: "Purpose: UE version migration assistant. Scans project for breaking changes, deprecated APIs, outdated assets, and deprecated config settings. Generates comprehensive migration report and applies auto-fixes. Triggers: '버전 업그레이드', '마이그레이션', 'version upgrade', 'migrate', 'UE 버전', '엔진 업그레이드'."
compatibility: "Requires NarshaMCP MCP server (v0.9.7+) with Unreal Engine 5.x C++ project"
argument-hint: "source_version target_version (e.g., 5.4 5.7) | --self-test"
metadata:
  version: "1.0.0"
  author: "Next-Stage-Inc"
  license: "MIT"
---

# UE Migration Assistant

**Version**: 1.0.0
**Issue**: #7224
**Purpose**: Comprehensive UE engine version migration: scan deprecated APIs/configs/assets, generate migration report, apply auto-fixes.

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Flow

1. **Parse arguments**: `$ARGUMENTS`
   > If `$ARGUMENTS` is empty, ask the user for source and target UE versions.
   > If only one version is given, treat it as target and auto-detect source.
2. **Detect versions** (if not provided)
3. **Run scan phase** using MCP tools
4. **Generate report**
5. **Offer auto-fix** (with user confirmation)

---

## Auto-Trigger Phrases

### Korean
- "버전 업그레이드", "마이그레이션 도와줘"
- "UE 5.6에서 5.7로 업그레이드", "엔진 업그레이드"
- "엔진 마이그레이션", "UE 버전 올려줘"

### English
- "version upgrade", "migrate to UE 5.7"
- "engine upgrade", "migration assistant"
- "upgrade from 5.4 to 5.7"

### Disambiguation
- `/ue-migrate`: UE engine version migration (scanning, reports, auto-fixes)
- `/ue-cl-tracker`: CL/commit history tracking (when was X added, which CL fixed Y)
- `/ue-debug`: Error diagnosis (current compile/runtime errors, not version migration)
- NOT for: data migration, database migration, asset type conversion

---

## Workflow

### Step 1: Detect Versions

**Auto-detect source version**:
```
ue_glob(pattern="*.uproject")
```
Then read the `.uproject` file and extract `EngineAssociation` field.

**Target version**: From user input or default to latest in migration DB.

**Validate migration path**:
- Source must be < Target
- Both must be UE 5.x (5.1 through 5.7 supported)

### Step 2: Scan Phase

Run these scans (sequentially for dependency):

#### 2a. Deprecated Config Scan
```python
ue_analyze_config(
    operation="scan_deprecated",
    source_version="5.4",
    target_version="5.7"
)
```

Reports deprecated `.ini` settings found in `Config/` directory.

#### 2b. Migration Database Query
The scan_deprecated call internally queries the migration database for:
- Breaking changes (signature changes, parameter additions)
- Deprecated APIs
- Class removals
- Header relocations
- Config changes

### Step 3: Generate Report

```python
ue_analyze_config(
    operation="generate_migration_report",
    source_version="5.4",
    target_version="5.7"
)
```

Generates a comprehensive Markdown report with 6 sections:
1. **Summary**: Total issues, auto/manual ratio
2. **Breaking Changes**: Severity Error items
3. **Deprecated APIs**: Severity Warning items
4. **Asset Updates**: Outdated assets + ResavePackages command
5. **Config Changes**: Deprecated .ini settings found
6. **Manual Actions**: Items requiring manual intervention

Report is saved to: `{project}/Intermediate/NarshaMCP/reports/migration_report.md`

### Step 4: Present Results

Display the report summary to the user:
- Total issues found
- Auto-fixable vs manual count
- Key breaking changes highlighted
- Link to full report file

### Step 5: Auto-Fix (Optional, User Confirmation Required)

> **IMPORTANT**: Always ask the user before applying fixes!

If auto-fixable items exist:
1. List the proposed changes
2. Ask for confirmation
3. Apply via `ue_fix_errors(operation="engine_upgrade", ...)` if available
4. For config renames: Use `ue_analyze_config(operation="modify_config", ...)`

### Step 6: CoreRedirects Generation (Optional)

For class/function renames, generate CoreRedirects entries:
```ini
[CoreRedirects]
+ClassRedirects=(OldName="/Script/MyGame.OldClass",NewName="/Script/MyGame.NewClass")
```

### Step 7: Verification Build (Optional)

```python
ue_build_pipeline(operation="smart")
```

Or:
```python
ue_fix_errors(operation="build_and_fix")
```

### Error Handling

Each step is independent. If one fails:
- Log the failure
- Continue with remaining steps
- Report partial results
- Mark failed steps in the report

Example:
```text
Step 2a: Deprecated Config Scan ✅ (found 5 matches)
Step 2b: Migration DB Query ✅ (12 breaking, 8 deprecated)
Step 3:  Report Generated ✅
Step 5:  Auto-Fix ❌ (Editor not connected — skipped)
Step 7:  Build Verify ❌ (UBT not found — skipped)
```

---

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| source_version | string | auto-detect | Source UE version (e.g., "5.4") |
| target_version | string | latest | Target UE version (e.g., "5.7") |

---

## MCP Tools Used

| Tool | Operation | Purpose |
|------|-----------|---------|
| `ue_analyze_config` | `scan_deprecated` | Find deprecated config settings |
| `ue_analyze_config` | `generate_migration_report` | Generate comprehensive report |
| `ue_fix_errors` | `engine_upgrade` | Apply auto-fixes (optional) |
| `ue_build_pipeline` | `smart` | Verification build (optional) |
| `ue_glob` | - | Find .uproject for version detection |
| `ue_read` | - | Read .uproject for EngineAssociation |

---

## Example Output

```text
=== UE Migration Report: 5.4 → 5.7 ===

Summary:
  Total issues: 24
  Breaking changes: 8 (3 auto-fixable)
  Deprecated APIs: 10 (6 auto-fixable)
  Deprecated configs: 4 (2 auto-fixable)
  Assets needing resave: 142
  Auto-fixable: 11 / Manual: 13

Report saved to: Intermediate/NarshaMCP/reports/migration_report.md

Would you like to apply the 11 auto-fixable changes?
```

---

## Self-Test Mode

`/ue-migrate --self-test` validates skill integrity without performing an actual migration.

### Checks

1. **MCP Health**: Call `ue_check_health()` — verify server is running and responsive
2. **scan_deprecated**: Call `ue_analyze_config(operation="scan_deprecated", source_version="5.4", target_version="5.5")` — verify structured response with `success: true`, `matches` array, `total_deprecated_settings` numeric
3. **generate_migration_report**: Call `ue_analyze_config(operation="generate_migration_report", source_version="5.4", target_version="5.5")` — verify 6-section markdown report, `report_path` contains `migration_report.md`, `summary` object present
4. **Routing**: Verify `.claude/skills/routing.json` contains `ue-migrate` entry with matching trigger patterns

### Output

```text
=== /ue-migrate Self-Test ===
[PASS] MCP Health: healthy (v0.9.7)
[PASS] scan_deprecated: structured response, 0 matches, 0ms
[PASS] generate_migration_report: 6 sections, report_path valid, summary present
[PASS] Routing: entry found, 6 ko + 6 en patterns matched

Result: 4/4 PASSED
```
## MCP Tool Examples

```python
# Find files to migrate
ue_glob(pattern="Source/**/*.h")
# Read migration target
ue_read(identifier="ABaseCharacter")
```
