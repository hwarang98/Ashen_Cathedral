---
name: ue-sequencer-cinematics
description: "Purpose: Sequencer cinematics workflow orchestration. Analyze, build, and debug Level Sequence cinematics using 4 sequencer sub-tools (72 ops). Structure analysis, track management, keyframe editing, camera cuts, and playback control. Triggers: 'cinematics', 'sequencer workflow', '시네마틱', '시퀀서 워크플로우', 'camera cut', 'level sequence', '레벨 시퀀스'."
argument-hint: ["sequence name or action (e.g., LS_IntroCinematic, 'add camera cut to LS_Boss')"]
---

# UE Sequencer Cinematics Workflow

**Version**: 1.0.0
**Purpose**: Orchestrate Level Sequence cinematics — from structure analysis to playback debugging

---

## Auto-Execution Instructions

> **CRITICAL**: When this skill is loaded, Claude **MUST execute the workflow automatically**.
> Do NOT just display documentation - actually run the MCP tools!

### Execution Requirements

1. **Extract target** sequence name or action from user query
2. **Detect intent** using the routing matrix below
3. **Execute MCP tools** across 4 sequencer sub-tools
4. **Generate structured report** with Mermaid timeline visualization

---

## Auto-Trigger Phrases

### Korean
- "시네마틱 분석", "시퀀서 워크플로우"
- "레벨 시퀀스 구조", "카메라 컷 추가"
- "시퀀서 키프레임", "시퀀서 재생"
- "시네마틱 만들어줘", "컷신 분석"

### English
- "Cinematics analysis", "Sequencer workflow"
- "Level sequence structure", "Add camera cut"
- "Sequencer keyframes", "Playback control"
- "Build a cinematic", "Cutscene analysis"

---

## Intent Auto-Routing Matrix

| Intent | Primary Tool | Key Operations |
|--------|-------------|----------------|
| Analyze structure | `ue_sequencer_structure` | `get_structure`, `list_bindings`, `get_shot_structure` |
| Find sequences | `ue_sequencer_structure` | `list_sequences`, `search_sequences` |
| Manage tracks | `ue_sequencer_tracks` | `list_tracks`, `add_track`, `compare_sequences` |
| Edit keyframes | `ue_sequencer_keyframes` | `get_keyframes`, `add_keyframe`, `get_curve_data` |
| Camera work | `ue_sequencer_playback` | `add_camera_cut`, `set_camera_binding`, `get_active_camera` |
| Playback control | `ue_sequencer_playback` | `play`, `stop`, `scrub`, `get_playback_status` |
| Add actors | `ue_sequencer_playback` | `add_spawnable`, `add_property_track` |
| Audit quality | `ue_sequencer_tracks` | `sequence_audit`, `validate_settings` |

---

## Workflow

### Step 1: Discover Sequences

```python
ToolSearch("select:mcp__narshamcp__ue_sequencer_structure")

# List all Level Sequences in the project
sequences = ue_sequencer_structure(operation="list_sequences")

# Or search by name pattern
sequences = ue_sequencer_structure(operation="search_sequences", params={
    "query": "LS_Intro*"
})
```

### Step 2: Analyze Structure

```python
# Get full structure (tracks, bindings, sub-sequences)
structure = ue_sequencer_structure(operation="get_structure", params={
    "sequence_name": "LS_IntroCinematic"
})

# Get hierarchical shot breakdown (for multi-shot cinematics)
shots = ue_sequencer_structure(operation="get_shot_structure", params={
    "sequence_name": "LS_IntroCinematic",
    "max_depth": 3
})

# List actor bindings
bindings = ue_sequencer_structure(operation="list_bindings", params={
    "sequence_name": "LS_IntroCinematic"
})
```

### Step 3: Inspect Tracks & Keyframes

```python
ToolSearch("select:mcp__narshamcp__ue_sequencer_tracks")
ToolSearch("select:mcp__narshamcp__ue_sequencer_keyframes")

# List all tracks
tracks = ue_sequencer_tracks(operation="list_tracks", params={
    "sequence_name": "LS_IntroCinematic"
})

# Get keyframes for a specific track
keyframes = ue_sequencer_keyframes(operation="get_keyframes", params={
    "sequence_name": "LS_IntroCinematic",
    "track_id": "<track_id from list_tracks>"
})

# Get curve data with tangents
curves = ue_sequencer_keyframes(operation="get_curve_data", params={
    "sequence_name": "LS_IntroCinematic",
    "track_id": "<track_id>"
})
```

### Step 4: Camera & Playback (Editor Required)

```python
ToolSearch("select:mcp__narshamcp__ue_sequencer_playback")

# Add camera cut track
ue_sequencer_playback(operation="add_camera_cut", params={
    "sequence_name": "LS_IntroCinematic",
    "camera_name": "CineCamera_Main",
    "start_frame": 0,
    "end_frame": 120
})

# Set active camera for a cut
ue_sequencer_playback(operation="set_camera_binding", params={
    "sequence_name": "LS_IntroCinematic",
    "camera_name": "CineCamera_CloseUp"
})

# Preview playback
ue_sequencer_playback(operation="play", params={
    "sequence_name": "LS_IntroCinematic"
})

# Get playback status
status = ue_sequencer_playback(operation="get_playback_status", params={
    "sequence_name": "LS_IntroCinematic"
})
```

### Step 5: Quality Audit

```python
# Run sequence audit (checks naming, empty tracks, orphan bindings)
audit = ue_sequencer_tracks(operation="sequence_audit", params={
    "sequence_name": "LS_IntroCinematic"
})

# Validate settings (frame rate, resolution, etc.)
validation = ue_sequencer_structure(operation="validate_settings", params={
    "sequence_name": "LS_IntroCinematic"
})
```

---

## Output Format

```
=== Sequencer Cinematics: [Sequence Name] ===

--- Overview ---
Sequence: LS_IntroCinematic
Duration: 10.0s (300 frames @ 30fps)
Tracks: 12 | Bindings: 5 | Sub-sequences: 2

--- Shot Structure ---
LS_IntroCinematic
+-- Shot_01 (0-90f) — CineCamera_Wide
+-- Shot_02 (90-180f) — CineCamera_CloseUp
+-- Shot_03 (180-300f) — CineCamera_Dolly

--- Bindings ---
  1. BP_MainCharacter (Possessable)
  2. CineCamera_Wide (Spawnable)
  3. CineCamera_CloseUp (Spawnable)
  4. DirectionalLight (Possessable)
  5. SK_Environment (Possessable)

--- Tracks Summary ---
  Transform: 3 tracks (156 keyframes)
  Camera: 2 tracks (3 camera cuts)
  Animation: 2 tracks (4 sections)
  Event: 1 track (5 events)
  Material: 1 track (8 keyframes)

--- Audit ---
  Issues: 2
  - WARNING: Track "OldTrack" has 0 keyframes (empty)
  - WARNING: Binding "LightRig" has no tracks

--- Mermaid Timeline ---
gantt
    title LS_IntroCinematic
    dateFormat X
    axisFormat %s

    section Cameras
    CineCamera_Wide    :0, 90
    CineCamera_CloseUp :90, 180
    CineCamera_Dolly   :180, 300

    section Animation
    Walk Montage       :0, 120
    Idle Montage       :120, 300
```

---

## Error Recovery

| Error | Cause | Fallback |
|-------|-------|----------|
| Sequence not found | Wrong name or not in project | `ue_sequencer_structure(list_sequences)` to find available sequences |
| Editor not connected | Playback/camera ops require live Editor | Skip Step 4, report offline limitation |
| Track ID invalid | Track removed or renamed | Re-query `list_tracks` to get current IDs |
| get_shot_structure max_depth | Capped at 5 for safety | Use default depth 3 for most projects |
| Binding state error | Actor not in level | Use `add_spawnable` to create spawnable actor |
| Empty audit result | No issues found | Report "Clean — no issues detected" |

---

## Evaluation Criteria

### Activation Test Cases

**Positive (5)** - Should activate:
1. "LS_IntroCinematic 구조 분석해줘" -> Activate (structure analysis)
2. "Add a camera cut to the boss cinematic" -> Activate (camera work)
3. "시퀀서 키프레임 확인" -> Activate (keyframe inspection)
4. "Compare LS_Intro and LS_Ending sequences" -> Activate (comparison)
5. "시네마틱 오디트" -> Activate (quality audit)

**Negative (3)** - Should NOT activate:
1. "ACharacter 어떻게 동작해?" -> Use ue-explain skill
2. "애니메이션 BP 분석" -> Use animation-workflow skill
3. "Niagara VFX 시퀀서에 추가" -> Use ue_manage_niagara directly

### Quantitative Metrics

| Metric | Target |
|--------|--------|
| Intent detection accuracy | >90% |
| Structure analysis completeness | >95% |
| Response time (single sequence) | <30s |
| Mermaid diagram validity | 100% |

---

**Status**: Phase 1 MVP
**Related**: Issue #6047 (MCP Skill Builder)
