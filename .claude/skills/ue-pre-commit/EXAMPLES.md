# UE Pre-Commit Impact Analysis -- Examples

## Example 1: Simple Function Body Change (LOW Risk)

**User**: "커밋해도 돼?"

**Staged files** (1 .cpp):
```text
Source/MyGame/Characters/MyCharacter.cpp
```

### Phase 1: VCS Scan
```bash
$ git diff --cached --name-only
Source/MyGame/Characters/MyCharacter.cpp
```
Result: 1 C++ file (.cpp only, no headers)

### Phase 2: Function Extraction
```bash
$ git diff --cached -U0
@@ -142,3 +142,5 @@ void AMyCharacter::Tick(float DeltaTime)
+    // Added velocity logging
+    UE_LOG(LogTemp, Log, TEXT("Velocity: %s"), *GetVelocity().ToString());
```
Extracted: `AMyCharacter::Tick` — function_body change (MEDIUM)

### Phase 3: Impact Analysis
```python
ue_analyze_symbols(operation="impact_analysis", params={"target": "AMyCharacter", "depth": 2})
# Result: 2 affected BPs (BP_PlayerCharacter, BP_EnemyBase), risk_level: "low"

ue_fix_errors(operation="hotreload_check", params={"auto_detect_git": true, "staged": true})
# Result: hot_reload_safe: true (no header changes)
```

### Phase 4: Skipped (LOW risk, no HIGH targets)

### Phase 5: Report
```text
=== Pre-Commit Impact Report ===

--- Scope ---
Mode: Staged files (git diff --cached)
Changed C++ files: 1 (.h: 0, .cpp: 1)
Targets analyzed: 1 (classes: 1, functions: 0)

--- Risk Summary ---
Overall Risk: LOW
Hot Reload Safe: YES

--- Changed Files ---
  [M] Source/MyGame/Characters/MyCharacter.cpp (function_body)

--- Impact by Target ---

1. AMyCharacter (class, MEDIUM - function body only)
   Affected BPs: 2
   - BP_PlayerCharacter (hop 1)
   - BP_EnemyBase (hop 1)

--- Hot Reload Safety ---
Status: SAFE (no header changes)

--- Verdict ---
SAFE TO COMMIT: 2 BPs affected (body-only change), hot reload safe.
```

---

## Example 2: Header Signature Change (HIGH Risk)

**User**: "Pre-commit check"

**Staged files** (2 files):
```text
Source/MyGame/Combat/DamageSystem.h
Source/MyGame/Combat/DamageSystem.cpp
```

### Phase 1: VCS Scan
```bash
$ git diff --cached --name-only
Source/MyGame/Combat/DamageSystem.h
Source/MyGame/Combat/DamageSystem.cpp
```
Result: 2 C++ files (1 header + 1 source)

### Phase 2: Function Extraction
```bash
$ git diff --cached -U0
diff --git a/Source/MyGame/Combat/DamageSystem.h b/Source/MyGame/Combat/DamageSystem.h
@@ -45,1 +45,1 @@ class MYGAME_API UDamageSystem : public UActorComponent
-    UFUNCTION(BlueprintCallable)
-    float CalculateDamage(AActor* Target, float BaseDamage);
+    UFUNCTION(BlueprintCallable)
+    float CalculateDamage(AActor* Target, float BaseDamage, EDamageType DamageType);

@@ -52,0 +53,3 @@
+    UPROPERTY(EditAnywhere, BlueprintReadWrite)
+    float DamageMultiplier = 1.0f;
```
Extracted:
- `UDamageSystem::CalculateDamage` — signature_change (HIGH): added parameter
- `UDamageSystem.DamageMultiplier` — property_change (HIGH): new UPROPERTY

### Phase 3: Impact Analysis
```python
ue_analyze_symbols(operation="impact_analysis", params={"target": "UDamageSystem", "depth": 2})
# Result: 8 affected BPs, risk_level: "medium"
# affected: BP_MeleeWeapon, BP_RangedWeapon, BP_ExplosiveBarrel, BP_Turret,
#           BP_EnemyBase, BP_BossEnemy, BP_DamageZone, BP_PlayerController

ue_fix_errors(operation="hotreload_check", params={"auto_detect_git": true, "staged": true})
# Result: hot_reload_safe: false
# issues: ["DamageSystem.h: function signature changed", "DamageSystem.h: UPROPERTY added"]
```

### Phase 4: Cross-Reference Extension (HIGH risk triggers)
```python
# PRIMARY: Use blueprint_impact from Phase 3's impact_analysis response
# Phase 3 already returned blueprint_impact.direct_bp_callers[] for UDamageSystem
# Result: 5 BP callers from impact_analysis

# FALLBACK: If blueprint_impact not available, use find_cpp_to_bp directly
ue_analyze_symbols(operation="find_cpp_to_bp", params={
    "function_name": "CalculateDamage", "limit": 30
})
# Result: 5 BP callers
# - BP_MeleeWeapon -> EventGraph::CallFunction_12
# - BP_RangedWeapon -> EventGraph::CallFunction_8
# - BP_ExplosiveBarrel -> OnOverlap::CallFunction_3
# - BP_Turret -> FireWeapon::CallFunction_22
# - BP_DamageZone -> ApplyDamage::CallFunction_7
```

### Phase 5: Report
```text
=== Pre-Commit Impact Report ===

--- Scope ---
Mode: Staged files (git diff --cached)
Changed C++ files: 2 (.h: 1, .cpp: 1)
Targets analyzed: 2 (classes: 1, functions: 1)

--- Risk Summary ---
Overall Risk: HIGH
Hot Reload Safe: NO (2 issues)

--- Changed Files ---
  [H] Source/MyGame/Combat/DamageSystem.h (signature_change + property_change)
  [M] Source/MyGame/Combat/DamageSystem.cpp (function_body)

--- Impact by Target ---

1. UDamageSystem::CalculateDamage (function, HIGH - signature changed)
   Added parameter: EDamageType DamageType
   BP Callers: 5 (ALL WILL BREAK)
   - BP_MeleeWeapon -> EventGraph::CallFunction_12
   - BP_RangedWeapon -> EventGraph::CallFunction_8
   - BP_ExplosiveBarrel -> OnOverlap::CallFunction_3
   - BP_Turret -> FireWeapon::CallFunction_22
   - BP_DamageZone -> ApplyDamage::CallFunction_7

2. UDamageSystem.DamageMultiplier (property, HIGH - new UPROPERTY)
   Affected BPs: 8 (all UDamageSystem users)

--- Hot Reload Safety ---
Status: NOT SAFE (2 issues)
  1. [DamageSystem.h] Function signature changed -> BP nodes will have missing pin
  2. [DamageSystem.h] UPROPERTY added -> serialized layout changed

--- Recommendations ---
  1. [CRITICAL] All 5 BP callers of CalculateDamage will break (missing parameter pin)
  2. [CRITICAL] Full rebuild required (hot reload not safe)
  3. [HIGH] After rebuild, open and resave all 5 BP callers to fix broken nodes
  4. [MEDIUM] Consider adding default value for DamageType to preserve BP compatibility

--- Verdict ---
BLOCKED: 8 BPs affected, 5 BP nodes will break, hot reload not safe.
Recommend: Add default parameter value, full rebuild, resave affected BPs.
```

---

## Example 3: Branch Refactor (`--branch` mode)

**User**: "/ue-pre-commit --branch"

**Branch changes** (12 C++ files on `feat/weapon-refactor`):
```text
Source/MyGame/Weapons/WeaponBase.h
Source/MyGame/Weapons/WeaponBase.cpp
Source/MyGame/Weapons/MeleeWeapon.h
Source/MyGame/Weapons/MeleeWeapon.cpp
Source/MyGame/Weapons/RangedWeapon.h
Source/MyGame/Weapons/RangedWeapon.cpp
Source/MyGame/Weapons/ProjectileBase.h
Source/MyGame/Weapons/ProjectileBase.cpp
Source/MyGame/Weapons/DamageCalculator.h
Source/MyGame/Weapons/DamageCalculator.cpp
Source/MyGame/Inventory/InventoryComponent.h
Source/MyGame/Inventory/InventoryComponent.cpp
```

### Phase 2: Batching (>10 targets -> class-level only)
Deduplicated classes: `AWeaponBase`, `AMeleeWeapon`, `ARangedWeapon`, `AProjectileBase`, `UDamageCalculator`, `UInventoryComponent` (6 classes)

### Phase 3: Impact Analysis (6 MCP calls)
Run `impact_analysis` per class. Results aggregated:
- Total affected BPs: 18 (deduplicated)
- Hot reload: NOT SAFE (header changes)

### Phase 5: Report (abbreviated)
```text
=== Pre-Commit Impact Report ===

--- Scope ---
Mode: Branch changes (git diff main...HEAD)
Changed C++ files: 12 (.h: 6, .cpp: 6)
Targets analyzed: 6 classes (batched from 12 files)

--- Risk Summary ---
Overall Risk: HIGH
Hot Reload Safe: NO
Total Affected BPs: 18

--- Impact by Target ---
1. AWeaponBase (6 BPs affected)
2. AMeleeWeapon (3 BPs affected)
3. ARangedWeapon (4 BPs affected)
4. AProjectileBase (2 BPs affected)
5. UDamageCalculator (5 BPs affected)
6. UInventoryComponent (3 BPs affected)

--- Verdict ---
PROCEED WITH CAUTION: Large refactor affecting 18 BPs across 6 classes.
Recommend: Full rebuild + comprehensive BP testing before merge.
```

---

## Example 4: No C++ Changes (Short-Circuit)

**User**: "Is it safe to commit?"

**Staged files**:
```text
Config/DefaultGame.ini
Content/Maps/MainMenu.umap
```

### Phase 1: VCS Scan
```bash
$ git diff --cached --name-only
Config/DefaultGame.ini
Content/Maps/MainMenu.umap
```
Result: 0 C++ files -> SHORT-CIRCUIT

### Report
```text
=== Pre-Commit Impact Report ===

--- Scope ---
Mode: Staged files (git diff --cached)
Changed files: 2 (C++: 0)

--- Verdict ---
SAFE TO COMMIT: No C++ files changed. No BP/asset impact from code changes.
Note: Config and asset changes may still have runtime effects.
```
