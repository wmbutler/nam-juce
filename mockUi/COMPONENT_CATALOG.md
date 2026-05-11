# mockUi → JUCE component catalog

Living index for [`index.jsx`](./index.jsx). Update the **JUCE mapping** column as you implement pieces.

## How to use this

1. Read the mock in layers (see project skill **nam-juce-mockui-juce**): file header → section comments → JSX.
2. When you implement or audit a piece, fill **JUCE file/class** (and **Parameters** if applicable).
3. Line ranges are approximate; jump to the symbol name in the editor if lines shift.

## Code layout (UI is a clean slate)

- **`Source/NamUi/`** — All plugin UI: root editor `NamUi::NamUiEditor` plus every mockUi widget as you add it. `NamUi/CMakeLists.txt` lists sources; this directory is on the target include path so local headers use `#include "NamUiEditor.h"` from `.cpp` files here, and the shell includes `"NamUi/NamUiEditor.h"`.
- **`PluginEditor.cpp` / `PluginEditor.h`** — Thin `AudioProcessorEditor` shell: fixed size from `NamUiEditor::kDesignWidth` / `kDesignHeight` (mock ~380×720), `resized()` fills the child.
- **`Source/`** (flat) — **DSP and shared non-UI** only (`PluginProcessor`, `NeuralAmpModeler`, `EqProcessor`, etc.). The old editor stack (`NamEditor`, `EqEditor`, `TopBarComponent`, `MyLookAndFeel`, etc.) has been **removed**; reintroduce behaviors via `NamUi` and existing processor APIs.
- **`PresetManager/`** — Still built; the legacy `PresetManagerComponent` is unused until you wire a new preset strip to `PresetManager` (mock uses a different on-disk preset story — align deliberately).
- **Naming:** Keep the `NamUi::` namespace for UI types. New **`NamUiLookAndFeel`** / palette when you add real controls (no old LaF on disk).

## Spec anchors (not components)

| Anchor | Lines (approx.) | Role |
|--------|-----------------|------|
| File header comment block | 1–130 | NAM dirs, browser semantics, presets JSON, persistence, signal flow |
| Color tokens `C` | 138–159 | Palette — mirror in LookAndFeel / palette struct (see **Appearance** below) |
| Font imports comment | 161–166 | Michroma, Share Tech Mono, Rajdhani → BinaryData |
| `NAMPlugin` layout comment | 1198–1229 | Editor size (~380×), row structure, defaults |

## React components

| Component | Lines (approx.) | Purpose / JUCE hint | Port? | JUCE file / class |
|-----------|-----------------|---------------------|-------|-------------------|
| `C` (tokens) | 138–159 | Colours only | n/a | — |
| `LED` | 173–189 | Group header indicator | yes | `NamUi::NamUiLed` |
| `SoftButton` | 198–235 | Preset panel SAVE/OVERWRITE/RENAME/NEW/DELETE | yes | `NamUi::NamUiStandardButton` (`StandardButtonStyle::softPreset`, `setDanger`, `setSoftCompact`) |
| `SourceSelector` | 244–318 | Input/output device dropdown | yes | `NamUi::NamUiSourceSelector` (`attachToAudioDeviceManager` in standalone) |
| `PresetNavButton` | 327–356 | PREV/NEXT in preset strip | yes | `NamUiStandardButton` (`StandardButtonStyle::presetNav`) |
| `ShiftButton` | 361–391 | ◁ ▷ manifest reorder | yes | `NamUiStandardButton` (`StandardButtonStyle::shiftNav`) |
| `NamingSaveButton` | 396–422 | SAVE in naming mode | yes | `NamUiStandardButton` (`presetNav`, label `"SAVE"`) |
| `PresetDisplay` | 436–644 | LCD panel, sweep, soft buttons, naming | yes | TBD |
| `BrowserRow` | 664–708 | All file-browser rows (collections + files + CAPTURE/IR buttons) | yes | `NamUi::NamUiBrowserRow` |
| `ActiveButton` | 717–755 | CAPTURE / IR (CAB) toggle on row | yes | `NamUiStandardButton` (`StandardButtonStyle::browserActivate`, `kBrowserActivateMinWidth`) |
| `NavButton` | 761–787 | ◀ ▶ in browser rows | yes | `NamUiStandardButton` (`StandardButtonStyle::browserNav`) |
| `Meter` | 802–846 | IN/OUT peak ladder | yes | TBD |
| `Fader` | 848–928 | **Removed from design** — mock says do not implement | **no** | — |
| `MuteButton` | 939–969 | Channel mute + LED | yes | `NamUi::NamUiMuteButton` (`ToggleButton`, muted = toggle on) |
| `Knob` | 972–1088 | Rotary controls (normalized 0…1 in mock; map via APVTS) | yes | TBD |
| `KnobGroup` | 1090–1156 | Bordered TONE / NOISE GATE with header LED toggle | yes | TBD |
| `IRRow` | 1158–1196 | **Deprecated** — use `BrowserRow` only | **no** | — |
| `NAMPlugin` | 1230–EOF | Root layout + state — maps to `AudioProcessorEditor` composition | yes | `NamUi::NamUiEditor` + children under `Source/NamUi/` |

## Appearance (light and dark mode)

- **Product requirement:** Ship **both** light and dark themes. The mock’s `C {}` block is the **dark** token set; the file header (around lines 127–129) says light mode should be a **separate token set** swapped at runtime, not one-off hex elsewhere.
- **Development order:** Implement and tune the UI in **dark mode first**, then add the light palette and flip logic once layout and behavior are stable.
- **JUCE:** There is no single built-in “plugin light/dark theme” — you implement it. Typical pattern: a small **palette struct** (dark + light `juce::Colour` values), optional sync with the OS via `juce::Desktop::getInstance().isDarkModeActive()` and `juce::Desktop::DarkModeSettingListener` (see `juce_Desktop.h` in this repo’s JUCE), plus a **manual override** if you want a plugin setting independent of the OS. Custom painting and `LookAndFeel` methods read from the active palette; call `repaint()` / refresh LaF when the theme changes.

## Composition notes (from mock)

- **Browser**: The header describes **five** stacked rows (capture collection, capture file, IR collection, IR file, plus related semantics). The inline layout comment near `NAMPlugin` mentions three rows; treat the **long header block** as authoritative for row count and locking.
- **gear_type with cab**: `metadata.gear_type` values `amp_cab` and `amp_pedal_cab` disable IR navigation and show NAM-style copy (“AMP CAB” / “AMP PEDAL CAB”); state is derived from `loadedCaptureType` in `NAMPlugin`.
- **Parameters**: Mock knobs use 0…1; real ranges live in `NeuralAmpModeler::createParameters` / `NamJUCEAudioProcessor::createParameters` — document IDs in the table below as you wire attachments.

## Parameter / state crosswalk (fill as you go)

| Mock state / knob | Engine parameter ID | Notes |
|-------------------|----------------------|--------|
| | | |

## Changelog

- Code layout: legacy UI removed; all UI in `Source/NamUi/`, shell in `PluginEditor`.
- Appearance: light + dark required; dev prioritizes dark; JUCE notes (palette + Desktop dark-mode hooks).
- Created: seed table from `index.jsx` structure.
