---
name: nam-juce-mockui-juce
description: >-
  Implements or ports the Neural Amp Modeler JUCE UI from mockUi/index.jsx with
  correct NAM semantics, APVTS ranges, and modular components. Use when editing
  nam-juce GUI, mockUi, NamUiEditor, preset/browser flows, capture vs IR vs full_rig,
  or converting React layout to JUCE FlexBox/resized(). On Apple Silicon, build the
  ARM binary explicitly for audio testing — Debug builds fail audio reproduction here.
---

# nam-juce: mockUi → JUCE

## Read mockUi in three layers

1. **File header (~lines 1–130)** in `mockUi/index.jsx`: authoritative rules — `~/NAM/` layout, preset JSON, persistence layers, signal flow, **full rig IR locking**, capture-type detection order, color tokens (`C`).
2. **Per-component comments**: intended JUCE counterpart (LED, SoftButton, knobs, browser rows). Prefer over inferring from CSS alone.
3. **JSX implementation**: flex layout and React state (`isFullRig`, `namDirSet`, group actives). Maps to `resized()` + component state, not to DSP unless stated.

## Build and test (Apple Silicon)

- **Build the ARM binary explicitly** for manual audio testing (e.g. Release or the artefact you use on arm64). **Debug builds fail for audio reproduction** in this project — do not rely on Debug standalone/plugin runs to validate sound or real-time behavior.

## JUCE API discipline

- Confirm APIs against **current JUCE module docs** (this repo vendors JUCE under `Modules/JUCE/`). Do not assume older API names or behaviors.
- Follow processor/APVTS patterns in `Source/PluginProcessor.cpp`. All UI code lives under `Source/NamUi/` (root: `NamUiEditor`).

## NAM semantics

| Term | Meaning |
|------|--------|
| **Capture** | `.nam` model under `Captures/` |
| **IR** | `.wav` cab impulse under `IR/` |
| **amp_head** | IR slot is meaningful; separate cab IR expected |
| **full_rig** | Cab baked into model — UI locks IR rows (“CAB BAKED IN”); preset IR fields ignored |

**Spec vs code:** `full_rig` / sidecar detection are fully spelled out in the mock header; **verify** processor/editor actually implements them before relying on C++ behavior — extend backend if the mock is the target.

## Layout: flex → JUCE

- Mock uses **flex** (`flexDirection`, `gap`, `justifyContent`). In JUCE use **`FlexBox`** or **`Grid`** inside `resized()`, or derive `Rectangle`s from a fixed **design width** (mock ~380px) with centralized layout constants.
- Treat absolute pixel hints in mock as **anchors** for `setBounds` math when not using FlexBox for a region.

## Parameters: mock vs engine

Do **not** map mock `0…1` knobs to parameters without checking ranges. Source of truth: `NeuralAmpModeler::createParameters` and `NamJUCEAudioProcessor::createParameters`.

| ID | Range | Notes |
|----|--------|------|
| `INPUT_ID` | −20 … +20 dB | default 0 |
| `OUTPUT_ID` | −40 … +40 dB | default 0 |
| `BASS_ID` / `MIDDLE_ID` / `TREBLE_ID` | 0 … 10 | default 5 |
| `NGATE_ID` | −101 … 0 dB | at ≤ −101 gate treated off in `updateParameters` |
| `CAB_ON_ID` | bool | IR bypass flag |
| EQ / cuts / doubler | see processor | `LOWCUT_ID`, `HIGHCUT_ID`, ten-band EQ params |

**Known spec gap:** mock uses **two** gate knobs (`gate` / `gateHigh`); engine exposes **one** threshold (`NGATE_ID`). Resolve explicitly (extend APVTS + trigger vs simplify UI) before implementing dual knobs.

## Modular UI and states

- Build reusable pieces (knob group + header LED, soft preset buttons, browser row, meters) under `Source/NamUi/`.
- Respect **disabled** (no interaction, dimmed), **active** (group on, green/amber tokens), **armed** (SAVE/DELETE confirm, red, timers). Centralize colors from mock `C` in `LookAndFeel` or a small palette struct — no scattered hex literals.

## Appearance (light / dark)

- **Requirement:** **Light and dark** themes; mock `C` is dark-first; light = second token set (see mock header ~127–129).
- **Development:** Build and polish **dark mode first**, then add the light palette and switching.
- **JUCE:** No automatic plugin theme — use dual palettes + custom paint/LookAndFeel. Optionally follow OS appearance with `juce::Desktop::getInstance().isDarkModeActive()` and `DarkModeSettingListener`; allow a plugin preference if users should override the OS.

## Delegation

- **Skill** (this file): stable rules and file anchors.
- **Subagents**: parallel codebase exploration or large refactors; feed them paths above and require alignment with APVTS + mock header.

## Quick anchors

- Mock spec: `mockUi/index.jsx`
- Component inventory (line anchors, port yes/no, JUCE column to fill): `mockUi/COMPONENT_CATALOG.md`
- **UI code:** `Source/NamUi/` (e.g. `NamUiEditor`); register `.cpp` in `Source/NamUi/CMakeLists.txt`.
- Editor shell: `Source/PluginEditor.cpp`, `Source/PluginEditor.h`
- DSP + params: `Source/NeuralAmpModeler.cpp`, `Source/PluginProcessor.cpp`
- NAM core: `Modules/NeuralAmpModelerCore/NAM/`
