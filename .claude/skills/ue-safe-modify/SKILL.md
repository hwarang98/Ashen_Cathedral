---
name: ue-safe-modify
model: opus
description: "Purpose: AI 변경 전 자동 스냅샷 + 검증 실패 시 원클릭 롤백. 5 modes: start(스냅샷), verify(검증), commit(확정), rollback(복원), auto(전자동). Git + filesystem dual-layer backup for C++/BP/Config. Triggers: '안전하게 수정', '롤백', '되돌려', 'safe modify', 'with rollback', 'undo changes', 'safely change', '안전장치', '백업하고 수정'."
compatibility: "Requires NarshaMCP MCP server (v0.7.0+) with Unreal Engine 5.x project. Git must be initialized."
allowed-tools: [Bash, Read, Grep, Glob, Edit, Write]
argument-hint: "start|verify|commit|rollback|auto [file1 file2 ...] [--description 'what to do']"
metadata:
  version: "1.0.0"
  author: "Next-Stage-Inc"
  license: "MIT"
  issue: "#7592"
---

## Current State (auto-collected)
- Branch: !`git branch --show-current`
- Status: !`git status --short`
- Project Root: !`echo $UECODEGEN_PROJECT_DIR`

> If the above data is empty, collect manually before proceeding.

# UE Safe Modify — Snapshot + Rollback Safety Net

**Version**: 1.0.0
**Issue**: #7592, #7580
**Purpose**: AI가 프로젝트 파일을 수정하기 전에 자동 스냅샷을 만들고, 검증 실패 시 원클릭 롤백
**Requires**: Git initialized project, NarshaMCP v0.7.0+

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the requested mode automatically**.
> Do NOT just display documentation — actually run the commands!

### Mode Selection

Parse `$ARGUMENTS` to determine mode:

| Argument | Mode | Description |
|----------|------|-------------|
| `start [files...]` | START | 스냅샷 생성 (대상 파일 지정 또는 auto-detect) |
| `verify` | VERIFY | 변경 사항 검증 (빌드, BP 무결성, Config 유효성) |
| `commit` | COMMIT | 검증 통과 후 백업 삭제, 확정 |
| `rollback` | ROLLBACK | 모든 변경을 스냅샷 시점으로 정밀 복원 |
| `auto [--description '...']` | AUTO | start → 수정 → verify → commit/rollback 전자동 |

> If `$ARGUMENTS` is empty or unrecognized, default to `start`.

---

## Auto-Trigger Phrases

### Korean
- "안전하게 수정해줘", "스냅샷 찍고 수정"
- "롤백 가능하게 수정", "되돌릴 수 있게"
- "수정 전 백업해줘", "안전장치 걸고 수정"
- "원래대로 돌려줘", "백업하고 수정해줘"
- "안전 모드로 수정"

### English
- "Safely modify", "Modify with rollback"
- "Snapshot before changing", "With undo support"
- "Rollback changes", "Undo AI changes"
- "Safely change", "Backup before change"
- "Safe mode modify"

---

## Architecture: Unified Filesystem Backup (with Git Enhancement)

All files are backed up via **filesystem copy** — this works universally, with or without git.
When git IS available, tracked files get an additional git-based restore path for extra safety.

```
┌─────────────────────────────────────────────────┐
│ PRIMARY: Filesystem Copy (ALL files)            │
│ .h, .cpp, .ini, .cs, .json, .uasset, .umap     │
│ Method: cp to backup dir, cp back on rollback   │
│ Works: ALWAYS (git or no git)                   │
│ Precision: per-file, sha256-verified            │
├─────────────────────────────────────────────────┤
│ ENHANCEMENT: Git Restore (when git available)   │
│ Tracked text files only (.h, .cpp, .ini, .cs)   │
│ Method: git checkout {head_sha} -- <files>      │
│ Benefit: faster, leverages git object store     │
│ Fallback: filesystem copy if git unavailable    │
├─────────────────────────────────────────────────┤
│ Manifest: .ue-safe-modify-backup/{id}/manifest  │
│ Records every file, type, hash, backup path     │
│ Enables precise rollback of THIS session only   │
└─────────────────────────────────────────────────┘
```

> **IMPORTANT**: Many UE projects are NOT git-initialized (e.g., downloaded Lyra, marketplace projects).
> The skill MUST work without git. Filesystem copy is the primary mechanism; git is a bonus.

### Backup Directory Structure

```
{project_root}/
└── .ue-safe-modify-backup/
    └── {session_id}/                    # e.g., 20260329_143022
        ├── manifest.json                # File tracking manifest
        └── Content/                     # Mirrors project structure
            ├── BP_Player.uasset
            └── Maps/
                └── MainMap.umap
```

### Manifest Format

```json
{
  "session_id": "20260329_143022",
  "head_sha": "abc123def456...",
  "created_at": "2026-03-29T14:30:22",
  "project_root": "D:/MyProject",
  "status": "active",
  "files": [
    {
      "path": "Source/MyGame/Characters/MyChar.h",
      "type": "tracked",
      "original_hash": "def456...",
      "backup_method": "git"
    },
    {
      "path": "Content/Blueprints/BP_Player.uasset",
      "type": "binary",
      "original_hash": "789abc...",
      "backup_method": "filesystem",
      "backup_path": ".ue-safe-modify-backup/20260329_143022/Content/Blueprints/BP_Player.uasset"
    }
  ]
}
```

---

## MODE 1: START — 스냅샷 생성

### Input
- `$ARGUMENTS`: `start [file1 file2 ...]`
- If no files specified: auto-detect target files in Source/ and Content/ directories

### Workflow

```bash
# Phase 1: Generate session ID and detect git availability
SESSION_ID=$(date +%Y%m%d_%H%M%S)
BACKUP_DIR=".ue-safe-modify-backup/${SESSION_ID}"
HAS_GIT=false
git rev-parse --git-dir > /dev/null 2>&1 && HAS_GIT=true
HEAD_SHA=""
$HAS_GIT && HEAD_SHA=$(git rev-parse HEAD)

# Phase 2: Collect target files
# Option A: Files specified in arguments
# Option B: Auto-detect
#   - With git: git status --short | filter relevant extensions
#   - Without git: scan Source/ and Content/ for specified files

# Phase 3: Backup ALL files via filesystem copy (PRIMARY method)
mkdir -p "${BACKUP_DIR}"
for file in all_target_files:
    target_dir="${BACKUP_DIR}/$(dirname $file)"
    mkdir -p "$target_dir"
    cp "$file" "$target_dir/"
    hash=$(sha256sum "$file" | cut -d' ' -f1)
    # Record in manifest with backup_path and hash

# Phase 4: Write manifest.json
# Write JSON manifest to ${BACKUP_DIR}/manifest.json
# Include: session_id, head_sha (if git), has_git, files[], created_at

# Phase 5: Ensure .gitignore includes backup directory (if git project)
if $HAS_GIT; then
    grep -q '.ue-safe-modify-backup/' .gitignore 2>/dev/null || \
        echo '.ue-safe-modify-backup/' >> .gitignore
fi
```

### Output

```
=== Safe Modify: Snapshot Created ===

Session: {session_id}
HEAD SHA: {head_sha} (short)
Files tracked: {N} ({M} tracked, {K} binary)
Backup dir: .ue-safe-modify-backup/{session_id}/

Tracked files (git restore):
  - Source/MyGame/Characters/MyChar.h
  - Source/MyGame/Characters/MyChar.cpp

Binary files (filesystem backup):
  - Content/Blueprints/BP_Player.uasset (2.3 MB)

Next: Make your changes, then run /ue-safe-modify verify
      Or /ue-safe-modify rollback to revert all changes
```

---

## MODE 2: VERIFY — 변경 사항 검증

### Input
- No arguments needed (reads from most recent active manifest)

### Prerequisites
- Active session exists (`.ue-safe-modify-backup/*/manifest.json` with `status: "active"`)

### Workflow

```
Phase 1: Load active manifest
  → Find most recent manifest.json with status "active"
  → Parse file list and classify by type

Phase 2: Detect what changed
  → git diff --name-only (for tracked files in manifest)
  → Compare binary file hashes (for .uasset files in manifest)
  → Classify: cpp_changed, bp_changed, config_changed, other_changed

Phase 3: Tier-based verification (only run tiers relevant to change type)
```

### Verification Tiers

**Tier 1: C++ Static Check** (if cpp_changed)
```python
ue_fix_errors(operation="preflight", params={
    "project_root": "{project_root}"
})
# Speed: <50ms | Accuracy: 90%
# Checks: syntax errors, missing includes, type mismatches
```

**Tier 2: C++ Deep Check** (if Tier 1 passes AND cpp_changed)
```python
ue_fix_errors(operation="preflight_deep", params={
    "project_root": "{project_root}"
})
# Speed: ~30s | Accuracy: 99%
# Checks: linker errors, systemic issues, category classification
```

**Tier 3: Blueprint Integrity** (if bp_changed)
```python
# For each modified .uasset that is a Blueprint:
ue_manage_blueprint(operation="get_structure", params={
    "blueprint_name": "{bp_name}",
    "include_nodes": true,
    "include_variables": true
})
# Checks: node structure parseable, no orphan connections, variables intact
```

**Tier 4: Config Validation** (if config_changed)
```python
# For each modified .ini file:
ue_analyze_config(operation="search_config", params={
    "query": "{changed_section_or_key}"
})
# Checks: config values are valid types, no broken references
```

**Tier 5: Architecture Lint** (optional, graceful degradation)
```python
# Check if /ue-architecture-lint skill exists (Issue #7590)
architecture_lint_exists = Glob(".claude/skills/ue-architecture-lint/SKILL.md")
if architecture_lint_exists:
    Skill("ue-architecture-lint")
else:
    # INFO: /ue-architecture-lint not installed. Skipping Tier 5.
    # This does NOT block verify — Tiers 1-4 are sufficient.
    pass
```

### Verdict Logic

```
PASS:  All applicable tiers pass → safe to commit
WARN:  Non-critical issues found (warnings only) → commit with caution
FAIL:  Critical issues found (errors) → recommend rollback
```

### Output

```
=== Safe Modify: Verification Report ===

Session: {session_id}
Changed files: {N} modified since snapshot

--- Tier 1: C++ Static Check ---
Result: PASS | {details}

--- Tier 2: C++ Deep Check ---
Result: PASS | {details}

--- Tier 3: Blueprint Integrity ---
Result: PASS | BP_Player: {node_count} nodes, {var_count} variables — structure intact

--- Tier 4: Config Validation ---
Result: SKIP (no config changes)

--- Tier 5: Architecture Lint ---
Result: SKIP (skill not installed)

=== Verdict: PASS ===
Recommendation: Safe to commit. Run /ue-safe-modify commit

=== Verdict: FAIL ===
Recommendation: Rollback recommended. Run /ue-safe-modify rollback
Failures:
  - Tier 2: 3 linker errors detected
  - Details: {error_list}
```

---

## MODE 3: COMMIT — 검증 후 확정

### Input
- No arguments needed

### Workflow

```
Phase 1: Check verification status
  → If verify was NOT run in this session:
    → Auto-run verify first
    → If verify FAILS: abort commit, recommend rollback

Phase 2: Verify passed — clean up backup
  → Load manifest
  → Delete backup directory: rm -rf .ue-safe-modify-backup/{session_id}/
  → If no other active sessions: remove backup root if empty

Phase 3: Summary report
  → List all changes that are now confirmed
  → Remind user to git add + git commit when ready
```

### Output

```
=== Safe Modify: Changes Committed ===

Session: {session_id} — CLOSED
Verification: PASSED (all {N} tiers)
Backup: Deleted (.ue-safe-modify-backup/{session_id}/)

Modified files (confirmed):
  - Source/MyGame/Characters/MyChar.h
  - Source/MyGame/Characters/MyChar.cpp
  - Content/Blueprints/BP_Player.uasset

Next: git add + git commit when ready
      Or /ue-pre-commit for impact analysis before committing
```

---

## MODE 4: ROLLBACK — 정밀 복원

### Input
- No arguments needed (rolls back most recent active session)
- Optional: `rollback {session_id}` to target a specific session

### Workflow

```
Phase 1: Load manifest
  → Find target manifest (most recent active, or specified session_id)
  → Parse file list

Phase 2: Restore all files from backup (PRIMARY — filesystem copy)
  → For each file in manifest:
    cp {backup_path} {file_path}
  → This works universally (git or no git)

Phase 3: Verify restoration (sha256 hash comparison)
  → For each restored file:
    current_hash=$(sha256sum {file_path} | cut -d' ' -f1)
    if current_hash != original_hash:
      WARNING: Hash mismatch for {file_path}
      # This can happen if the file was modified outside this session

Phase 5: Clean up
  → Update manifest status to "rolled_back"
  → Delete backup directory
  → Report results
```

### Output

```
=== Safe Modify: Rollback Complete ===

Session: {session_id} — ROLLED BACK
HEAD SHA restored to: {head_sha} (for listed files only)

Restored files:
  ✅ Source/MyGame/Characters/MyChar.h — hash verified
  ✅ Source/MyGame/Characters/MyChar.cpp — hash verified
  ✅ Content/Blueprints/BP_Player.uasset — hash verified

All {N} files restored to pre-modification state.
Backup directory cleaned up.
```

### Hash Mismatch Handling

If a file's restored hash doesn't match the original:

```
⚠️ Hash mismatch: Source/MyGame/MyChar.h
   Expected: def456...
   Got:      abc789...
   Cause: File may have been modified by another process during this session.
   Action: Manual review recommended. The backup hash reflects the state at
           snapshot time. Use `git diff` to inspect differences.
```

---

## MODE 5: AUTO — 전자동 (Start → Modify → Verify → Commit/Rollback)

### Input
- `auto --description "AMyCharacter.h에 Jump 기능 추가"`
- `auto Source/MyGame/MyChar.h --description "Add health regeneration"`

### Workflow

```
Phase 1: START
  → Execute MODE 1 (start) with specified files or auto-detect
  → Report snapshot created

Phase 2: MODIFY
  → Claude performs the modifications described in --description
  → Uses standard Edit/Write tools
  → All changes tracked by manifest

Phase 3: VERIFY
  → Execute MODE 2 (verify)
  → Evaluate verdict

Phase 4: DECISION GATE
  → PASS or WARN: Execute MODE 3 (commit)
  → FAIL: Execute MODE 4 (rollback) + error report

Phase 5: REPORT
  → Final summary of what happened
```

### Output (Success)

```
=== Safe Modify: Auto Complete (SUCCESS) ===

Description: "AMyCharacter.h에 Jump 기능 추가"
Session: {session_id}

Phase 1 (Start):   ✅ Snapshot created — 2 files tracked
Phase 2 (Modify):  ✅ Changes applied
Phase 3 (Verify):  ✅ All tiers passed
Phase 4 (Commit):  ✅ Backup cleaned up

Modified files:
  - Source/MyGame/Characters/MyChar.h (+15 lines)
  - Source/MyGame/Characters/MyChar.cpp (+42 lines)
```

### Output (Failure + Auto-Rollback)

```
=== Safe Modify: Auto Complete (ROLLED BACK) ===

Description: "AMyCharacter.h에 Jump 기능 추가"
Session: {session_id}

Phase 1 (Start):    ✅ Snapshot created — 2 files tracked
Phase 2 (Modify):   ✅ Changes applied
Phase 3 (Verify):   ❌ FAILED
  - Tier 2: 3 linker errors
    - LNK2019: unresolved external symbol "UCharacterMovementComponent::Jump"
    - LNK2019: unresolved external symbol "ACharacter::CanJump"
    - LNK2001: unresolved external symbol "ACharacter::JumpMaxCount"
Phase 4 (Rollback): ✅ All files restored to pre-modification state

Files restored:
  ✅ Source/MyGame/Characters/MyChar.h — hash verified
  ✅ Source/MyGame/Characters/MyChar.cpp — hash verified

Recommendation:
  The modification caused linker errors. Likely missing module dependency
  in Build.cs. Consider adding "Engine" to PublicDependencyModuleNames.
```

---

## Error Recovery

| Error | Cause | Recovery |
|-------|-------|----------|
| No active session found | `verify`/`commit`/`rollback` without prior `start` | Run `start` first |
| Manifest corrupted | JSON parse failure | Fallback: `git checkout HEAD -- .` for tracked files, warn about binary files |
| Backup directory missing | Manually deleted or disk error | For tracked files: `git checkout {head_sha} -- <files>` still works. Binary files: unrecoverable, warn user |
| Editor locks .uasset | UE Editor has file handle | Warn: "Close Unreal Editor before rollback", retry after |
| Hash mismatch after restore | External modification during session | Report mismatch, recommend manual review via `git diff` |
| `.gitignore` not writable | Permission issue | Warn user to manually add `.ue-safe-modify-backup/` to .gitignore |
| Git not initialized | No `.git` directory | Abort with error: "Git required for safe-modify. Run `git init` first." |
| MCP tools unavailable | Server not running | Verify tiers 1-4 skip gracefully, warn "verification incomplete" |

---

## Integration Points

### With /ue-pre-commit (Issue #5299)
```
/ue-safe-modify start → (modifications) → /ue-safe-modify verify
                                          → /ue-pre-commit (impact analysis)
                                          → /ue-safe-modify commit
                                          → git commit
```

### With /ue-architecture-lint (Issue #7590)
```
/ue-safe-modify verify
  → Tier 1-4: MCP-based checks
  → Tier 5: Glob for ue-architecture-lint skill
    → Found: invoke for architecture validation
    → Not found: skip gracefully (INFO message only)
```

### With /ue-debug (Issue #7591)
```
/ue-safe-modify auto → FAIL → rollback
  → User can then: /ue-debug to diagnose the specific error
  → Fix the root cause
  → /ue-safe-modify auto (retry)
```

---

## Safety Invariants

1. **Backup directory is NEVER committed** — `.gitignore` entry enforced at `start`
2. **Rollback is always per-file** — never `git checkout -- .` (which would revert unrelated changes)
3. **Manifest is the single source of truth** — no implicit file tracking
4. **Hash verification after rollback** — guarantees restoration accuracy
5. **Binary files use filesystem copy** — git cannot selectively restore binary diffs
6. **Session isolation** — multiple sessions can coexist without interference
7. **No destructive operations on unrelated files** — only files in the manifest are touched

---

## Activation Test Cases

### Positive (should activate this skill)
1. "안전하게 수정해줘" → `/ue-safe-modify`
2. "롤백 가능하게 AMyChar.h 수정" → `/ue-safe-modify`
3. "Safely modify BP_Player" → `/ue-safe-modify`
4. "Modify with rollback support" → `/ue-safe-modify`
5. "백업하고 수정해줘" → `/ue-safe-modify`

### Negative (should NOT activate)
1. "AMyChar를 ABaseChar로 리네임" → `/safe-class-rename` (rename, not safe-modify)
2. "BP_Player 일괄 수정" → `/asset-modification-wizard` (bulk modification)
3. "빌드 에러 해결해줘" → `/ue-debug` (error fixing, not safe modification)
4. "커밋 전 검사해줘" → `/ue-pre-commit` (pre-commit analysis, not modification)
5. "프로젝트 진단해줘" → `/ue-audit` (project health, not modification)
