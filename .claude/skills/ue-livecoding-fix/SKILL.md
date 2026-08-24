---
name: ue-livecoding-fix
description: "Purpose: Auto-diagnose and retry Live Coding (Hot Reload) failures. 5-step workflow (Status Check -> Error Extract -> Diagnose -> Fix Apply -> Retry Compile). Orchestrates ue_editor_automation + ue_fix_errors for seamless compile-fix-retry loop. Triggers: 'live coding 실패', 'hot reload 에러', '핫 리로드', 'live coding fix', '라이브 코딩', 'hot reload failed', 'live coding error', 'patch failed'."
argument-hint: ["error description or empty for auto-detect"]
---

# UE Live Coding Fix -- Auto-Diagnose & Retry

**Version**: 1.0.0
**Issue**: #5297
**Purpose**: Automatically diagnose Live Coding/Hot Reload failures and orchestrate fix-retry loop

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Check status** of Live Coding subsystem
2. **Extract errors** from Editor output log
3. **Diagnose** errors with PDB-verified fix suggestions
4. **Verify** hot reload safety of changed files
5. **Guide user** to apply fixes, then **retry** compilation

---

## Auto-Trigger Phrases

### Korean
- "라이브 코딩 실패", "라이브 코딩 에러"
- "핫 리로드 실패", "핫 리로드 에러"
- "Live Coding 안돼", "패치 실패"
- "컴파일 핫 리로드 안돼"

### English
- "Live Coding failed", "Live Coding error"
- "Hot reload failed", "Hot reload error"
- "Patch failed", "Live coding fix"
- "Hot reload not working"

---

## Workflow

### Step 1: Status Check (Live Coding State)

```python
ue_editor_automation(operation="get_livecoding_status")
# Returns: is_compiling, last_result, is_enabled
```

**Decision Tree**:
- `is_compiling == true` -> Wait 5 seconds, re-check (max 3 times)
- `is_enabled == false` -> Report: "Live Coding is disabled. Enable via Editor Preferences > Live Coding"
- `last_result == "Success"` -> Report: "Last Live Coding compile succeeded. No action needed."
- Editor offline (error response) -> Skip to Step 3 with `ue_fix_errors(operation="scan_build_log")` instead

### Step 2: Extract Errors from Editor Log

```python
ue_editor_automation(operation="get_error_log", severity_filter="error", max_entries=200)
```

Extract compile error lines containing: `error C`, `error LNK`, `Compile failed`, `Live coding failed`.

If no errors found -> Report: "No compile errors in Editor log. Check manually or retry compile."

### Step 3: Diagnose Errors (PDB-verified)

For each extracted error, use the error analysis pipeline:

```python
# Option A: Smart mode (auto-detects best approach)
ue_fix_errors(operation="smart")

# Option B: Manual mode for specific errors
ue_fix_errors(operation="manual", error_message="<error text from Step 2>")
```

This returns:
- Error classification (missing include, linker, syntax, etc.)
- PDB-verified fix suggestions with confidence scores
- File paths and line numbers for each error

### Step 4: Hot Reload Safety Check

```python
ue_fix_errors(operation="hotreload_check", auto_detect_git=true)
```

**Check for**:
- Template changes in UCLASS (not hot-reload safe)
- Static UPROPERTY modifications (may cause issues)
- Struct layout changes (requires full rebuild)

If `hot_reload_safe == false` -> Warn: "Changes include hot-reload-incompatible modifications. Full rebuild recommended instead of Live Coding."

### Step 5: Apply Fixes & Retry

1. Present fix suggestions to user with confidence scores
2. For high-confidence fixes (>= 0.85): Show diff and recommend applying
3. For lower-confidence fixes: Show details for manual review
4. After fixes are applied:

```python
# Retry Live Coding compile
ue_editor_automation(operation="compile_project")
# This triggers LiveCoding.Compile and polls for success/failure
```

5. If retry fails -> Return to Step 2 (max 3 retry cycles)
6. If retry succeeds -> Report success

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| `get_livecoding_status` fails | Editor offline | Use `ue_fix_errors(operation="scan_build_log")` for offline diagnosis |
| `get_error_log` returns empty | Log rotated or cleared | Use `ue_fix_errors(operation="scan_build_log")` to check Saved/Logs/ |
| `hotreload_check` finds template issues | Template in UCLASS | Recommend full rebuild: `ue_editor_automation(operation="compile_project")` or manual UBT |
| `compile_project` timeout | Large project | Suggest checking Editor output log manually |
| 3 retries exhausted | Deep/cascading errors | Escalate to `/ue-debug` skill for full diagnosis |
| No PDB index available | First build or index missing | Suggestions fall back to heuristic mode (lower confidence) |

---

## Output Format

```text
=== Live Coding Fix Report ===

--- Status ---
Live Coding: Enabled
Last Result: Failed
Is Compiling: No

--- Diagnosis (N errors found) ---
1. [MyCharacter.cpp:42] error C2065: 'MyVar' undeclared identifier
   Fix (95% confidence): Add #include "MyVar.h"

2. [MyCharacter.cpp:88] error C2039: 'OldFunc' not a member of 'AMyActor'
   Fix (85% confidence): Rename to 'NewFunc' (API changed in 5.4)

--- Hot Reload Safety ---
All changes are hot reload safe.

--- Retry ---
Applying Live Coding compile...
Result: Success! Patch applied in 3.2s.
```

---

## Common Failures

| Error | Cause | Solution |
|-------|-------|----------|
| Live Coding disabled | Editor preference | Enable in Edit > Editor Preferences > Live Coding |
| "Modules updated" but changes not visible | DLL not patched | Restart Editor (rare edge case with static init) |
| Linker errors after fix | Missing module dependency | Check Build.cs for required modules |
| Repeated failure on same error | Cascading error | Fix the first error only, then retry |

---

## Related

- **Issue #5297**: Auto-diagnose Live Coding failures + retry loop
- **Issue #4843**: compile_project LiveCoding pipeline
- **Issue #4319**: hotreload_check operation
- **Issue #5291**: Build Error Pipeline (reused error analysis)
- **Skills**: `/ue-debug` (full diagnosis), `/unreal-error-doctor` (compilation errors)

---

**Status**: Phase 1 MVP
**Last Updated**: 2026-02-25
