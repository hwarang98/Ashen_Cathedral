---
name: ue-network-audit
model: opus
description: "Purpose: Network replication audit for UE multiplayer projects. Scans replicated properties, estimates bandwidth, validates DOREPLIFETIME registrations, audits RPCs, and tracks GAS ability bindings. Triggers: 'network audit', 'bandwidth analysis', 'replication check', '네트워크 감사', '대역폭 분석', '리플리케이션 검사', 'RPC audit'."
compatibility: "Requires NarshaMCP MCP server (v0.9.7+) with Unreal Engine 5.x C++ project"
argument-hint: "ClassName (optional), num_players (optional, default 32)"
metadata:
  version: "1.0.0"
  author: "Next-Stage-Inc"
---

# UE Network Audit — Replication Health Analysis

**Version**: 1.0.0
**Issue**: #7234, #7229 (Epic)
**Purpose**: Static source analysis of UE networking: replicated properties, bandwidth estimation, DOREPLIFETIME validation, RPC audit, and GAS ability bindings.

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the 4-phase analysis automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Target**: `$ARGUMENTS`
   > If `$ARGUMENTS` is empty, run project-wide audit (no class filter).
   > If a class name is provided, focus on that specific class.
2. **Execute 4-phase networking audit**
3. **Generate structured report** with bandwidth, warnings, and recommendations

---

## CRITICAL RULES — Call Count Minimization

> **MANDATORY**: This skill MUST complete in **3-5 MCP/tool calls** (Bash excluded).
> Optimal: 3 calls. Maximum: 5 calls.

| # | Rule |
|---|------|
| 1 | **ToolSearch 1x only** — call once at start |
| 2 | **replication_health_report is the primary call** — covers bandwidth + validation + RPC in one report |
| 3 | **track_gas_rpc_bindings only if GAS detected** — skip for non-GAS projects |
| 4 | **No redundant find_replicated_properties** — health_report already includes property scan |

### Target Call Sequences

```text
Optimal (3 calls):                    With GAS (4 calls):
1. ToolSearch (1x)                    1. ToolSearch (1x)
2. replication_health_report          2. replication_health_report
3. analyze_replication_conditions     3. track_gas_rpc_bindings
                                      4. analyze_replication_conditions
```

---

## Auto-Trigger Phrases

### Korean
- "네트워크 감사해줘", "대역폭 분석", "리플리케이션 검사"
- "RPC 감사", "복제 프로퍼티 검증", "네트워크 건강도"
- "멀티플레이어 대역폭", "리플리케이션 검증"

### English
- "Network audit", "Bandwidth analysis", "Replication check"
- "RPC audit", "Replication audit", "Network health"
- "Check replicated properties", "Validate DOREPLIFETIME"

---

## Disambiguation

| Skill | Purpose | When to Use |
|-------|---------|-------------|
| `/ue-network-audit` | **Health overview** (bandwidth estimate, DOREPLIFETIME validation, RPC inventory) | Architecture review, pre-deployment health check |
| `/ue-network-lint` | **Per-line violation scanning** (NET-001~010 rules, like a linter) | Find specific anti-patterns in RPC/replication code |
| `/ue-insights-profiler` | **Runtime profiling** (needs .utrace) | After deployment, FPS drops, live performance issues |

These are **complementary**: audit for big picture health, lint for per-line violations, profile for runtime behavior.

---

## 4-Phase Analysis Workflow

```text
Phase 1: Health Report
   │ Scan all headers for UPROPERTY(Replicated), UFUNCTION(Server/Client/NetMulticast)
   │ Cross-validate DOREPLIFETIME macros, estimate bandwidth
   ▼
Phase 2: GAS RPC Tracking (conditional)
   │ If GameplayAbility subclasses detected → track ability RPC bindings
   │ Validate authority context, NetExecutionPolicy
   ▼
Phase 3: Condition Analysis
   │ Analyze replication conditions usage, suggest optimizations
   ▼
Phase 4: Report Generation
   │ Structured report with severity levels and actionable recommendations
   ▼
[Complete Network Audit Report]
```

### Phase 1: Health Report (PRIMARY CALL)

```python
ToolSearch("select:mcp__narshamcp__ue_analyze_symbols")

# ONE comprehensive health report — covers bandwidth + validation + RPC audit
report = ue_analyze_symbols(operation="replication_health_report", params={
    "class_name": "<ClassName>",   # optional — omit for project-wide
    "num_players": 32,              # default 32
    "project_root": "<auto>"        # auto-detected
})
# Returns: summary, bandwidth_estimate, warnings, rpc_audit, overall_health
```

### Phase 2: GAS RPC Tracking (CONDITIONAL)

> Only execute if Phase 1 report mentions GameplayAbility classes or GAS-related warnings.

```python
gas = ue_analyze_symbols(operation="track_gas_rpc_bindings", params={
    "check_authority": true,
    "class_name": "<AbilityClass>"  # optional filter
})
# Returns: abilities, rpc_bindings, authority_warnings
```

### Phase 3: Condition Analysis

```python
conditions = ue_analyze_symbols(operation="analyze_replication_conditions", params={
    "class_name": "<ClassName>"     # optional
})
# Returns: all_conditions (13 types), per-property condition usage, optimization suggestions
```

### Phase 4: Report Generation

Combine all phase results into structured report.

---

## Output Format

```text
=== Network Replication Audit ===

--- Summary ---
Replicated actors: N
Total replicated properties: M
RPC functions: K (Server: X, Client: Y, Multicast: Z)
Overall health: [GOOD/WARNING/CRITICAL]

--- Bandwidth Estimate (32 players) ---
| Actor Class         | Props | Bytes/Update | Hz   | Bytes/s  | Severity |
|---------------------|-------|--------------|------|----------|----------|
| AMyCharacter        | 5     | 152          | 30   | 4,560    | OK       |
| AProjectile         | 3     | 36           | 100  | 3,600    | OK       |
| TOTAL per player    |       |              |      | 8,160    | OK       |
| TOTAL (32 players)  |       |              |      | 261,120  | WARNING  |

--- Validation Warnings ---
[WARNING] AMyCharacter::ViewRotation — Replicated but no DOREPLIFETIME macro
  Fix: Add DOREPLIFETIME(AMyCharacter, ViewRotation); to GetLifetimeReplicatedProps

[INFO] AMyVehicle::Position — Using COND_None (no condition)
  Suggestion: Consider COND_SkipOwner for ~25% bandwidth savings

--- RPC Audit ---
Server RPCs (reliable): ServerAttack, ServerConfirmActivation
Client RPCs (reliable): ClientPlayHitReaction, ClientNotifyResult
Multicast RPCs (unreliable): MulticastPlayEffect

--- GAS Ability Bindings --- (if applicable)
| Ability Class       | Server RPCs | Client RPCs | NetExecPolicy   |
|---------------------|-------------|-------------|-----------------|
| UGA_Weapon_Fire     | 1           | 1           | ServerInitiated |

--- Optimization Recommendations ---
1. [HIGH] Add DOREPLIFETIME for AMyCharacter::ViewRotation (missing registration)
2. [MEDIUM] Consider COND_OwnerOnly for health-related properties (75% savings)
3. [LOW] MulticastPlayEffect is unreliable — verify acceptable for gameplay
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| No Source directory | Non-C++ project or wrong path | Report: "No C++ source found. Verify project_root." |
| 0 replicated actors found | Pure Blueprint replication or headers not in Source/ | Check for Blueprint-only replication via ue_manage_blueprint |
| GAS tracking returns 0 | No GameplayAbility subclasses | Skip Phase 2, note in report |
| PDB not loaded | First session, no ue_check_health | Suggest: "Run ue_check_health() first for full symbol resolution" |

---

## Evaluation Criteria

### Activation Test Cases

**Positive (5)** — Should activate:
1. "네트워크 감사해줘" -> Activate (project-wide audit)
2. "AMyCharacter bandwidth analysis" -> Activate (class-specific)
3. "Replication check" -> Activate
4. "RPC audit for my project" -> Activate
5. "리플리케이션 검증" -> Activate

**Negative (3)** — Should NOT activate:
1. "왜 느려?" -> Use /ue-insights-profiler (runtime profiling)
2. "네트워크 에러 해결해줘" -> Use /ue-debug (error diagnosis)
3. "멀티플레이어 시스템 만들어줘" -> Use /ue-scaffold (feature creation)

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| DOREPLIFETIME detection rate | 100% |
| GAS RPC binding detection | 90%+ |
| Bandwidth estimate accuracy | ±20% |
| Tool call count | 3-5 (Bash excluded) |
| Response time | <30s for project-wide |
