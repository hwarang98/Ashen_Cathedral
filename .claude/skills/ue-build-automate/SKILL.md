---
name: ue-build-automate
description: "Purpose: Automate UE build/cook/package pipelines using ue_build_pipeline. Answers 'Build my project', 'Cook for Windows', 'Package for shipping', 'Run commandlet'. 9 operations: build, cook, package, BuildCookRun, commandlet, Gauntlet. Triggers: '빌드 자동화', '쿡', '패키징', '커맨드릿 실행', 'build project', 'cook', 'package', 'run commandlet', 'BuildCookRun'."
argument-hint: ["--build (editor build)", "--cook (cook content)", "--package (full package)", "--bcr (BuildCookRun)", "--commandlet <name>"]
---

# UE Build Automate

**Version**: 1.1.0
**Issue**: #6099
**Purpose**: Automate UE build, cook, and package pipelines via MCP

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements
1. **Parse intent** from user query (build, cook, package, commandlet, or full pipeline)
2. **Select pipeline operation** based on intent
3. **Execute MCP build calls** with appropriate parameters
4. **Monitor result** and report success/failure with error details

---

## Auto-Trigger Phrases

### Korean
- "프로젝트 빌드", "에디터 빌드"
- "쿡해줘", "콘텐츠 쿠킹"
- "패키징", "배포용 빌드"
- "커맨드릿 실행", "BuildCookRun"
- "빌드 자동화", "빌드 파이프라인"

### English
- "Build project", "Editor build"
- "Cook for Windows", "Cook content"
- "Package for shipping", "Package project"
- "Run commandlet", "BuildCookRun"
- "Build pipeline", "Build automation"

---

## Workflow

### Step 1: Intent Detection

Classify user request into pipeline stage:

```python
PIPELINE_ROUTES = {
    "build":      ["빌드", "컴파일", "build", "compile"],
    "cook":       ["쿡", "쿠킹", "cook", "cooking"],
    "package":    ["패키징", "배포", "package", "shipping", "deploy"],
    "bcr":        ["BuildCookRun", "BCR", "풀 빌드", "full build"],
    "commandlet": ["커맨드릿", "commandlet", "resave", "validate"],
    "gauntlet":   ["가운틀렛", "gauntlet", "자동화 테스트"]
}
```

### Step 1.5: Confirmation Gate

> **All 8 operations spawn real UE processes (UBT/UAT).** There is no dry-run or info-only mode.
> Always confirm intent before execution to avoid accidental full builds.

Before executing, present the detected parameters to the user:

```text
Detected: operation={op}, platform={platform}, configuration={config}
Estimated time: {estimate}
Proceed? [y/N]
```

**Time estimates by operation**:
| Operation | Typical Duration | Default Timeout |
|-----------|-----------------|-----------------|
| build_editor | 2-10 min | 300s |
| cook | 5-30 min | 900s |
| package | 5-20 min | 600s |
| buildcookrun | 15-60+ min | 1800s |
| run_commandlet | 1-10 min | 300s |
| gauntlet_run | 5-30 min | 900s |

**Rules**:
- Ambiguous requests ("빌드해줘") → ask user to specify operation + platform + configuration
- `buildcookrun` → always warn: "Full BuildCookRun may take 30+ minutes on large projects"
- Never use `operation="smart"` without explicit params — it defaults to `build_editor` (full compile)

### Step 2: Execute Pipeline

> **Smart Mode Routing Reference**:
> `smart` routes by checking params in this priority order:
> 1. `commandlet_name` present → `run_commandlet`
> 2. `cook=true && package=true` → `buildcookrun`
> 3. `cook=true` → `cook`
> 4. `package=true` → `package`
> 5. `target_name` present → `build_target`
> 6. **No params → `build_editor` (full compile!)**
>
> **Always specify `operation` explicitly** to avoid unintended smart routing.

#### 2A: Editor Build (compile only)

```python
ue_build_pipeline(operation="build_editor", params={
    "configuration": "Development",  # Development, Shipping, Debug
    "platform": "Win64",
    "timeout": 300  # 5 min (default 600s is too long for feedback)
})
```

#### 2B: Cook Content

```python
ue_build_pipeline(operation="cook", params={
    "platform": "Windows",  # Windows, Linux, Android, iOS
    "configuration": "Development",
    "timeout": 900  # 15 min
})
```

#### 2C: Package Project

```python
ue_build_pipeline(operation="package", params={
    "platform": "Windows",
    "configuration": "Shipping",
    "output_dir": "<output_path>",
    "timeout": 600  # 10 min
})
```

#### 2D: BuildCookRun (full pipeline)

```python
ue_build_pipeline(operation="buildcookrun", params={
    "platform": "Windows",
    "configuration": "Shipping",
    "cook": true,
    "package": true,
    "timeout": 1800  # 30 min (large projects may need more)
})
```

#### 2E: Run Commandlet

```python
ue_build_pipeline(operation="run_commandlet", params={
    "commandlet_name": "<CommandletName>",  # e.g., "CompileAllBlueprints", "ResavePackages"
    "extra_args": "<additional_args>",
    "timeout": 300  # 5 min
})
```

#### 2F: Gauntlet Test Run

```python
ue_build_pipeline(operation="gauntlet_run", params={
    "test_name": "<test_suite>",
    "platform": "Windows",
    "timeout": 900  # 15 min
})
```

### Step 3: Error Handling

If build fails, auto-diagnose:

```python
# Parse build errors
ue_fix_errors(operation="smart", params={
    "project_root": "<project_root>",
    "auto_detect_git": true
})
```

### Step 4: Report Results

Generate build report with timing, warnings, and errors.

---

## Output Format

```text
=== Build Pipeline Report ===

Pipeline: <build|cook|package|bcr|commandlet>
Platform: <Windows|Linux|Android|iOS>
Configuration: <Development|Shipping|Debug>

--- Result ---
Status: SUCCESS / FAILED
Duration: Xm Xs
Warnings: N
Errors: M

--- Details ---
[If success]
  Output: <output_path_or_binary>
  Size: X MB

[If failed]
  Error Summary:
    1. <file>:<line> — <error_message>
    2. <file>:<line> — <error_message>

  Auto-Fix Suggestion:
    ue_fix_errors(operation="smart") → <suggested_fix>

--- Warnings (top 5) ---
| # | File | Warning |
|---|------|---------|
| 1 | MyActor.cpp:42 | Unused variable 'TempVal' |
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| Build fails with compile errors | Code errors | `ue_fix_errors(operation="smart")` for auto-diagnosis and fix suggestions |
| Cook fails with missing assets | Asset references broken | `ue_search_assets(operation="find_dependencies")` to identify missing refs |
| Package fails with signing error | Certificate not configured | Report error with platform-specific signing setup instructions |
| Commandlet not found | Typo or module not loaded | `ue_build_pipeline(operation="smart")` to list available commandlets |
| Timeout on large project | Build takes >10 minutes | **Prevention**: 1) Use explicit `operation` (avoid smart fallback to build_editor) 2) Set `timeout` parameter 3) Use `target_name` to scope build. **Recovery**: Report partial progress; suggest incremental build with specific targets |

---

## Activation Test Cases

**Positive (5)** - Should activate:
1. "프로젝트 빌드해줘" -> editor build
2. "Windows로 패키징" -> package
3. "CompileAllBlueprints 커맨드릿 실행" -> commandlet
4. "Cook for Windows" -> cook
5. "BuildCookRun 전체 파이프라인" -> bcr

**Negative (3)** - Should NOT activate:
1. "빌드 에러 해결해줘" -> ue-debug skill
2. "C++ 클래스 만들어줘" -> ue-scaffold skill
3. "라이브 코딩 실패" -> ue-livecoding-fix skill

---

**Status**: v1.1.0
**Related**: Issue #6099
