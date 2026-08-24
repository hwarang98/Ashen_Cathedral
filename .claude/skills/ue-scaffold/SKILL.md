---
name: ue-scaffold
description: "Purpose: UE feature scaffolding with architecture design + code generation. Answers 'Build feature X' by analyzing requirements, suggesting architecture, and generating C++ classes with proper UE patterns. Auto-detects feature type (Character, GAS, Weapon, UI, AI). Triggers: 'scaffold', 'create feature', 'build', 'set up', 'make', '만들어줘', '기능 생성', '구현해줘', '스캐폴드', 'new feature', 'implement'."
argument-hint: ["feature description (e.g., 'melee combat character', 'inventory system', 'AI patrol enemy')"]
---

# UE Scaffold — Feature Scaffolding

**Version**: 1.0.0
**Issue**: #4800
**Purpose**: Answer "Build feature X" with architecture design + automated code generation

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the 6-step workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Classify** the user's feature request (Step 1)
2. **Suggest architecture** — dry-run preview before writing (Step 2)
3. **Check conflicts** — verify target files don't already exist (Step 3)
4. **Generate code** using `ue_generate_code(derive_class/scaffold_class)` (Step 4)
5. **Validate** — run dependency_check to catch missing modules (Step 5)
6. **Present** generated files, validation results, and next steps

---

## Auto-Trigger Phrases

### Korean
- "X 기능 만들어줘", "X 캐릭터 만들어줘"
- "X 시스템 구현해줘", "X 스캐폴드"
- "새 기능 추가해줘", "X 클래스 생성"
- "GAS 캐릭터 만들어줘", "인벤토리 만들어줘"

### English
- "Build X feature", "Create X system"
- "Scaffold X", "Set up X"
- "Implement X", "Make a new X"
- "Generate X class", "New character with GAS"

---

## Feature Type Auto-Detection

### Step 1: Classify Feature Request

```python
FEATURE_PATTERNS = {
    "character": {
        "keywords": ["character", "player", "캐릭터", "플레이어", "hero",
                     "pawn", "movement", "이동"],
        "base_class": "ACharacter",
        "components": ["UCharacterMovementComponent", "UCapsuleComponent",
                       "USkeletalMeshComponent"],
        "features": ["movement", "input_setup"]
    },
    "ability_system": {
        "keywords": ["ability", "GAS", "combat", "attack", "어빌리티",
                     "전투", "공격", "spell", "skill"],
        "base_class": "UGameplayAbility",
        "related": ["UGameplayEffect", "UAttributeSet"],
        "features": ["gas_setup", "attribute_set"]
    },
    "weapon_item": {
        "keywords": ["weapon", "item", "무기", "아이템", "pickup",
                     "equipment", "장비", "inventory"],
        "base_class": "AActor",
        "components": ["UStaticMeshComponent", "USphereComponent"],
        "features": ["replication", "interaction"]
    },
    "ui_widget": {
        "keywords": ["UI", "widget", "HUD", "menu", "위젯", "메뉴",
                     "화면", "인터페이스"],
        "base_class": "UUserWidget",
        "features": ["widget_setup"]
    },
    "ai_npc": {
        "keywords": ["AI", "NPC", "enemy", "적", "봇", "patrol",
                     "behavior", "순찰", "state tree"],
        "base_class": "AAIController",
        "related": ["ACharacter (NPC pawn)", "UBehaviorTree/UStateTree"],
        "features": ["ai_setup"]
    },
    "component": {
        "keywords": ["component", "컴포넌트", "system component",
                     "actor component"],
        "base_class": "UActorComponent",
        "features": ["replication", "tick"]
    },
    "game_mode": {
        "keywords": ["game mode", "게임 모드", "match", "round",
                     "game state"],
        "base_class": "AGameModeBase",
        "related": ["AGameStateBase", "APlayerState"],
        "features": ["multiplayer"]
    }
}
```

### Step 2: Architecture Suggestion (Dry-Run)

```python
ToolSearch("select:mcp__narshamcp__ue_generate_code")

# First: Get architecture suggestion (always do this before generating)
# This acts as a dry-run — shows what WILL be generated without writing files
suggestion = ue_generate_code(operation="suggest_class", params={
    "description": "<user's feature description>",
    "project_context": "<relevant project info>"
})
# Returns: recommended class name, base class, components, features, file list

# IMPORTANT: Present the suggestion to user and confirm before proceeding to Step 3.
# Show: class names, base classes, file paths, features, Build.cs dependencies.
```

### Step 3: Pre-Generation Checks

Before generating code, verify no file conflicts:

```python
# Check if target files already exist using Glob
# For each file that suggest_class recommended:
Glob(pattern="Source/**/MyHero.h")    # Check .h
Glob(pattern="Source/**/MyHero.cpp")  # Check .cpp

# If files exist:
#   - WARN user: "MyHero.h already exists at Source/MyGame/Characters/MyHero.h"
#   - Ask: overwrite, rename (MyHero2), or abort
#   - Do NOT silently overwrite existing files
```

### Step 4: Generate Code

```python
# Option A: Derive from existing class (most common)
result = ue_generate_code(operation="derive_class", params={
    "base_class": "ACharacter",
    "class_name": "AMyHero",
    "output_dir": "Source/MyGame/Characters",
    "features": ["gas_setup", "movement", "input_setup"]
})

# Option B: Scaffold entire class structure
result = ue_generate_code(operation="scaffold_class", params={
    "class_name": "AMyWeapon",
    "base_class": "AActor",
    "output_dir": "Source/MyGame/Weapons",
    "features": ["replication", "interaction"]
})
```

### Step 5: Post-Generation Validation

```python
# Verify generated files exist and are non-empty
Glob(pattern="Source/MyGame/Characters/MyHero.*")

# Quick compile check — detect obvious issues before user tries to build
ToolSearch("select:mcp__narshamcp__ue_fix_errors")
validation = ue_fix_errors(mode="dependency_check")
# Catches: missing Build.cs modules, circular deps
```

### Step 6 (Optional): Set Up GAS Bindings

For ability-related features, also configure gameplay:

```python
ToolSearch("select:mcp__narshamcp__ue_manage_gameplay")

# Create input action binding for the ability (requires Editor)
ue_manage_gameplay(operation="create_input_action", params={
    "action_name": "IA_Attack",
    "input_tag": "InputTag.Ability.Attack"
})
```

---

## Output Format

```text
=== Scaffold: [Feature Name] ===

--- Architecture ---
Feature Type: [Character / Ability / Weapon / UI / AI / Component]

Classes to generate:
  1. AMyHero : ACharacter
     +-- Components: MovementComponent, CapsuleComponent
     +-- Features: GAS setup, Enhanced Input
  2. UMyHeroAttributeSet : UAttributeSet
     +-- Attributes: Health, MaxHealth, Stamina
  3. UGA_MyHeroAttack : UGameplayAbility
     +-- Tags: Ability.Attack

--- Pre-Check ---
  File conflicts: None (0 existing files)
  Build.cs modules: 3 to add (GameplayAbilities, GameplayTags, GameplayTasks)

--- Generated Files ---
  [x] Source/MyGame/Characters/MyHero.h (42 lines)
  [x] Source/MyGame/Characters/MyHero.cpp (85 lines)
  [x] Source/MyGame/Attributes/MyHeroAttributeSet.h (28 lines)
  [x] Source/MyGame/Attributes/MyHeroAttributeSet.cpp (35 lines)

--- Post-Validation ---
  Dependency check: PASS (no missing modules after adding Build.cs deps)

--- Configuration ---
Build.cs dependencies to add:
  - "GameplayAbilities"
  - "GameplayTags"
  - "GameplayTasks"

--- Next Steps ---
  1. Add Build.cs module dependencies (shown above)
  2. Create Blueprint derived from AMyHero
  3. Set up Input Mapping Context in Project Settings
  4. Create GameplayEffects for damage/healing
  5. Wire up AnimBP for character animations
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| Feature type unclear | Ambiguous description | Ask user to clarify with feature type options |
| Base class not found | Invalid or engine-only class | Suggest closest available base class |
| File already exists | Target .h/.cpp exists | Warn user, offer: overwrite / rename (append suffix) / abort |
| Code gen fails | Missing output directory or permissions | Report error and suggest manual creation path |
| GAS bindings fail | Project doesn't use GAS | Skip GAS step, report in Next Steps |
| dependency_check finds issues | Missing Build.cs modules | List required modules in Next Steps with exact `PublicDependencyModuleNames` line |

---

## Evaluation Criteria

### Activation Test Cases

**Positive (5)** - Should activate:
1. "근접 전투 캐릭터 만들어줘" -> Activate (character + GAS)
2. "Build an inventory system" -> Activate (weapon/item)
3. "새 AI 적 NPC 구현해줘" -> Activate (AI)
4. "Create a health bar widget" -> Activate (UI)
5. "게임 모드 만들어줘" -> Activate (game mode)

**Negative (3)** - Should NOT activate:
1. "ACharacter 어떻게 동작해?" -> Use ue-explain skill
2. "빌드 에러 해결해줘" -> Use ue-debug skill
3. "BP_Player 변경 영향 분석" -> Use ue-impact skill

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| Feature type detection | >90% |
| Generated code compilability | >85% |
| Architecture suggestion quality | >80% |
| Time to scaffold | <60s |

---

**Status**: Phase 1 MVP
**Related**: Issue #4800 (MCP-First Tool Selection)
