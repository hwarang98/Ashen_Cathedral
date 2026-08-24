# Asset Modification Wizard - Usage Examples

Complete examples and conversation scenarios for Asset Modification Wizard.

---

## Example 1: Simple Bulk Property Change

**User**: "모든 BP_Enemy 블루프린트에서 MaxHealth를 100으로 바꿔줘"

**Asset Modification Wizard**:

```text
🔍 Stage 1: Target Discovery
Found 3 Blueprints matching "BP_Enemy*"

📊 Stage 2: Impact Analysis
Risk Level: Low ✅ (Safe to modify)

⚡ Stage 3: Strategy Selection
Selected: Remote Control API (50x faster)

🔍 Stage 4: Dry-Run Preview

| Blueprint | Property | Old | New | Status |
|-----------|----------|-----|-----|--------|
| BP_Enemy_Melee | MaxHealth | 50 | 100 | ✅ |
| BP_Enemy_Ranged | MaxHealth | 30 | 100 | ✅ |
| BP_Enemy_Boss | MaxHealth | 500 | 100 | ⚠️ |

⚠️ Warning: BP_Enemy_Boss decrease (500 → 100)

Do you want to proceed? (Yes/No)

[User: Yes]

✅ Stage 5: Execution Complete
Success rate: 100% (3/3)
Time: 3.2 seconds
Backup: /Game/.backup/2025-10-22_143022/
```

---

## Example 2: PCG Parameter Tuning

**User**: "PCG 그래프에서 모든 Seed 값을 1234로 변경해줘"

**Response**:

```text
🔍 Stage 1: Target Discovery
Found 8 PCG graphs

📊 Stage 2: Impact Analysis
Risk Level: Low ✅
- PCG graphs can be safely modified
- No active instances in level

⚡ Stage 3: Strategy Selection
Selected: Remote Control API
Estimated time: 1-2 seconds

🔍 Stage 4: Dry-Run Preview

| PCG Graph | Node | Parameter | Old | New |
|-----------|------|-----------|-----|-----|
| PCG_Forest | Generate | Seed | 42 | 1234 |
| PCG_Rocks | Scatter | Seed | 100 | 1234 |
| PCG_Grass | Distribute | Seed | 777 | 1234 |
... (5 more)

Do you want to proceed? (Yes/No)

[User: Yes]

✅ Stage 5: Execution Complete
Success rate: 100% (8/8)
Time: 1.8 seconds
All PCG graphs updated successfully
```

---

## Example 3: Complex Property Addition

**User**: "BP_Weapon* 블루프린트에 AmmoCapacity 속성 추가해줘 (기본값 30)"

**Response**:

```text
🔍 Stage 1: Target Discovery
Found 7 Blueprints matching "BP_Weapon*"

📊 Stage 2: Impact Analysis
Risk Level: Medium ⚠️
- Adding new property requires Editor for node graph modification
- 7 Blueprints will be modified
- 15 child Blueprints may need updates

⚡ Stage 3: Strategy Selection
Selected: Remote Control API (Editor required for property addition)
- Editor detected: ✅
- Capability check: ✅ Can add properties

🔍 Stage 4: Dry-Run Preview

Proposed Changes:
| Blueprint | Action | Property | Type | Default |
|-----------|--------|----------|------|---------|
| BP_Weapon_Pistol | Add | AmmoCapacity | int32 | 30 |
| BP_Weapon_Rifle | Add | AmmoCapacity | int32 | 30 |
| BP_Weapon_Shotgun | Add | AmmoCapacity | int32 | 30 |
... (4 more)

ℹ️ Note: Child Blueprints will inherit this property

Do you want to proceed? (Yes/No)

[User: Yes]

✅ Stage 5: Execution Complete
Success rate: 100% (7/7)
Time: 4.5 seconds

All Blueprints now have AmmoCapacity property
Child Blueprints can override the default value (30)
```

---

## Example 4: Safe Modification with Impact Check

**User**: "BP_PlayerCharacter 수정해도 안전한지 확인하고 Health 값 바꿔줘"

**Response**:

```text
🔍 Stage 1: Target Discovery
Found 1 Blueprint: BP_PlayerCharacter

📊 Stage 2: Impact Analysis (Comprehensive)

**Direct Dependencies**:
- Referenced by: 23 Blueprints
- Child Blueprints: 3 (BP_PlayerCharacter_Melee, BP_PlayerCharacter_Ranged, BP_PlayerCharacter_Tank)
- Active in Levels: 5 persistent levels

**Modification Impact**:
- Changing Health affects: BP_HealthBar, BP_PlayerHUD, BP_DamageSystem
- Risk Level: **Medium** ⚠️

**Call Graph**:
- Health property used in 12 Blueprint functions
- Referenced from C++: 3 classes (AMyPlayerController, UHealthComponent, UDamageCalculator)

**Recommendation**:
⚠️ Medium-risk modification. Consider:
1. Preview changes carefully
2. Test in safe environment first
3. Verify child Blueprints after modification
4. Check HUD and damage systems

Do you want to continue? (Yes/Cancel/More Info)

[User: Yes]

⚡ Stage 3: Strategy Selection
Selected: Remote Control API

🔍 Stage 4: Dry-Run Preview

| Blueprint | Property | Old | New | Impact |
|-----------|----------|-----|-----|--------|
| BP_PlayerCharacter | Health | 100 | 150 | ⚠️ Referenced by 23 BPs |

⚠️ Warnings:
- 23 Blueprints reference this property
- 3 child Blueprints will inherit the new default
- HUD and damage systems may need adjustment

Do you want to proceed? (Yes/No/Exclude Children)

[User: Yes]

✅ Stage 5: Execution Complete
Modified: BP_PlayerCharacter
Child Blueprints: Updated (inherited new value)
Time: 2.1 seconds

✅ Validation Passed
Backup: /Game/.backup/2025-10-22_150045/

📋 Next Steps:
1. Test player health in gameplay
2. Verify HUD displays correct values
3. Check damage system calculations
```

---

## Example 5: Fallback to .uasset Modification

**User**: "BP_Enemy* 수정해줘 (Editor 없이)"

**Response**:

```text
🔍 Stage 1: Target Discovery
Found 3 Blueprints matching "BP_Enemy*"

📊 Stage 2: Impact Analysis
Risk Level: Low ✅

⚡ Stage 3: Strategy Selection
❌ Editor not detected
Selected: .uasset Modification (Safe, slower)
Estimated time: 3-5 minutes (vs 2-5 seconds with Editor)

ℹ️ Note: For 50-200x speedup, open Unreal Editor

🔍 Stage 4: Dry-Run Preview

| Blueprint | Property | Old | New | Method |
|-----------|----------|-----|-----|--------|
| BP_Enemy_Melee | MaxHealth | 50 | 100 | .uasset |
| BP_Enemy_Ranged | MaxHealth | 30 | 100 | .uasset |
| BP_Enemy_Boss | MaxHealth | 500 | 100 | .uasset |

Do you want to proceed? (Yes/No/Open Editor First)

[User: Yes]

✅ Stage 5: Execution Complete
Method: .uasset Modification
Success rate: 100% (3/3)
Time: 3.8 minutes

All modifications applied successfully
Backup: /Game/.backup/2025-10-22_151234/

💡 Tip: Next time, open Editor for 50-200x faster modifications!
```

---

## 🎓 Tips for Best Results

### Always Start with Dry-Run

✅ **Good**:

```text
User: "BP_Enemy 수정 미리보기 먼저 보여줘"
→ Stage 1-4 only, wait for user approval
```

❌ **Bad**:

```text
User: "BP_Enemy 바로 수정해줘"
→ Skip preview, risky!
```

**Recommendation**: Always request preview first, especially for bulk modifications.

---

### Use Impact Analysis for Safe Modifications

**Low Impact (<5 callers)**:

- Direct modification OK
- Quick turnaround

**Medium Impact (5-20 callers)**:

- Review preview carefully
- Test in safe environment
- Consider child Blueprint impact

**High Impact (>20 callers)**:

- Comprehensive testing required
- Staged rollout recommended
- Communicate with team

---

### Leverage Remote Control API

**Best Practice**:

```text
1. Open Unreal Editor
2. Run Asset Modification Wizard
3. Get 50-200x speedup
4. Real-time validation
```

**If Editor Closes During Modification**:

- Automatic fallback to .uasset modification
- No data loss
- Completes successfully (slower)

---

### Plan for Rollback

**Automatic Backups**:

```text
Location: /Game/.backup/<timestamp>/
Contains: Original .uasset files before modification
Retention: User-controlled (no auto-deletion)
```

**Rollback Command**:

```text
ue_modify_assets(
    operation="rollback",
    backup_path="/Game/.backup/2025-10-22_143022/"
)
```

---

**Note**: All examples use real-world data from MyProject project testing.
