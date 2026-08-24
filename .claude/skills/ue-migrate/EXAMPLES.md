# /ue-migrate Examples

## Example 1: Basic Migration (5.4 to 5.7)

**User**: "UE 5.4에서 5.7로 마이그레이션 도와줘"

**Invocation**:
```bash
/ue-migrate 5.4 5.7
```

**Workflow**:
1. Scan deprecated APIs via `ue_analyze_config(operation="scan_deprecated", source_version="5.4", target_version="5.7")`
2. Generate migration report via `ue_analyze_config(operation="generate_migration_report", source_version="5.4", target_version="5.7")`
3. Report saved to `Intermediate/NarshaMCP/reports/migration_report.md`
4. Auto-fix applicable changes (with user confirmation)
5. Generate CoreRedirects for class/function renames
6. Verify build via `ue_build_pipeline`

**Output**: Markdown report with 6 sections (Summary, Breaking Changes, Deprecated APIs, Asset Updates, Config Changes, Manual Actions).

---

## Example 2: Single Target Version (Auto-Detect Source)

**User**: "Migrate to UE 5.7"

**Invocation**:
```bash
/ue-migrate 5.7
```

**Workflow**: Same as Example 1, but source version is auto-detected from the project's `.uproject` EngineAssociation field.

---

## Example 3: Korean Trigger

**User**: "엔진 업그레이드 5.5에서 5.6"

**Invocation**:
```bash
/ue-migrate 5.5 5.6
```

**Output**:
```
=== Migration Report: UE 5.5 → 5.6 ===

## 1. Summary
| Metric | Count |
|--------|-------|
| Deprecated configs found | 6 |
| Auto-fixable | 2 |
| Manual required | 4 |

## 5. Config Changes
| Setting | File | Line | Type | Auto-Fix |
|---------|------|------|------|----------|
| r.DistanceFieldShadowing | DefaultEngine.ini | 45 | deprecated | No |
| VolumetricFog.HistoryMissSupersampleCount | DefaultEngine.ini | 102 | removed | No |
...
```
