---
name: ue-test-gen
description: "Purpose: One-shot UE Automation Test code generation from target class/function. Uses ue_generate_code(generate_test) for template-based test creation with correct macros, assertions, and flags. Fast single-pass alternative to ue-automation-test-builder. Triggers: '테스트 생성', '테스트 만들어줘', 'generate test for', 'create test for', '유닛 테스트 생성'."
allowed-tools: [Bash, Read, Glob, Grep, Edit, Write, Task]
argument-hint: ["target class or function name (e.g., ACharacter::TakeDamage)"]
---

# UE Test Generator

**Version**: 1.0.0
**Priority**: High
**Goal**: One-shot generation of compilable UE Automation Test code from a target class or function
**Issue**: #5294

---

## Purpose

Generate **complete, compilable** UE Automation Test code from a target class/function name.
This skill uses `ue_generate_code(generate_test)` for template-based generation — fast, deterministic, and accurate.

**Difference from `/ue-automation-test-builder`**:
- `/ue-test-gen` = **Fast one-shot** generation (seconds, template-based)
- `/ue-automation-test-builder` = **Interactive 6-phase cycle** (minutes, AI-guided analysis/execution/evaluation)

**4 Test Types Supported**:

| Type | UE Macro | When to Use |
|------|----------|-------------|
| `simple` | `IMPLEMENT_SIMPLE_AUTOMATION_TEST` | Pure unit tests, no world needed |
| `bdd_spec` (default) | `BEGIN_DEFINE_SPEC / END_DEFINE_SPEC` | Recommended — Given/When/Then |
| `complex_latent` | `IMPLEMENT_COMPLEX_AUTOMATION_TEST` + latent commands | Multi-frame async tests |
| `functional` | `AFunctionalTest` subclass | Level-based actor tests |

---

## Trigger Phrases

### Korean
- "테스트 생성", "테스트 만들어줘"
- "유닛 테스트 생성", "테스트 코드 만들어"
- "AMyCharacter 테스트 생성해줘"
- "TakeDamage 테스트 만들어"

### English
- "generate test for X", "create test for X"
- "generate unit test", "create automation test for"
- "test code for AMyCharacter"

### Should NOT Activate
- "자동화 테스트 만들어" → `/ue-automation-test-builder` (interactive cycle)
- "오토메이션 테스트" → `/ue-automation-test-builder`
- "테스트 반복 50번" → `/test-iterate` (edge case iteration)
- "테스트 실행" → `/test-run` (execution, not generation)

---

## 3-Step Workflow

```text
Step 1: Analyze   →   Step 2: Generate   →   Step 3: Write & Verify
(ue_analyze_symbols)  (ue_generate_code)      (Write + optional build)
```

### Step 1: Analyze Target (Optional but Recommended)

Use `ue_analyze_symbols` to get function signatures for better test assertions:

```python
ue_analyze_symbols(operation="get_methods", class_name="<TargetClass>")
```

This returns parameter types and return types, enabling type-specific assertion selection:
- `float` → `TestNearlyEqual` (with tolerance)
- `bool` → `TestTrue` / `TestFalse`
- `int32` → `TestEqual`
- `FVector` → `TestEqual` (UE has overloads)
- `AActor*` → `TestNotNull`

### Step 2: Generate Test Code

```python
ue_generate_code(operation="generate_test", params={
    "test_target": "AMyCharacter::TakeDamage",  # Required
    "test_type": "bdd_spec",                     # Optional (default: bdd_spec)
    "module_name": "MyGame",                     # Optional (default: Game)
    "output_dir": "Source/MyGame/Tests",          # Optional (default: Source/Tests)
    "functions": ["TakeDamage", "GetHealth"]      # Optional (test multiple functions)
})
```

**Smart mode** also works — just provide `test_target`:
```python
ue_generate_code(operation="smart", test_target="AMyCharacter")
# Auto-routes to generate_test
```

### Step 3: Write & Verify

1. Write the generated `.cpp` file to the project
2. Optionally verify compilation:
```python
ue_fix_errors(operation="build_and_fix", project_root="<project>")
```
3. Optionally run the test:
```python
ue_editor_automation(operation="run_automation_tests",
    test_name="MyGame.MyCharacter.TakeDamage")
```

---

## Parameters Reference

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `test_target` | string | Yes | — | Class or function: `"AMyCharacter"` or `"AMyCharacter::TakeDamage"` |
| `test_type` | string | No | `"bdd_spec"` | `simple`, `bdd_spec`, `complex_latent`, `functional` |
| `test_name` | string | No | Auto-generated | Test class name (e.g., `"FMyCharacterSpec"`) |
| `test_flags` | string | No | Auto-detected | UE `EAutomationTestFlags` expression |
| `output_dir` | string | No | `"Source/Tests"` | Output directory for generated files |
| `module_name` | string | No | `"Game"` | Module name for test pretty name path |
| `functions` | array | No | `[]` | Specific functions to generate tests for |

---

## Test Type Selection Guide

```text
Is it a pure C++ value check?           → simple
Need Given/When/Then structure?         → bdd_spec (recommended)
Does it require multiple frames/ticks?  → complex_latent
Does it need a level with actors?       → functional
```

---

## Output Format

The `generate_test` operation returns:

```json
{
  "success": true,
  "operation": "generate_test",
  "test_type": "bdd_spec",
  "test_target": "AMyCharacter::TakeDamage",
  "test_name": "FMyCharacterTakeDamageSpec",
  "cpp_code": "// ... complete test code ...",
  "cpp_path": "Source/Tests/MyCharacterTakeDamageSpec.cpp",
  "test_flags": "EAutomationTestFlags::EditorContext | ...",
  "pretty_name": "MyGame.MyCharacter.TakeDamage",
  "functions_tested": ["TakeDamage"],
  "assertions_generated": 3,
  "warnings": []
}
```

**Key**: Non-functional tests (simple, bdd_spec, complex_latent) produce a **single .cpp file** — UE automation test macros define entire classes inline.

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| Target class not found | Typo or missing PDB index | Use `ue_analyze_symbols(search_symbols)` to verify class exists |
| generate_test returns empty | Unsupported test pattern | Fall back to `derive_class` with test template features |
| Compilation failure after generation | Missing includes or dependencies | Run `ue_fix_errors(smart)` to auto-resolve |
| Editor not running for test execution | Editor required for automation tests | Skip execution step, output code only |
