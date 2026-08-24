---
name: ue-plan-review
description: "Purpose: UE architecture-aware implementation plan reviewer. Iteratively scores and improves plans until target quality (default 9.5/10). 6 base categories + optional UE Architecture, GAS Compliance, Replication Safety. Triggers: 'plan review', 'review plan', 'plan score', '플랜 리뷰', '계획 검토', '설계 리뷰', '아키텍처 리뷰'."
argument-hint: ["topic or issue number, --target 9.5, --max-iterations 5"]
---

# UE Plan Review — UE Architecture-Aware Plan Reviewer

**Version**: 1.0.0
**Issue**: #6732
**Purpose**: Iteratively review and improve UE implementation plans until target quality score is reached

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the review workflow automatically**.
> Do NOT just display documentation - actually score the plan and iterate!

### Execution Requirements

1. **Extract topic** from user query or argument (topic string, issue number, or existing plan)
2. **Gather UE project context** using MCP tools
3. **Create or load plan** for the topic
4. **Score iteratively** until target reached or exit condition met
5. **Report final score** with category breakdown

---

## Auto-Trigger Phrases

### Korean
- "플랜 리뷰해줘", "계획 검토해줘"
- "설계 리뷰", "아키텍처 리뷰"
- "구현 계획 점수 매겨줘"
- "이 플랜 괜찮아?", "계획 개선해줘"

### English
- "Review this plan", "Score my plan"
- "Architecture review", "Design review"
- "Is this plan good enough?", "Improve this plan"
- "Rate my implementation plan"

---

## Workflow

### Step 0: Parse Arguments

1. Parse arguments from `$ARGUMENTS`.
   - `$ARGUMENTS[0]` -> topic string or issue number
   - `--skip-plan` -> review existing plan only (skip plan creation)
   - `--target <score>` -> target quality score (default: 9.5)
   - `--max-iterations <N>` -> max review iterations (default: 5)
   - `--no-gas` -> force-disable GAS Compliance category
   - `--no-replication` -> force-disable Replication Safety category

> If `$ARGUMENTS[0]` is empty, gather topic from current branch context or ask user.

### Phase 1: Gather UE Project Context

Collect project-specific context to inform scoring accuracy.

```python
# 1. Check project health and type
ue_check_health()  # -> project_root, engine_version, project_name

# 2. Scan project structure for UE-specific patterns
ue_glob(pattern="BP_*")        # -> Blueprint inventory (scope check)

# 3. Detect GAS/Replication usage in project (unless --no-gas / --no-replication)
ue_grep(query="AbilitySystemComponent", domain="source", limit=3)  # -> GAS detection
ue_grep(query="GetLifetimeReplicatedProps", domain="source", limit=3)  # -> Replication detection
```

**Context Detection Output**:
- `has_gas`: true/false (enables GAS Compliance category — requires BOTH project detection AND topic relevance)
- `has_replication`: true/false (enables Replication Safety category — same dual check)
- `engine_version`: UE version for API compatibility checks

**Topic Relevance Check** (dual-gate activation):
GAS/Replication categories activate only when BOTH conditions are met:
1. **Project has it**: `ue_grep` returns matches (project uses GAS/Replication)
2. **Topic mentions it**: topic string contains relevant keywords:
   - GAS keywords: `GAS`, `Ability`, `ASC`, `GameplayEffect`, `GameplayTag`, `AttributeSet`
   - Replication keywords: `Replicate`, `멀티플레이어`, `Multiplayer`, `RPC`, `NetMulticast`, `Server`, `리플리케이션`

### Phase 2: Create Plan (unless `--skip-plan`)

If no existing plan, create one with UE-aware structure:

```text
## Objective
[What this implementation achieves]

## Files to Modify
[List with full paths, verified via Glob/Grep]

## Module Dependencies
[.Build.cs modules affected, new dependencies needed]

## Implementation Steps
1. [Step with specific file:line references]
2. [UPROPERTY/UFUNCTION declarations needed]
3. [Blueprint integration points]
...

## Test Strategy
- Unit tests (Automation Framework)
- Blueprint functional tests
- Editor integration tests

## Risks and Mitigations
- [Risk]: [Mitigation]
```

**Validation**: Every file path, class name, and function reference in the plan MUST be verified:

```python
# Verify C++ symbols exist
ue_analyze_symbols(operation="search_symbols", params={"query": "SymbolName"})

# Verify file paths exist
ue_glob(pattern="*FileName*", domain="source")

# Verify module dependencies
ue_grep(query="PublicDependencyModuleNames", domain="source", path_pattern="*Build.cs")
```

### Phase 3: Iterative Review Loop

#### Scoring Categories

**Base Categories (always active)**:

| Category | Weight | Evaluates |
|----------|--------|-----------|
| Completeness | 15% | All requirements covered, no missing steps |
| Accuracy | 15% | File paths, symbols, API calls are correct and verified |
| Feasibility | 15% | Steps are practical and achievable in stated order |
| Testing | 15% | Test strategy covers unit, integration, edge cases |
| Risk | 15% | Risks identified with concrete mitigations |
| UE Architecture | 15% | See UE Architecture rubric below |

**Optional Categories (auto-detected from Phase 1)**:

| Category | Weight | Condition | Evaluates |
|----------|--------|-----------|-----------|
| GAS Compliance | 10% | `has_gas == true` | See GAS rubric below |
| Replication Safety | 10% | `has_replication == true` | See Replication rubric below |

**Dynamic Weight Redistribution**:

Formula: `base_weight = (100 - active_optional_count * 10) / 6`

| Active Optional | Base Weight (each) | GAS | Replication | Total |
|-----------------|-------------------|-----|-------------|-------|
| None | 16.7% (100/6) | — | — | 100% |
| GAS only | 15% (90/6) | 10% | — | 100% |
| Replication only | 15% (90/6) | — | 10% | 100% |
| Both | 13.33% (80/6) | 10% | 10% | 100% |

#### UE Architecture Rubric (15%)

| Score | Criteria |
|-------|----------|
| 9-10 | Component-based design, proper UPROPERTY/UFUNCTION specifiers, correct module boundaries, follows Epic coding standards |
| 7-8 | Mostly correct UE patterns, minor specifier issues (e.g., missing BlueprintReadOnly where needed) |
| 5-6 | Functional but non-idiomatic (monolithic classes, missing EditAnywhere/BlueprintCallable where expected) |
| 3-4 | Significant UE anti-patterns (tight coupling, wrong component hierarchy, missing GENERATED_BODY) |
| 1-2 | Fundamentally wrong architecture (no UE patterns, raw pointers instead of UPROPERTY, no module separation) |

**Key checks**:
- UPROPERTY specifiers: `EditAnywhere`, `BlueprintReadWrite`, `Replicated`, `Category`
- UFUNCTION specifiers: `BlueprintCallable`, `BlueprintNativeEvent`, `Server`/`Client`
- Component composition over deep inheritance
- Proper use of `CreateDefaultSubobject` in constructors
- Module dependency direction (no circular deps)

#### GAS Compliance Rubric (10%, when active)

| Score | Criteria |
|-------|----------|
| 9-10 | Correct ASC ownership, proper GA/GE/GC separation, GameplayTag hierarchy, AttributeSet design |
| 7-8 | Working GAS setup, minor issues (e.g., hardcoded tags instead of data-driven) |
| 5-6 | GAS used but with anti-patterns (ability logic in Character, no GameplayCue separation) |
| 3-4 | Significant GAS misuse (no ASC, direct attribute modification, no tag-based activation) |
| 1-2 | GAS mentioned but fundamentally misunderstood |

**Key checks**:
- AbilitySystemComponent ownership (Pawn vs PlayerState)
- GameplayAbility activation/cancellation flow
- GameplayEffect duration types (Instant/Duration/Infinite)
- GameplayTag hierarchy structure (e.g., `Ability.Skill.Fire`)
- AttributeSet with proper `GAMEPLAYATTRIBUTE_*` macros

#### Replication Safety Rubric (10%, when active)

| Score | Criteria |
|-------|----------|
| 9-10 | Correct authority checks, proper RPC usage, minimal bandwidth, RepNotify for client state |
| 7-8 | Working replication, minor issues (e.g., unnecessary replicated properties) |
| 5-6 | Replication works but wasteful (over-replicating, missing conditions like COND_SkipOwner) |
| 3-4 | Authority/client confusion, RPC direction errors, race conditions |
| 1-2 | No consideration of network topology |

**Key checks**:
- `HasAuthority()` checks before state changes
- RPC direction: `Server` (client->server), `Client` (server->client), `NetMulticast`
- `GetLifetimeReplicatedProps` + `DOREPLIFETIME` macros
- Replication conditions (`COND_OwnerOnly`, `COND_SkipOwner`)
- Bandwidth awareness (rep frequency, relevancy)

#### Loop Configuration

| Parameter | Default | Description |
|-----------|---------|-------------|
| Target score | 9.5 | Exit when weighted average >= this |
| Max iterations | 5 | Hard stop after N iterations |
| Timeout | 20 min | Wall-clock time limit |
| Bloat guard | 3x | Exit if plan exceeds 3x initial size |

#### Exit Conditions

The loop exits when **any** condition is met:

1. **Target achieved** — weighted score >= target
2. **Max iterations reached** — hit iteration limit
3. **Stagnation** — same score for 2 consecutive iterations
4. **Diminishing returns** — delta < 0.2 per iteration
5. **Timeout** — 20 minutes elapsed
6. **Score regression** — score decreased (safety check)
7. **Bloat guard** — plan exceeds 3x initial line count

#### Per-Iteration Steps

1. **Score** the plan across all active categories
2. **Identify weakest** category (lowest individual score)
3. **Apply focused improvements** to that category:
   - Verify file paths with `ue_glob`
   - Check symbol existence with `ue_analyze_symbols`
   - Validate module deps with `ue_grep` on `.Build.cs`
   - Add missing test cases
   - Strengthen risk mitigations
4. **Record delta** (score change from prior iteration)
5. **Continue or exit** based on exit conditions

### Phase 4: Final Report

Generate the final review report.

---

## Output Format

```text
=== UE Plan Review Results ===
Topic: {topic_or_issue}
Project: {project_name} (UE {engine_version})
Target: {target_score}/10
Date: {YYYY-MM-DD}

Active Categories: Base 6 [+ GAS Compliance] [+ Replication Safety]

Score Journey: {initial} -> {final} (+{delta}, {N} iterations)
Exit Reason: {target_achieved|max_iterations|stagnation|timeout}

Category Scores:
  Completeness:       {score}/10 (weight%)
  Accuracy:           {score}/10 (weight%)
  Feasibility:        {score}/10 (weight%)
  Testing:            {score}/10 (weight%)
  Risk:               {score}/10 (weight%)
  UE Architecture:    {score}/10 (weight%)
  [GAS Compliance:    {score}/10 (10%)]
  [Replication Safety:{score}/10 (10%)]

Weighted Average: {final_score}/10

Plan: {line_count} lines, {file_count} files to change
Weakest Category: {category_name} ({score}/10)
Top Improvement: {description of biggest score gain}
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| `ue_check_health` fails | MCP server not connected | Skip project context; use base 6 categories only; warn user |
| Score stagnation after 2 iterations | All improvable items require manual judgment | Exit with best score; report weakest category for manual review |
| Plan bloat exceeds 3x initial size | Iterative additions without pruning | Remove redundant steps, merge overlapping items; re-score |
| Referenced file path does not exist | Hallucinated or outdated path | Use `ue_glob` or `Grep` to find correct file; update plan |
| GAS/Replication detection false positive | Keywords found in unrelated code | User can override: `--no-gas` or `--no-replication` flags |
| No plan exists and `--skip-plan` used | Nothing to review | Prompt user to provide plan or remove `--skip-plan` flag |

---

## Evaluation Criteria

### Activation Test Cases

**Positive (4)** - Should activate:
1. "이 구현 계획 리뷰해줘" -> Activate (Korean plan review)
2. "Review my implementation plan" -> Activate (English plan review)
3. "Score this design for UE best practices" -> Activate (architecture scoring)
4. "/ue-plan-review 'Add health component to ACharacter'" -> Activate (explicit invocation)

**Negative (3)** - Should NOT activate:
1. "ACharacter 설명해줘" -> Use ue-explain skill
2. "빌드 에러 고쳐줘" -> Use ue-debug skill
3. "캐릭터 만들어줘" -> Use ue-scaffold skill

### Scenario Validation

| Scenario | Input | Expected | Pass Criteria |
|----------|-------|----------|---------------|
| Easy | "ACharacter에 UHealthComponent 추가" | 1-2 iterations | Score >= 8.0, UE Architecture score present |
| Normal | "인벤토리 시스템 (3 classes)" | 2-3 iterations | Score >= 9.0, UE Architecture evaluated |
| Hard | "GAS 스킬 시스템 (5 classes + GAS)" | 3-4 iterations | Score >= 9.5, GAS Compliance category active |
| Expert | "멀티플레이어 무기 시스템 (Replication)" | 3-5 iterations | Score >= 9.5, Replication Safety category active |

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| Plan quality improvement per iteration | >= +0.3 average |
| Exit within max iterations | 95% of cases |
| False positive GAS/Replication detection | < 5% |
| File path accuracy after review | 100% (all verified) |
| Response time (full review cycle) | < 5 min for Easy, < 15 min for Expert |

---

**Status**: Phase 1 MVP
**Related**: Issue #6732, Reference: `.claude/skills/plan-review/SKILL.md`
