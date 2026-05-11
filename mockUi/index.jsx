/**
 * NAM PLUGIN — UI CONCEPT
 * ========================
 * This file is a React/JSX prototype of the Neural Amp Modeler (NAM) plugin UI.
 * It is intended as a faithful visual specification for a JUCE C++ implementation.
 *
 * FRAMEWORK: Fork of sdatkinson/NeuralAmpModelerPlugin (iPlug2) or nam-juce (JUCE/CMake)
 * RECOMMENDED BASE: nam-juce — https://github.com/Tr3m/nam-juce
 * TARGET PLATFORMS: macOS (AU/VST3), Windows (VST3)
 * FONTS: Michroma (primary UI), Share Tech Mono (values/readouts), Rajdhani (body)
 *   → In JUCE, embed these as BinaryData and load via juce::CustomTypeface
 *
 * DIRECTORY STRUCTURE (set via File > Settings > Set NAM Directory):
 *   ~/NAM/
 *     Captures/   ← .nam files, organized in sub-folders by manufacturer/collection
 *     IR/         ← .wav impulse response files, organized in sub-folders by cabinet/brand
 *     Presets/    ← manifest.json + [id].json preset files
 *   On first launch (or if no directory is set), prompt via juce::FileChooser.
 *   If user cancels, default to ~/NAM/. Create all 3 subdirs automatically.
 *   Store chosen root path in juce::PropertiesFile for persistence across sessions.
 *
 * BROWSER ROWS (5 total, stacked):
 *   Row 1 — Capture Collection: scans ~/NAM/Captures/ for subdirectories
 *   Row 2 — Capture file: scans current collection folder for .nam files
 *            CAPTURE button: first press = load + activate. Subsequent = toggle bypass.
 *            On load, parse the .nam JSON and read metadata.gear_type.
 *   Row 3 — IR Collection: scans ~/NAM/IR/ for subdirectories
 *   Row 4 — IR file: scans current IR collection folder for .wav files
 *            IR (CAB) button: first press = load + activate. Subsequent = toggle bypass.
 *   GEAR TYPE LOCKING: When metadata.gear_type is amp_cab or amp_pedal_cab,
 *            rows 3 and 4 are both disabled simultaneously. Navigation arrows hidden,
 *            text shows the NAM gear type (e.g. "AMP CAB" or "AMP PEDAL CAB"),
 *            IR (CAB) button absent. Unlocks for gear types without a cab.
 *   NAM DIR NOT SET: All rows show "File › Settings → Set NAM Directory" centered,
 *            arrows and buttons hidden, until directory is configured.
 *
 * GEAR TYPE DETECTION:
 *   1. Parse the .nam file directly as JSON and read metadata.gear_type.
 *   2. amp_cab and amp_pedal_cab include a cab, so IR rows lock.
 *   3. amp, pedal_amp, preamp, pedal, and studio leave IR rows available.
 *   4. If gear_type is missing or unknown, leave IR rows available.
 *
 * PRESET FILE FORMAT (~/NAM/Presets/):
 *   manifest.json — ordered list of presets, controls PREV/NEXT order:
 *   [
 *     { "id": "a3f8c2d1", "name": "Marshall Plexi Hot" },
 *     { "id": "b7e4a9f2", "name": "BE-100 Crunch Channel" }
 *   ]
 *
 *   Individual preset files named by ID hash (e.g. a3f8c2d1.json):
 *   {
 *     "id": "a3f8c2d1",
 *     "name": "Marshall Plexi Hot",
 *     "capture": "Captures/Friedman Amplification/BE-100 Crunch Channel.nam",
 *     "ir": "IR/Celestion Cabinets/V30 4x12 — SM57.wav",
 *     "ir_collection": "Celestion Cabinets",
 *     "bass": 0.55, "mid": 0.45, "treble": 0.6,
 *     "gate_open": 0.0, "gate_close": 1.0,
 *     "input_level": 0.5, "output_level": 0.5,
 *     "tone_active": true, "gate_active": false,
 *     "ir_active": true, "capture_active": true
 *   }
 *   Note: presets do not store our own capture type. On load, the .nam file's
 *   metadata.gear_type determines whether ir and ir_collection should be ignored.
 *
 *   ID generation: short UUID or timestamp-based hash at creation time.
 *   Use juce::Uuid().toDashedString().substring(0,8) or similar.
 *   Paths stored RELATIVE to NAM root — never absolute.
 *   Resolve: juce::File(namRoot).getChildFile(relativePath).
 *
 *   Preset operations:
 *   SAVE NEW  → generate new ID, write [id].json, append to manifest.json
 *   SAVE/WRITE → overwrite [id].json only, manifest order unchanged
 *   RENAME    → update "name" in [id].json and matching entry in manifest.json
 *   DELETE    → delete [id].json, remove entry from manifest.json
 *   ◁ ▷       → reorder entries in manifest.json array only, no file changes
 *   PREV/NEXT → navigate by manifest array order, loads full preset state
 *   If manifest entry points to missing file → skip gracefully, log warning
 *   If manifest.json missing → scan dir for *.json (excluding manifest), sort A-Z as fallback
 *
 * STATE PERSISTENCE LAYERS (three separate systems):
 *   1. juce::PropertiesFile — persists across ALL sessions, independent of DAW:
 *      - NAM root directory path (absolute)
 *      - Last loaded preset name
 *      - Last collection/capture/IR indices
 *      - Input/output enabled states
 *      Stored at: ~/Library/Application Support/[Company]/[Plugin]/settings.plist (macOS)
 *                 %APPDATA%\[Company]\[Plugin]\settings.xml (Windows)
 *
 *   2. getStateInformation / setStateInformation — DAW session recall:
 *      - All parameter values + current preset name + current file paths
 *      - Called by DAW on session save/load. Blob stored inside DAW project file.
 *      - Use relative paths here too for portability.
 *
 *   3. ~/NAM/Presets/*.json — named preset library:
 *      - User-created presets, persistent and DAW-independent
 *      - Shareable — users can copy .json files to share sounds
 *      - Always use relative paths from NAM root
 *
 * TONE3000 INTEGRATION (future):
 *   API: https://api.tone3000.com — REST, OAuth 2.0 PKCE
 *   Auth flow: Use juce::WebBrowserComponent (embedded WebView) for OAuth
 *   Deep link redirect: register custom URL scheme e.g. nam://callback
 *   Captures = "Captures" in Tone3000 terminology (.nam files)
 *   IRs = "Impulse Responses" in Tone3000 terminology (.wav files)
 *   Captures whose metadata.gear_type includes a cab should bypass/lock the IR slot
 *   Amp Head captures require a separate IR for the cab simulation
 *
 * PRESET SYSTEM:
 *   A preset stores: capture path, IR path, tone stack values (bass/mid/treble),
 *   noise gate values (low/high), input level, output level, capture active,
 *   IR active, tone active, gate active.
 *   Presets saved as JSON files in ~/NAM/ root (not in Captures/ or IR/).
 *   SAVE: first click arms OVERWRITE (red, 5s timeout), second click confirms.
 *   DELETE: first click arms (2s timeout), second click confirms.
 *   PREV/NEXT: sweep animation left/right through preset list.
 *
 * SIGNAL FLOW (left to right):
 *   Input Source → [Input Level Knob] → [Noise Gate] → [NAM Capture Model]
 *   → [Tone Stack EQ] → [IR/Cab] → [Output Level Knob] → Output Source
 *   Muting input zeros the input meter and disables the input source selector.
 *   Muting output zeros the output meter and disables the output source selector.
 *
 * LIGHT MODE: All color tokens are in the C{} object below.
 *   Light mode should be a separate token set swapped at runtime.
 *   Do not hardcode colors anywhere outside C{}.
 */

import { useState, useEffect, useRef } from "react";

// ── Color tokens ──────────────────────────────────────────────────────────────
// JUCE: Map these to juce::Colour values in a LookAndFeel subclass.
// All colors reference these tokens — never hardcode elsewhere.
// Light mode = swap this object. Dark mode is the default.
const C = {
  bg:          "#0e0e10",   // Plugin background
  bgDeep:      "#080809",   // Unused — reserved for future deeper panels
  bgPanel:     "#111114",   // Panel/section backgrounds
  bgDisplay:   "#06060a",   // LCD preset display background
  border:      "#2a2a32",   // Standard border
  borderDim:   "#1a1a20",   // Subtle/inactive border
  amber:       "#f5a623",   // Primary amber — preset display text, knob indicators
  amberDim:    "#7a4f0a",   // Dimmed amber — inactive soft buttons, display border
  amberGlow:   "rgba(245,166,35,0.15)",  // Amber glow for active states
  amberGlow2:  "rgba(245,166,35,0.06)",  // Very subtle amber tint
  green:       "#39e97b",   // Active/enabled indicator color (INPUT, OUTPUT, groups)
  greenGlow:   "rgba(57,233,123,0.35)",  // Green LED glow
  greenDark:   "#1a3d2a",   // Reserved
  red:         "#e94040",   // Danger/armed state (DELETE confirm, OVERWRITE)
  textPrimary: "#e8e8f0",   // Primary readable text, knob labels, 0dB labels
  textDim:     "#555566",   // Inactive/placeholder text
  textMid:     "#8888aa",   // Secondary labels, meter labels
  meterGreen:  "#2ecc71",   // Meter segments — safe zone
  meterYellow: "#f1c40f",   // Meter segments — warning zone (>65%)
  meterRed:    "#e74c3c",   // Meter segments — clip zone (>85%)
};

// ── Fonts ─────────────────────────────────────────────────────────────────────
// JUCE: Embed Michroma, Share Tech Mono, Rajdhani as BinaryData.
// Use juce::CustomTypeface::createSystemTypefaceFor() to load them.
const fontFace = `
  @import url('https://fonts.googleapis.com/css2?family=Michroma&family=Share+Tech+Mono&family=Rajdhani:wght@300;400;500;600&display=swap');
`;

// ── LED Component ─────────────────────────────────────────────────────────────
// JUCE: Render as a filled circle with a radial gradient and drop shadow.
// Used as enable/disable indicator inside KnobGroup headers.
// Active = green glow. Inactive = dark unlit appearance.
// Click target is the entire parent group header button, not just the LED itself.
function LED({ active, onClick, color = C.green, glowColor = C.greenGlow }) {
  return (
    <div
      onClick={onClick}
      style={{
        width: 10, height: 10,
        borderRadius: "50%",
        background: active ? color : "#1e1e28",
        boxShadow: active ? `0 0 6px 2px ${glowColor}, 0 0 2px ${color}` : "none",
        border: `1px solid ${active ? color : "#333344"}`,
        cursor: "pointer",
        flexShrink: 0,
        transition: "all 0.15s ease",
      }}
    />
  );
}

// ── Soft Button (outline, amber) ───────────────────────────────────────────────
// JUCE: Used exclusively inside the preset display panel for SAVE/OVERWRITE/RENAME/NEW/DELETE.
// Amber outline, transparent background at rest. Hover = subtle amber tint.
// danger=true switches to red (used for OVERWRITE and DELETE confirm states).
// SAVE flow: first click → arms OVERWRITE (danger, 5s timeout) → second click saves.
// DELETE flow: first click → arms confirm (2s timeout) → second click deletes.
// All buttons are same fixed width inside the display panel (width: 100% of column).
function SoftButton({ label, onClick, danger = false, small = false, enabled = true }) {
  const [pressed, setPressed] = useState(false);
  const color = danger ? C.red : "#f58c00";

  // Enabled = bright orange border + text. Disabled = dim amber. Pressed = inverted (orange fill, dark text).
  const border = pressed ? color : enabled ? color : C.amberDim;
  const text   = pressed ? C.bgDisplay : enabled ? color : C.amberDim;
  const bg     = pressed ? color : "transparent";

  return (
    <button
      onClick={enabled ? onClick : undefined}
      onMouseDown={() => enabled && setPressed(true)}
      onMouseUp={() => setPressed(false)}
      onMouseLeave={() => setPressed(false)}
      onTouchStart={() => enabled && setPressed(true)}
      onTouchEnd={() => setPressed(false)}
      style={{
        background: bg,
        border: `1px solid ${border}`,
        color: text,
        fontFamily: "'Michroma', monospace",
        fontSize: small ? 8 : 10,
        letterSpacing: "0.15em",
        padding: small ? "4px 8px" : "6px 12px",
        cursor: enabled ? "pointer" : "default",
        outline: "none",
        borderRadius: 2,
        transition: "background 0.08s, color 0.08s, border-color 0.08s",
        userSelect: "none", WebkitUserSelect: "none",
        textAlign: "center",
        whiteSpace: "nowrap",
      }}
    >
      {label}
    </button>
  );
}

// ── Source Selector ───────────────────────────────────────────────────────────
// JUCE: Maps to audio device I/O selection via juce::AudioDeviceManager.
// Two instances: INPUT (top-left) and OUTPUT (top-right, mirrored layout).
// Label is static — enable/disable is handled by the MUTE button in the channel strip.
// mirror=true flips the text alignment for the OUTPUT selector.
// In JUCE: populate options from AudioDeviceManager::getAvailableDeviceTypes()
// and AudioDeviceManager::getCurrentAudioDevice()->getInputChannelNames().
function SourceSelector({ label, options, value, onChange, enabled = true, mirror = false }) {
  const [open, setOpen] = useState(false);

  return (
    <div style={{ position: "relative", flex: 1, display: "flex", flexDirection: "column", gap: 4 }}>
      {/* Static label */}
      <div style={{
        background: "transparent",
        border: `1px solid ${C.borderDim}`,
        borderRadius: 2,
        color: C.textMid,
        fontFamily: "'Michroma'", fontSize: 8, letterSpacing: "0.2em",
        padding: "4px 10px",
        display: "flex", alignItems: "center", gap: 7,
        flexDirection: mirror ? "row-reverse" : "row",
      }}>
        <div style={{
          width: 6, height: 6, borderRadius: "50%",
          background: enabled ? C.green : "transparent",
          border: `1px solid ${enabled ? C.green : C.textDim}`,
          boxShadow: enabled ? `0 0 5px ${C.green}` : "none",
          flexShrink: 0,
          transition: "all 0.15s",
        }} />
        {label}
      </div>

      {/* Channel value — dropdown trigger */}
      <div
        onClick={() => setOpen(!open)}
        style={{
          display: "flex", alignItems: "center",
          padding: "5px 10px",
          background: C.bgPanel,
          border: `1px solid ${C.border}`,
          cursor: "pointer",
          justifyContent: mirror ? "flex-end" : "flex-start",
        }}
      >
        <span style={{
          color: C.amber, fontFamily: "'Share Tech Mono'", fontSize: 10,
          flex: 1, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap",
          textAlign: mirror ? "right" : "left",
        }}>
          {value}
        </span>
      </div>

      {open && (
        <div style={{
          position: "absolute", top: "100%", left: 0, right: 0, zIndex: 100,
          background: C.bgPanel, border: `1px solid ${C.border}`,
          borderTop: "none",
        }}>
          {options.map(o => (
            <div
              key={o}
              onClick={() => { onChange(o); setOpen(false); }}
              style={{
                padding: "6px 10px",
                color: o === value ? C.amber : C.textMid,
                fontFamily: "'Share Tech Mono'", fontSize: 10,
                cursor: "pointer",
                textAlign: mirror ? "right" : "left",
                background: o === value ? C.amberGlow2 : "transparent",
              }}
            >
              {o}
            </div>
          ))}
        </div>
      )}
    </div>
  );
}

// ── Preset Nav Button ─────────────────────────────────────────────────────────
// JUCE: Bright orange (#f58c00) outline button. At rest = outline only.
// On mousedown/touchstart = full invert (orange fill, dark text).
// Used for PREV and NEXT preset navigation inside the preset display panel.
// PREV sweeps new name in from right. NEXT sweeps from left.
// Sweep animation: translateX + opacity, 0.25s ease.
// Counter (e.g. 4/20) sits centered between PREV and NEXT in same orange color.
function PresetNavButton({ label, onClick, enabled = true }) {
  const [pressed, setPressed] = useState(false);
  const orange = "#f58c00";
  const border = pressed ? orange : enabled ? orange : C.amberDim;
  const text   = pressed ? C.bgDisplay : enabled ? orange : C.amberDim;
  const bg     = pressed ? orange : "transparent";
  return (
    <button
      onClick={enabled ? onClick : undefined}
      onMouseDown={() => enabled && setPressed(true)}
      onMouseUp={() => setPressed(false)}
      onMouseLeave={() => setPressed(false)}
      onTouchStart={() => enabled && setPressed(true)}
      onTouchEnd={() => setPressed(false)}
      style={{
        background: bg,
        border: `1px solid ${border}`,
        color: text,
        fontFamily: "'Michroma'", fontSize: 8, letterSpacing: "0.12em",
        padding: "4px 10px",
        cursor: enabled ? "pointer" : "default", outline: "none",
        borderRadius: 1,
        transition: "background 0.08s, color 0.08s, border-color 0.08s",
        userSelect: "none", WebkitUserSelect: "none",
      }}
    >
      {label}
    </button>
  );
}

// ── Shift Button ──────────────────────────────────────────────────────────────
// Used for ◁ ▷ order shift buttons in preset display. No border at rest.
// Same invert-on-press behavior as PresetNavButton.
function ShiftButton({ label, onClick, enabled = true }) {
  const [pressed, setPressed] = useState(false);
  const orange = "#f58c00";
  const border = pressed ? orange : enabled ? orange : C.amberDim;
  const text   = pressed ? C.bgDisplay : enabled ? orange : C.amberDim;
  const bg     = pressed ? orange : "transparent";
  return (
    <button
      onClick={enabled ? onClick : undefined}
      onMouseDown={() => enabled && setPressed(true)}
      onMouseUp={() => setPressed(false)}
      onMouseLeave={() => setPressed(false)}
      onTouchStart={() => enabled && setPressed(true)}
      onTouchEnd={() => setPressed(false)}
      style={{
        background: bg,
        border: `1px solid ${border}`,
        color: text,
        fontFamily: "'Share Tech Mono'", fontSize: 11, letterSpacing: "0.12em",
        padding: "2px 10px",
        cursor: enabled ? "pointer" : "default", outline: "none",
        borderRadius: 1,
        transition: "background 0.08s, color 0.08s, border-color 0.08s",
        userSelect: "none", WebkitUserSelect: "none",
        flex: 1,
      }}
    >
      {label}
    </button>
  );
}

// ── Naming Save Button ────────────────────────────────────────────────────────
// SAVE button used inside the preset display naming mode.
// Bright orange + invertible only when a name has been entered. Otherwise dim.
function NamingSaveButton({ enabled, onClick }) {
  const [pressed, setPressed] = useState(false);
  const orange = "#f58c00";
  return (
    <button
      onClick={enabled ? onClick : undefined}
      onMouseDown={() => enabled && setPressed(true)}
      onMouseUp={() => setPressed(false)}
      onMouseLeave={() => setPressed(false)}
      onTouchStart={() => enabled && setPressed(true)}
      onTouchEnd={() => setPressed(false)}
      style={{
        background: pressed ? orange : "transparent",
        border: `1px solid ${enabled ? orange : C.amberDim}`,
        color: pressed ? C.bgDisplay : enabled ? orange : C.amberDim,
        fontFamily: "'Michroma'", fontSize: 8, letterSpacing: "0.12em",
        padding: "4px 10px",
        cursor: enabled ? "pointer" : "default", outline: "none",
        borderRadius: 1,
        transition: "background 0.08s, color 0.08s, border-color 0.08s",
        userSelect: "none", WebkitUserSelect: "none",
      }}
    >
      SAVE
    </button>
  );
}

// ── Preset Display ────────────────────────────────────────────────────────────
// JUCE: The centerpiece LCD-style panel. Dark background (bgDisplay), amber text.
// Scanline overlay via repeating CSS gradient — replicate with a semi-transparent
// striped texture or custom paint() method in JUCE.
// Fixed height (134px) to accommodate up to 3 wrapped lines of preset name.
// Preset name: Michroma font, 24px, amber with text glow. Left-justified.
// Name animates in on change (sweep left/right via translateX + opacity).
// PREV/NEXT buttons pinned to bottom via marginTop: auto.
// Counter (index/total) centered between PREV/NEXT in orange (#f58c00).
// Soft buttons (SAVE, RENAME, NEW, DELETE) stacked vertically on right side,
// separated from name area by a thin amber vertical divider line.
// JUCE: Store presets as JSON in ~/NAM/ root via juce::JSON and juce::File.
function PresetDisplay({ name, onSave, onRename, onNew, onDelete, onPrev, onNext, total, index, sweepDir, sweepKey, presetList, onMove, isDirty, hasLoadedPreset }) {
  const [deleteArmed, setDeleteArmed] = useState(false);
  const [saveArmed,   setSaveArmed]   = useState(false);
  const [flash,       setFlash]       = useState(null);
  const [naming,      setNaming]      = useState(false);
  const [namingMode,  setNamingMode]  = useState("new"); // "new" | "rename"
  const [namingText,  setNamingText]  = useState("");
  const inputRef = useRef(null);
  const orange = "#f58c00";

  // Auto-focus the text input when naming mode opens
  useEffect(() => {
    if (naming && inputRef.current) {
      inputRef.current.focus();
      if (namingMode === "rename") {
        inputRef.current.select();
      }
    }
  }, [naming, namingMode]);

  const handleSave = () => {
    if (!saveArmed) {
      setSaveArmed(true);
      setTimeout(() => setSaveArmed(false), 5000);
    } else {
      setSaveArmed(false);
      setFlash("WRITTEN");
      onSave?.();
      setTimeout(() => setFlash(null), 1000);
    }
  };

  const handleDelete = () => {
    if (!deleteArmed) { setDeleteArmed(true); setTimeout(() => setDeleteArmed(false), 2000); }
    else { setDeleteArmed(false); onDelete?.(); }
  };

  const handleNewClick = () => {
    setNamingText("");
    setNamingMode("new");
    setNaming(true);
  };

  const handleRenameClick = () => {
    setNamingText(name.toUpperCase());
    setNamingMode("rename");
    setNaming(true);
  };

  const handleNamingCancel = () => {
    setNaming(false);
    setNamingText("");
  };

  const handleNamingSave = () => {
    const trimmed = namingText.trim();
    if (!trimmed) return;
    if (namingMode === "new") {
      onNew?.(trimmed);
    } else {
      onRename?.(trimmed);
    }
    setNaming(false);
    setNamingText("");
  };

  const sweepAnim = sweepDir === "right"
    ? "sweepInLeft 0.25s ease"
    : sweepDir === "left"
      ? "sweepInRight 0.25s ease"
      : "none";

  return (
    <div style={{
      background: C.bgDisplay,
      border: `1px solid ${C.amberDim}`,
      borderRadius: 2,
      display: "flex",
      alignItems: "stretch",
      height: 134,
      boxShadow: `inset 0 0 40px rgba(0,0,0,0.6), 0 0 1px ${C.amberDim}`,
      overflow: "hidden",
      position: "relative",
      flexShrink: 0,
    }}>
      {/* Scanline overlay */}
      <div style={{
        position: "absolute", inset: 0, pointerEvents: "none",
        background: "repeating-linear-gradient(0deg, transparent, transparent 2px, rgba(0,0,0,0.08) 2px, rgba(0,0,0,0.08) 4px)",
        zIndex: 1,
      }} />

      {/* Name area */}
      <div style={{
        flex: 1, display: "flex", flexDirection: "column",
        justifyContent: "flex-start", padding: "10px 14px",
        position: "relative", zIndex: 2,
      }}>
        {flash ? (
          <div style={{
            fontFamily: "'Michroma'", fontSize: 13, letterSpacing: "0.25em",
            color: C.amber,
            textShadow: `0 0 20px ${C.amber}, 0 0 40px ${C.amberGlow}`,
            animation: "pulse 0.5s ease",
          }}>
            {flash}
          </div>
        ) : naming ? (
          <>
            {/* Editable preset name input */}
            <input
              ref={inputRef}
              type="text"
              value={namingText}
              onChange={(e) => setNamingText(e.target.value.toUpperCase())}
              onKeyDown={(e) => {
                if (e.key === "Enter")  handleNamingSave();
                if (e.key === "Escape") handleNamingCancel();
              }}
              placeholder="MY PRESET"
              maxLength={40}
              style={{
                background: "transparent",
                border: "none",
                outline: "none",
                fontFamily: "'Michroma'", fontSize: 24, letterSpacing: "0.08em",
                color: C.amber, lineHeight: 1.2,
                textShadow: `0 0 20px rgba(245,166,35,0.5), 0 0 60px rgba(245,166,35,0.15)`,
                caretColor: C.amber,
                width: "100%", padding: 0,
              }}
            />
            {/* Cancel / counter / Save */}
            <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginTop: "auto" }}>
              <PresetNavButton label="CANCEL" onClick={handleNamingCancel} />
              <span style={{
                fontFamily: "'Share Tech Mono'", fontSize: 11,
                color: orange, letterSpacing: "0.1em",
              }}>
                {namingMode === "new" ? `${total + 1}/${total + 1}` : `${index + 1}/${total}`}
              </span>
              <NamingSaveButton enabled={namingText.trim().length > 0} onClick={handleNamingSave} />
            </div>
          </>
        ) : (
          <>
            <div
              key={sweepKey}
              style={{
                fontFamily: "'Michroma'", fontSize: 24, letterSpacing: "0.08em",
                color: C.amber, lineHeight: 1.2,
                textShadow: `0 0 20px rgba(245,166,35,0.5), 0 0 60px rgba(245,166,35,0.15)`,
                animation: sweepAnim,
                wordBreak: "break-all",
                overflowWrap: "anywhere",
                hyphens: "none",
              }}
            >
              {name.toUpperCase()}
            </div>
            {/* Prev / counter / Next — pinned to bottom */}
            <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginTop: "auto" }}>
              <PresetNavButton label="PREV" onClick={onPrev} enabled={total > 1} />
              <span style={{
                fontFamily: "'Share Tech Mono'", fontSize: 11,
                color: orange, letterSpacing: "0.1em",
              }}>
                {total > 0 ? `${index + 1}/${total}` : "—"}
              </span>
              <PresetNavButton label="NEXT" onClick={onNext} enabled={total > 1} />
            </div>
          </>
        )}
      </div>

      {/* Divider */}
      <div style={{ width: 1, background: C.amberDim, margin: "10px 0", zIndex: 2 }} />

      {/* Soft buttons — all disabled during naming mode */}
      <div style={{
        display: "flex", flexDirection: "column", gap: 4,
        padding: "10px 10px", justifyContent: "center", zIndex: 2, minWidth: 70,
        pointerEvents: naming ? "none" : "auto",
      }}>
        <SoftButton label="NEW"    onClick={handleNewClick} enabled={isDirty || total === 0} small />
        <SoftButton
          label={saveArmed ? "WRITE" : "SAVE"}
          onClick={handleSave}
          danger={saveArmed}
          enabled={isDirty && hasLoadedPreset}
          small
        />
        <SoftButton label="RENAME" onClick={handleRenameClick} enabled={hasLoadedPreset} small />
        <SoftButton
          label={deleteArmed ? "CONFIRM" : "DELETE"}
          onClick={handleDelete}
          danger={deleteArmed}
          enabled={hasLoadedPreset}
          small
        />
        {/* Order shift buttons */}
        <div style={{ display: "flex", gap: 4, marginTop: 2, width: "100%" }}>
          <ShiftButton label="◀" onClick={() => onMove(index, index - 1)} enabled={hasLoadedPreset && index > 0} />
          <ShiftButton label="▶" onClick={() => onMove(index, index + 1)} enabled={hasLoadedPreset && index < total - 1} />
        </div>
      </div>
    </div>
  );
}

// ── Browser Row ───────────────────────────────────────────────────────────────
// JUCE: Used for all 5 file browser rows.
// Row 1 (Capture Collection): no active button. Scans ~/NAM/Captures/ for subdirectories.
// Row 2 (Capture file): CAPTURE button. Scans current collection for .nam files.
//   First press = load + activate. Subsequent = toggle bypass.
  //   On load, parse each .nam file's metadata.gear_type. Do not store our own capture type.
// Row 3 (IR Collection): no active button. Scans ~/NAM/IR/ for subdirectories.
  //   Locked (disabled=true, text="AMP CAB" or "AMP PEDAL CAB") when gear_type includes a cab.
// Row 4 (IR file): IR (CAB) button. Scans current IR collection for .wav files.
  //   Locked same as Row 3 when gear_type includes a cab.
//   First press = load + activate. Subsequent = toggle bypass.
// All rows: when NAM dir not set, show "File › Settings → Set NAM Directory" centered.
// Navigation: ◀ ▶ arrows page through items with wrap-around.
// Counter (e.g. 3/12) shown between arrows and active button.
// disabled=true: hides arrows and counter, centers text, dims row, blocks active button.
// JUCE: Use juce::RangedDirectoryIterator to scan dirs.
// Store all indices in juce::PropertiesFile. Use relative paths in preset JSON.
// When resolving: juce::File(namRoot).getChildFile(relativePath).
function BrowserRow({ items, index, onPrev, onNext, total, rightEl, isActive, onActivate, buttonLabel = "ACTIVE", disabled = false }) {
  return (
    <div style={{
      display: "flex", alignItems: "center", gap: 0,
      background: C.bgPanel,
      border: `1px solid ${C.border}`,
      height: 36,
    }}>
      {!disabled && <NavButton onClick={onPrev}>◀</NavButton>}
      <div style={{
        flex: 1, textAlign: disabled ? "center" : "left",
        fontFamily: "'Share Tech Mono'", fontSize: disabled ? 10 : 12,
        color: disabled ? C.textDim : C.textPrimary,
        overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap",
        padding: "0 8px",
        letterSpacing: "0.04em",
      }}>
        {items[index]}
      </div>
      {!disabled && (
        <div style={{
          fontFamily: "'Share Tech Mono'", fontSize: 9,
          color: C.textMid, padding: "0 10px", flexShrink: 0,
          borderLeft: `1px solid ${C.border}`,
          borderRight: `1px solid ${C.border}`,
          height: "100%", display: "flex", alignItems: "center",
        }}>
          {index + 1}/{total}
        </div>
      )}
      {!disabled && <NavButton onClick={onNext}>▶</NavButton>}
      {onActivate !== undefined && (
        <ActiveButton key={buttonLabel} active={isActive} onClick={disabled ? undefined : onActivate} label={buttonLabel} />
      )}
      {rightEl && (
        <div style={{
          padding: "0 10px", borderLeft: `1px solid ${C.border}`,
          height: "100%", display: "flex", alignItems: "center",
        }}>
          {rightEl}
        </div>
      )}
    </div>
  );
}

// ── Active Button ─────────────────────────────────────────────────────────────
// JUCE: Green outline button with LED dot. Sits on right edge of BrowserRow.
// Active = green fill tint + green border + glowing LED dot.
// Inactive = dim border, no fill, dark LED outline only.
// Fixed minWidth: 96px — both CAPTURE and IR (CAB) buttons must match this exactly.
// Toggling does NOT affect row appearance — only the DSP processing chain.
// label prop: "CAPTURE" for row 2, "IR (CAB)" for row 3.
function ActiveButton({ active, onClick, label = "ACTIVE" }) {
  const [hov, setHov] = useState(false);
  return (
    <button
      onClick={(e) => { e.currentTarget.blur(); onClick?.(); }}
      onMouseEnter={() => setHov(true)}
      onMouseLeave={() => setHov(false)}
      style={{
        background: active
          ? "rgba(57,233,123,0.12)"
          : hov ? "rgba(255,255,255,0.04)" : "transparent",
        border: `1px solid ${active ? C.green : hov ? C.border : C.borderDim}`,
        color: active ? C.green : hov ? C.textMid : C.textDim,
        fontFamily: "'Michroma'", fontSize: 7, letterSpacing: "0.18em",
        padding: "0 10px",
        height: "100%",
        cursor: "pointer",
        outline: "none",
        WebkitAppearance: "none",
        appearance: "none",
        flexShrink: 0,
        minWidth: 96,
        borderLeft: `1px solid ${active ? C.green : C.border}`,
        display: "flex", alignItems: "center", gap: 6,
        transition: "all 0.15s",
      }}
    >
      <div style={{
        width: 6, height: 6, borderRadius: "50%",
        background: active ? C.green : "transparent",
        border: `1px solid ${active ? C.green : C.textDim}`,
        boxShadow: active ? `0 0 5px ${C.green}` : "none",
        flexShrink: 0,
        transition: "all 0.15s",
      }} />
      {label}
    </button>
  );
}

// ── Nav Button (◀ ▶) ──────────────────────────────────────────────────────────
// JUCE: Simple left/right arrow button used in all BrowserRows.
// Hover = amber tint background. Borders on both sides create a clean separator look.
// Used for collection, capture, and IR navigation with wrap-around indexing.
function NavButton({ children, onClick }) {
  const [pressed, setPressed] = useState(false);
  const orange = "#f58c00";
  return (
    <button
      onClick={onClick}
      onMouseDown={() => setPressed(true)}
      onMouseUp={() => setPressed(false)}
      onMouseLeave={() => setPressed(false)}
      onTouchStart={() => setPressed(true)}
      onTouchEnd={() => setPressed(false)}
      style={{
        background: pressed ? orange : "transparent",
        border: "none",
        color: pressed ? C.bgDisplay : C.amberDim,
        fontFamily: "'Share Tech Mono'", fontSize: 11,
        padding: "0 10px", height: "100%", cursor: "pointer",
        transition: "background 0.08s, color 0.08s", flexShrink: 0,
        borderLeft: `1px solid ${C.border}`,
        borderRight: `1px solid ${C.border}`,
        userSelect: "none",
        WebkitUserSelect: "none",
      }}
    >
      {children}
    </button>
  );
}



// ── Meter ─────────────────────────────────────────────────────────────────────
// JUCE: Vertical LED-style peak meter. Render as stacked rectangles in paint().
// wide=true doubles segment width (16px vs 8px) — used in main channel strip.
// Segment colors: green (0-65%), yellow (65-85%), red (85-100%).
// Label ("IN" / "OUT") renders BELOW the meter track.
// 0dB label renders between the meter and the level knob below it.
// In JUCE: drive level from AudioBuffer RMS/peak per block in processBlock().
// Use juce::Timer to repaint at ~60fps. Apply ballistics (fast attack, slow decay).
// When muted: level is forced to 0 (meter goes dark).
// Pre-fader metering — shows model output before output level knob is applied.
function Meter({ label, level, height = 160, wide = false }) {
  const segH    = 5;
  const segGap  = 2;
  const segW    = wide ? 16 : 8;
  const isFlex  = height === "100%";
  const segments = isFlex ? 30 : Math.round(Number(height) / (segH + segGap));
  const filled  = Math.round(level * segments);

  return (
    <div style={{
      display: "flex", flexDirection: "column", alignItems: "center",
      gap: 4, width: segW,
      flex: isFlex ? 1 : undefined,
    }}>
      <div style={{
        display: "flex", flexDirection: "column-reverse", gap: segGap,
        flex: isFlex ? 1 : undefined,
        height: isFlex ? undefined : height,
        justifyContent: "flex-start",
      }}>
        {Array.from({ length: segments }).map((_, i) => {
          const active = i < filled;
          const pct    = i / segments;
          const color  = pct > 0.85 ? C.meterRed : pct > 0.65 ? C.meterYellow : C.meterGreen;
          return (
            <div key={i} style={{
              width: segW, height: segH,
              background: active ? color : "#1a1a22",
              boxShadow: active && pct > 0.85 ? `0 0 4px ${C.meterRed}` : "none",
              borderRadius: 1,
              transition: "background 0.05s",
              flexShrink: 0,
            }} />
          );
        })}
      </div>
      <div style={{
        fontFamily: "'Michroma'", fontSize: 7, letterSpacing: "0.1em",
        color: C.textMid,
      }}>
        {label}
      </div>
    </div>
  );
}

// ── Fader ─────────────────────────────────────────────────────────────────────
// NOTE: The vertical fader was REMOVED from the final design.
// Output level is now controlled by the OUTPUT LEVEL knob below the output meter.
// This component is kept here for reference but is NOT used in the current layout.
// JUCE: Do not implement this component.
function Fader({ value, onChange, height = 160 }) {
  const trackRef = useRef(null);
  const dragging = useRef(false);
  const isFlex   = height === "100%";

  const handleMouseDown = (e) => {
    dragging.current = true;
    e.preventDefault();
  };

  useEffect(() => {
    const onMove = (e) => {
      if (!dragging.current || !trackRef.current) return;
      const rect = trackRef.current.getBoundingClientRect();
      const pct  = 1 - Math.max(0, Math.min(1, (e.clientY - rect.top) / rect.height));
      onChange(pct);
    };
    const onUp = () => { dragging.current = false; };
    window.addEventListener("mousemove", onMove);
    window.addEventListener("mouseup",   onUp);
    return () => { window.removeEventListener("mousemove", onMove); window.removeEventListener("mouseup", onUp); };
  }, [onChange]);

  const thumbPct = 1 - value;

  return (
    <div style={{
      display: "flex", flexDirection: "column", alignItems: "center",
      gap: 4, width: 14,
      flex: isFlex ? 1 : undefined,
    }}>
      <div
        ref={trackRef}
        style={{
          width: 8,
          height: isFlex ? undefined : height,
          flex: isFlex ? 1 : undefined,
          position: "relative",
          background: "#0e0e16",
          border: `1px solid ${C.border}`,
          borderRadius: 2, cursor: "pointer",
        }}
      >
        {/* Unity mark */}
        <div style={{
          position: "absolute", left: -5, right: -5,
          top: "25%", height: 1,
          background: C.amberDim,
        }} />
        {/* Thumb */}
        <div
          onMouseDown={handleMouseDown}
          onDoubleClick={() => onChange(0.75)}
          style={{
            position: "absolute", left: -5, right: -5,
            top: `calc(${thumbPct * 100}% - 7px)`,
            height: 14,
            background: "#2a2a38",
            border: `1px solid ${C.border}`,
            borderRadius: 2, cursor: "grab",
            display: "flex", alignItems: "center", justifyContent: "center",
            boxShadow: "0 2px 6px rgba(0,0,0,0.5)",
          }}
        >
          <div style={{ width: "60%", height: 1, background: C.textDim }} />
        </div>
      </div>
      <div style={{
        fontFamily: "'Michroma'", fontSize: 7, letterSpacing: "0.1em",
        color: C.textMid,
      }}>
        OUT
      </div>
    </div>
  );
}

// ── Mute Button ───────────────────────────────────────────────────────────────
// JUCE: Small orange (#f58c00) outline button with LED dot indicator.
// Two instances: one below input meter, one below output meter.
// Active (muted) = orange fill tint + orange border + glowing orange LED dot.
// Muting INPUT: zeroes input meter + toggles INPUT source selector button off.
// Muting OUTPUT: zeroes output meter + toggles OUTPUT source selector button off.
// JUCE: Apply a gain of 0.0f to the audio buffer when muted.
// Use a short ramp (e.g. 10ms) to avoid click artifacts on mute/unmute.
// Do NOT kill DSP processing on mute — keep model running to avoid glitchy re-engage.
function MuteButton({ active, onClick }) {
  const [hov, setHov] = useState(false);
  return (
    <button
      onClick={onClick}
      onMouseEnter={() => setHov(true)}
      onMouseLeave={() => setHov(false)}
      style={{
        background: active ? "rgba(245,140,0,0.15)" : hov ? "rgba(255,255,255,0.04)" : "transparent",
        border: `1px solid ${active ? "#f58c00" : hov ? C.border : C.borderDim}`,
        borderRadius: 2,
        color: active ? "#f58c00" : C.textDim,
        fontFamily: "'Michroma'", fontSize: 7, letterSpacing: "0.18em",
        padding: "3px 8px",
        cursor: "pointer", outline: "none",
        display: "flex", alignItems: "center", gap: 5,
        transition: "all 0.15s",
        flexShrink: 0,
      }}
    >
      <div style={{
        width: 5, height: 5, borderRadius: "50%",
        background: active ? "#f58c00" : "transparent",
        border: `1px solid ${active ? "#f58c00" : C.textDim}`,
        boxShadow: active ? "0 0 5px rgba(245,140,0,0.7)" : "none",
        flexShrink: 0,
        transition: "all 0.15s",
      }} />
      MUTE
    </button>
  );
}

// ── Knob ──────────────────────────────────────────────────────────────────────
// JUCE: Render as a custom juce::Slider (ROTARY_VERTICAL_DRAG style) or custom Component.
// Range: 0.0 to 1.0 normalized. Map to parameter range in processBlock().
// Drag: vertical mouse drag (up = increase). 150px per full range sweep.
// Double-click: reset to defaultValue (per-knob, defaults to 0.5).
//   INPUT LEVEL / OUTPUT LEVEL: defaultValue = 0.5 (center = 0dB unity)
//   BASS / MID / TREBLE:         defaultValue = 0.5 (center = 0dB / flat EQ)
//   GATE OPEN:                   defaultValue = 0   (-100dBFS = fully open)
//   GATE CLOSE:                  defaultValue = 1   (0dBFS = fully open)
// Indicator: amber dot at edge of knob body. Travel arc: -135° to +135°.
// Label renders BELOW knob. Supports multi-line via \n (whiteSpace: pre-line).
// Knob sizes in use: 36px (level knobs), 40px (tone/gate knobs).
// JUCE: All knobs should be automatable juce::AudioProcessorParameter instances.
//
// Parameter mappings:
//   INPUT LEVEL:  ±20dB, center (0.5 normalized) = 0dB unity
//   OUTPUT LEVEL: ±20dB, center (0.5 normalized) = 0dB unity
//   BASS:         shelf filter, ±12dB, center ~100Hz
//   MID:          peak filter, ±12dB, center ~800Hz
//   TREBLE:       shelf filter, ±12dB, center ~8kHz
//   GATE OPEN:    level threshold below which gate closes, -100dBFS to 0dBFS
//   GATE CLOSE:   hysteresis threshold above which gate opens, -100dBFS to 0dBFS
//   CLOSE must always be >= OPEN. Gate is effectively disabled at defaults (open=0, close=1).
//   Original NAM plugin uses a single threshold -100dB to 0dB. Our two-knob design adds hysteresis.
function Knob({ label, value, onChange, size = 52, defaultValue = 0.5 }) {
  const dragging = useRef(false);
  const startY   = useRef(0);
  const startVal = useRef(0);

  const handleMouseDown = (e) => {
    dragging.current = true;
    startY.current   = e.clientY;
    startVal.current = value;
    e.preventDefault();
  };

  useEffect(() => {
    const onMove = (e) => {
      if (!dragging.current) return;
      const delta = (startY.current - e.clientY) / 150;
      onChange(Math.max(0, Math.min(1, startVal.current + delta)));
    };
    const onUp = () => { dragging.current = false; };
    window.addEventListener("mousemove", onMove);
    window.addEventListener("mouseup",   onUp);
    return () => { window.removeEventListener("mousemove", onMove); window.removeEventListener("mouseup", onUp); };
  }, [onChange]);

  const angle  = -135 + value * 270;
  const r      = size / 2 - 6;
  const rad    = (angle - 90) * Math.PI / 180;
  const px     = size / 2 + r * Math.cos(rad);
  const py     = size / 2 + r * Math.sin(rad);

  // Format value readout based on label context
  const getReadout = () => {
    if (label.includes("dBFS") || label === "OPEN" || label === "CLOSE") {
      const db = -100 + value * 100;
      return db >= 0 ? "0 dB" : `${db.toFixed(0)} dB`;
    }
    if (label.includes("LEVEL")) {
      const db = (value - 0.5) * 40;
      return db >= 0 ? `+${db.toFixed(1)} dB` : `${db.toFixed(1)} dB`;
    }
    if (["BASS", "MID", "TREBLE"].some(t => label.includes(t))) {
      const db = (value - 0.5) * 24;
      return db >= 0 ? `+${db.toFixed(1)} dB` : `${db.toFixed(1)} dB`;
    }
    const pct = Math.round(value * 100);
    return `${pct}%`;
  };

  return (
    <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 3 }}>
      {/* Value readout — top */}
      <div style={{
        fontFamily: "'Share Tech Mono'", fontSize: 9, letterSpacing: "0.1em",
        color: C.textPrimary, textAlign: "center", minHeight: 12,
        whiteSpace: "nowrap",
      }}>
        {getReadout()}
      </div>
      <svg
        width={size} height={size}
        onMouseDown={handleMouseDown}
        onDoubleClick={() => onChange(defaultValue)}
        style={{ cursor: "pointer", overflow: "visible" }}
      >
        {/* Track arc */}
        <circle cx={size/2} cy={size/2} r={r}
          fill="none" stroke="#1a1a26" strokeWidth={3} />
        {/* Knob body */}
        <circle cx={size/2} cy={size/2} r={size/2 - 3}
          fill="url(#knobGrad)" stroke={C.border} strokeWidth={1} />
        {/* Indicator dot */}
        <circle cx={px} cy={py} r={3}
          fill={C.amber}
          style={{ filter: `drop-shadow(0 0 3px ${C.amber})` }}
        />
        <defs>
          <radialGradient id="knobGrad" cx="40%" cy="35%" r="65%">
            <stop offset="0%" stopColor="#2e2e3e" />
            <stop offset="100%" stopColor="#141420" />
          </radialGradient>
        </defs>
      </svg>
      {/* Label — bottom */}
      <div style={{
        fontFamily: "'Michroma'", fontSize: 7, letterSpacing: "0.12em",
        color: C.textPrimary, textAlign: "center",
        whiteSpace: "pre-line", lineHeight: 1.5,
      }}>
        {label}
      </div>
    </div>
  );
}

// ── Knob Group ────────────────────────────────────────────────────────────────
// JUCE: A bordered panel with a clickable header that enables/disables the group.
// The border has a "cutout" effect — the header button sits on top of the top border.
// Header = LED dot + label text, acts as a single clickable toggle button.
// Active = green LED glow, visible border, full opacity knobs.
// Inactive = dark LED, dim border, knobs at 28% opacity, pointer-events disabled.
// Two groups in use:
//   TONE: Bass, Mid, Treble knobs — post-model parametric EQ
//   NOISE GATE: Low (open threshold), High (hysteresis) knobs — pre-model gate
// Both groups sit in the center column of the main strip, vertically space-evenly.
// JUCE: Bypassing a group should smoothly fade its DSP contribution (10ms ramp).
function KnobGroup({ label, children, active, onToggle }) {
  const [hov, setHov] = useState(false);

  return (
    <div style={{
      display: "inline-flex", flexDirection: "column",
      border: `1px solid ${active ? C.border : C.borderDim}`,
      borderRadius: 2,
      transition: "border-color 0.2s",
      overflow: "visible",
      position: "relative",
    }}>
      {/* Header button — sits on top border, centered */}
      <div
        onClick={onToggle}
        onMouseEnter={() => setHov(true)}
        onMouseLeave={() => setHov(false)}
        style={{
          position: "absolute", top: -13, left: "50%",
          transform: "translateX(-50%)",
          display: "flex", alignItems: "center", gap: 7,
          background: hov
            ? active ? "rgba(57,233,123,0.08)" : "rgba(255,255,255,0.04)"
            : C.bg,
          border: `1px solid ${active ? C.border : C.borderDim}`,
          borderRadius: 2,
          padding: "2px 8px 2px 6px",
          cursor: "pointer",
          zIndex: 2,
          whiteSpace: "nowrap",
          transition: "background 0.15s, border-color 0.2s",
        }}
      >
        <LED active={active} />
        <span style={{
          fontFamily: "'Michroma'", fontSize: 8, letterSpacing: "0.2em",
          color: active ? C.textPrimary : C.textMid,
          transition: "color 0.2s",
          lineHeight: 1,
        }}>
          {label}
        </span>
      </div>

      {/* Knob content */}
      <div style={{
        display: "flex", gap: 16, padding: "20px 16px 12px",
        opacity: active ? 1 : 0.28,
        transition: "opacity 0.2s",
        pointerEvents: active ? "auto" : "none",
      }}>
        {children}
      </div>
    </div>
  );
}

// ── IR Row ────────────────────────────────────────────────────────────────────
// NOTE: This component is DEPRECATED — replaced by BrowserRow with buttonLabel="IR (CAB)".
// JUCE: Do not implement this component. Use BrowserRow logic for all 3 browser rows.
function IRRow({ items, index, onPrev, onNext, total, active, onToggle }) {
  return (
    <div style={{
      display: "flex", alignItems: "center", gap: 0,
      background: C.bgPanel,
      border: `1px solid ${C.border}`,
      height: 36,
      opacity: active ? 1 : 0.45,
      transition: "opacity 0.2s",
    }}>
      <NavButton onClick={onPrev}>◀</NavButton>
      <div style={{
        flex: 1, textAlign: "center",
        fontFamily: "'Share Tech Mono'", fontSize: 12,
        color: active ? C.textPrimary : C.textDim,
        overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap",
        padding: "0 8px",
        letterSpacing: "0.04em",
        transition: "color 0.2s",
      }}>
        {active ? items[index] : "BYPASSED"}
      </div>
      <div style={{
        fontFamily: "'Share Tech Mono'", fontSize: 9,
        color: C.textMid, padding: "0 10px", flexShrink: 0,
        borderLeft: `1px solid ${C.border}`,
        borderRight: `1px solid ${C.border}`,
        height: "100%", display: "flex", alignItems: "center",
      }}>
        {index + 1}/{total}
      </div>
      <NavButton onClick={onNext}>▶</NavButton>
      <ActiveButton active={active} onClick={onToggle} />
    </div>
  );
}

// ── Main Plugin ───────────────────────────────────────────────────────────────
// JUCE: This is the top-level plugin editor (AudioProcessorEditor subclass).
// Plugin dimensions: 380px wide. Height is content-driven (~680-720px typical).
// JUCE: Set fixed size via setSize(380, 720) in the editor constructor.
//
// LAYOUT (top to bottom):
//   Row 1: INPUT selector (left) + OUTPUT selector (right) — audio routing
//   Row 2: Preset display — LCD panel, amber text, PREV/NEXT/counter + soft buttons
//   Row 3: Browser rows — Collection / Capture / IR (CAB) — 3 stacked rows
//   Row 4: Main strip — Input column | Knob groups center | Output column
//           Input column:  IN meter → 0dB label → INPUT LEVEL knob → MUTE button
//           Center:        TONE group (Bass/Mid/Treble) + NOISE GATE group (Low/High)
//           Output column: OUT meter → 0dB label → OUTPUT LEVEL knob → MUTE button
//
// FILE MENU (native OS menu, not in plugin UI):
//   File > Settings > Set NAM Directory
//     Opens juce::FileChooser at ~/
//     On selection: creates NAM/Captures/ and NAM/IR/ inside chosen folder
//     On cancel: creates ~/NAM/Captures/ and ~/NAM/IR/
//     Stores path in juce::PropertiesFile
//
// DEFAULT STATE ON FIRST LAUNCH (no session, no preset loaded):
//   Input enabled: true (green LED lit)
//   Output enabled: true (green LED lit)
//   Capture active: false — user must browse and press CAPTURE to load
//   IR (CAB) active: false — user must browse and press IR (CAB) to load
//   Tone Stack active: FALSE — user must explicitly enable
//   Noise Gate active: FALSE — user must explicitly enable
//   All knobs: center/unity positions (0.5 normalized)
//   collIdx: 0, captIdx: 0, irCollIdx: 0, irIdx: 0
//   loadedCaptureType: null (no capture loaded, IR rows available but inactive)
//   Browser rows: if NAM root not set, all 5 rows show "File › Settings → Set NAM Directory"
export default function NAMPlugin() {
  // ── All state declarations — must come before any derived constants ──────
  const [inputSrc,      setInputSrc]      = useState("Scarlett 2i2 — Input 1");
  const [outputSrc,     setOutputSrc]     = useState("Scarlett 2i2 — Output 1/2");
  const [inputEnabled,  setInputEnabled]  = useState(true);
  const [outputEnabled, setOutputEnabled] = useState(true);

  const [presetIdx,     setPresetIdx]     = useState(0);
  const [sweepDir,      setSweepDir]      = useState(null);
  const [sweepKey,      setSweepKey]      = useState(0);
  const [presetOrder,   setPresetOrder]   = useState([]);

  const [namDirSet,     setNamDirSet]     = useState(true);

  const [collIdx,       setCollIdx]       = useState(1);
  const [captIdx,       setCaptIdx]       = useState(0);
  const [irCollIdx,     setIrCollIdx]     = useState(0);
  const [irIdx,         setIrIdx]         = useState(0);
  const [captureLoaded, setCaptureLoaded] = useState(false);
  const [irLoaded,      setIrLoaded]      = useState(false);
  const [loadedCaptureType, setLoadedCaptureType] = useState(null);

  const [inGain,        setInGain]        = useState(0.5);
  const [outGain,       setOutGain]       = useState(0.5);
  const [inMuted,       setInMuted]       = useState(false);
  const [outMuted,      setOutMuted]      = useState(false);
  const [inLevel,       setInLevel]       = useState(0.7);
  const [outLevel,      setOutLevel]      = useState(0.65);

  const [bass,          setBass]          = useState(0.5);
  const [mid,           setMid]           = useState(0.5);
  const [treble,        setTreble]        = useState(0.5);
  const [gate,          setGate]          = useState(0);
  const [gateHigh,      setGateHigh]      = useState(1);

  const [toneActive,    setToneActive]    = useState(false);
  const [gateActive,    setGateActive]    = useState(false);
  const [irActive,      setIrActive]      = useState(false);
  const [captureActive, setCaptureActive] = useState(false);

  // ── Derived constants (depend on state above) ─────────────────────────────
  const gearTypeHasCab = (type) => type === "amp_cab" || type === "amp_pedal_cab";
  const gearTypeLabel  = (type) => (type ?? "").replace(/_/g, " ").toUpperCase();
  const isFullRig      = gearTypeHasCab(loadedCaptureType);
  const lockedIrLabel  = gearTypeLabel(loadedCaptureType);
  const NAM_NOT_SET = "File › Settings → Set NAM Directory";

  // Preset data — id-keyed object simulating the JSON file store on disk.
  // In production: each preset is a [id].json file in ~/NAM/Presets/.
  // The presetOrder array (above) acts as the manifest.json.
  // Both must stay in sync: SAVE NEW writes a new file + appends to manifest,
  // WRITE updates [id].json, RENAME updates name in both, DELETE removes from both.
  const INITIAL_PRESETS = [
    { id: "p1", name: "BE-100 Crunch Channel",  collIdx: 1, captIdx: 0, irCollIdx: 0, irIdx: 0, bass: 0.6,  mid: 0.45, treble: 0.55, gate: 0,    gateHigh: 1,    inGain: 0.55, outGain: 0.5,  toneActive: true,  gateActive: false, irActive: true,  captureActive: true  },
    { id: "p2", name: "Marshall Plexi — Hot",   collIdx: 2, captIdx: 1, irCollIdx: 2, irIdx: 1, bass: 0.7,  mid: 0.5,  treble: 0.6,  gate: 0.15, gateHigh: 0.35, inGain: 0.6,  outGain: 0.45, toneActive: true,  gateActive: true,  irActive: true,  captureActive: true  },
    { id: "p3", name: "Fender Twin Clean",      collIdx: 0, captIdx: 2, irCollIdx: 0, irIdx: 0, bass: 0.5,  mid: 0.55, treble: 0.65, gate: 0,    gateHigh: 1,    inGain: 0.5,  outGain: 0.55, toneActive: true,  gateActive: false, irActive: false, captureActive: true  },
    { id: "p4", name: "Mesa Mark IV Lead",      collIdx: 3, captIdx: 0, irCollIdx: 1, irIdx: 2, bass: 0.65, mid: 0.4,  treble: 0.5,  gate: 0.2,  gateHigh: 0.4,  inGain: 0.55, outGain: 0.5,  toneActive: true,  gateActive: true,  irActive: true,  captureActive: true  },
    { id: "p5", name: "Vox AC30 Bright",        collIdx: 4, captIdx: 3, irCollIdx: 0, irIdx: 0, bass: 0.45, mid: 0.6,  treble: 0.7,  gate: 0,    gateHigh: 1,    inGain: 0.5,  outGain: 0.5,  toneActive: true,  gateActive: false, irActive: false, captureActive: true  },
    { id: "p6", name: "Soldano SLO Crunch",     collIdx: 1, captIdx: 1, irCollIdx: 3, irIdx: 3, bass: 0.6,  mid: 0.45, treble: 0.55, gate: 0.25, gateHigh: 0.45, inGain: 0.6,  outGain: 0.45, toneActive: false, gateActive: true,  irActive: true,  captureActive: true  },
  ];
  const [presetStore, setPresetStore] = useState(() =>
    Object.fromEntries(INITIAL_PRESETS.map(p => [p.id, p]))
  );

  // Initialize presetOrder from initial presets on first render
  useEffect(() => {
    setPresetOrder(INITIAL_PRESETS.map(p => ({ id: p.id, name: p.name })));
  }, []);

  // Helper — generate a short unique id (in JUCE: juce::Uuid().toDashedString().substring(0,8))
  const makeId = () => `p_${Date.now().toString(36)}_${Math.random().toString(36).slice(2, 6)}`;

  // Build a preset object from current state
  const captureCurrentState = (name) => ({
    name,
    collIdx, captIdx, irCollIdx, irIdx,
    bass, mid, treble, gate, gateHigh,
    inGain, outGain,
    toneActive, gateActive, irActive, captureActive,
  });

  // Snapshot of currently loaded preset values — used for dirty detection
  const [loadedSnapshot, setLoadedSnapshot] = useState(null);

  // Detect if current state differs from loaded preset (or differs from defaults if no preset loaded)
  const currentState = {
    collIdx, captIdx, irCollIdx, irIdx,
    bass, mid, treble, gate, gateHigh,
    inGain, outGain,
    toneActive, gateActive, irActive, captureActive,
    captureLoaded,
  };

  const isDirty = loadedSnapshot
    ? Object.keys(loadedSnapshot).some(k => loadedSnapshot[k] !== currentState[k])
    : captureLoaded || irLoaded || toneActive || gateActive ||
      bass !== 0.5 || mid !== 0.5 || treble !== 0.5 ||
      gate !== 0 || gateHigh !== 1 ||
      inGain !== 0.5 || outGain !== 0.5;

  const loadPreset = (idx, dir) => {
    const entry = presetOrder[idx];
    const p = entry ? presetStore[entry.id] : null;
    if (!p) return;
    setSweepDir(dir);
    setSweepKey(k => k + 1);
    setPresetIdx(idx);
    setCollIdx(p.collIdx);
    setCaptIdx(p.captIdx);
    setIrCollIdx(p.irCollIdx);
    setIrIdx(p.irIdx);
    const presetCaptureType = captureCollectionData[collections[p.collIdx]]?.[p.captIdx]?.type ?? "amp";
    setLoadedCaptureType(presetCaptureType);
    setCaptureLoaded(true);
    setCaptureActive(p.captureActive);
    setIrLoaded(!gearTypeHasCab(presetCaptureType));
    setIrActive(p.irActive);
    setBass(p.bass);
    setMid(p.mid);
    setTreble(p.treble);
    setGate(p.gate);
    setGateHigh(p.gateHigh);
    setInGain(p.inGain);
    setOutGain(p.outGain);
    setToneActive(p.toneActive);
    setGateActive(p.gateActive);
    setLoadedSnapshot({
      collIdx: p.collIdx, captIdx: p.captIdx, irCollIdx: p.irCollIdx, irIdx: p.irIdx,
      bass: p.bass, mid: p.mid, treble: p.treble, gate: p.gate, gateHigh: p.gateHigh,
      inGain: p.inGain, outGain: p.outGain,
      toneActive: p.toneActive, gateActive: p.gateActive,
      irActive: p.irActive, captureActive: p.captureActive,
      captureLoaded: true,
    });
  };

  const goPreset = (dir) => {
    if (presetOrder.length === 0) return;
    const next = dir === "right"
      ? (presetIdx + 1) % presetOrder.length
      : (presetIdx - 1 + presetOrder.length) % presetOrder.length;
    loadPreset(next, dir);
  };

  const movePreset = (from, to) => {
    setPresetOrder(order => {
      const next = [...order];
      const [item] = next.splice(from, 1);
      next.splice(to, 0, item);
      return next;
    });
    setPresetIdx(to);
  };

  const jumpPreset = (i) => {
    loadPreset(i, i > presetIdx ? "right" : "left");
  };

  // ── Preset CRUD operations ────────────────────────────────────────────────
  // JUCE: Each of these reads/writes JSON files in ~/NAM/Presets/ and updates
  // the manifest.json (here represented by presetOrder). Mock implementation
  // stores everything in-memory via React state.
  const writePreset = () => {
    const entry = presetOrder[presetIdx];
    if (!entry) return;
    const updated = { ...captureCurrentState(entry.name), id: entry.id };
    setPresetStore(s => ({ ...s, [entry.id]: updated }));
    setLoadedSnapshot({
      collIdx: updated.collIdx, captIdx: updated.captIdx, irCollIdx: updated.irCollIdx, irIdx: updated.irIdx,
      bass: updated.bass, mid: updated.mid, treble: updated.treble, gate: updated.gate, gateHigh: updated.gateHigh,
      inGain: updated.inGain, outGain: updated.outGain,
      toneActive: updated.toneActive, gateActive: updated.gateActive,
      irActive: updated.irActive, captureActive: updated.captureActive,
      captureLoaded: true,
    });
  };

  const newPreset = (providedName) => {
    const id = makeId();
    const name = providedName?.trim() || "MY PRESET";
    const preset = { ...captureCurrentState(name), id };
    setPresetStore(s => ({ ...s, [id]: preset }));
    setPresetOrder(order => [...order, { id, name }]);
    setPresetIdx(presetOrder.length); // new preset goes to end
    setLoadedSnapshot({
      collIdx: preset.collIdx, captIdx: preset.captIdx, irCollIdx: preset.irCollIdx, irIdx: preset.irIdx,
      bass: preset.bass, mid: preset.mid, treble: preset.treble, gate: preset.gate, gateHigh: preset.gateHigh,
      inGain: preset.inGain, outGain: preset.outGain,
      toneActive: preset.toneActive, gateActive: preset.gateActive,
      irActive: preset.irActive, captureActive: preset.captureActive,
      captureLoaded: true,
    });
  };

  const renamePreset = (newName) => {
    const entry = presetOrder[presetIdx];
    if (!entry || !newName || newName === entry.name) return;
    setPresetStore(s => ({ ...s, [entry.id]: { ...s[entry.id], name: newName } }));
    setPresetOrder(order => order.map((p, i) => i === presetIdx ? { ...p, name: newName } : p));
  };

  const deletePreset = () => {
    const entry = presetOrder[presetIdx];
    if (!entry) return;
    setPresetStore(s => {
      const next = { ...s };
      delete next[entry.id];
      return next;
    });
    setPresetOrder(order => order.filter((_, i) => i !== presetIdx));
    setPresetIdx(i => Math.max(0, Math.min(i, presetOrder.length - 2)));
    setLoadedSnapshot(null);
  };

  // ── Capture collection data ───────────────────────────────────────────────
  // PROTOTYPE ONLY: This hardcoded data simulates what the filesystem provides.
  // JUCE IMPLEMENTATION: Do NOT hardcode this data. Instead:
  //   1. Scan ~/NAM/Captures/ with juce::RangedDirectoryIterator for subdirectories
  //      → these become the collections array (one entry per subfolder)
  //   2. When collIdx changes, scan that subfolder for *.nam files
  //      → these become the captures array for that collection
  //   3. For each .nam file, parse metadata.gear_type to determine IR eligibility.
  //   4. Reset captIdx to 0 whenever collIdx changes.
  //   All data is derived at runtime from the filesystem. Never store it statically.
  const captureCollectionData = namDirSet ? {
    "Bogner Ecstasy":          [{ name: "Ecstasy 101B Ch3 — Hot",  type: "amp" }, { name: "Ecstasy 101B Ch2 — Clean", type: "amp" }, { name: "Shiva EL34 Crunch",        type: "amp" }],
    "Friedman Amplification":  [{ name: "BE-100 Crunch Channel",   type: "amp" }, { name: "BE-100 Clean Bright",      type: "amp" }, { name: "Full Rig — BE-100 V30",    type: "amp_cab" }],
    "Marshall Amplification":  [{ name: "Full Rig — JCM800 4x12",  type: "amp_pedal_cab" }, { name: "JCM800 Lead Ch",           type: "amp" }, { name: "Plexi 100W Hot",           type: "amp" }],
    "Mesa Boogie":             [{ name: "Mark IV Lead",             type: "amp" }, { name: "Rectifier Modern",         type: "amp" }, { name: "Full Rig — Mark V 112",    type: "amp_cab" }],
    "Orange Amplifiers":       [{ name: "Full Rig — Rockerverb 50", type: "amp_cab" }, { name: "AD30 Clean",               type: "amp" }, { name: "OR15 Crunch",              type: "amp" }],
    "Peavey EVH":              [{ name: "5150 Brown Sound",         type: "amp" }, { name: "Full Rig — 5150 4x12",     type: "amp_pedal_cab" }, { name: "5150 III Lead",            type: "amp" }],
  } : { [NAM_NOT_SET]: [{ name: NAM_NOT_SET, type: "amp" }] };

  const collections = namDirSet ? Object.keys(captureCollectionData) : [NAM_NOT_SET];
  const captureData = captureCollectionData[collections[collIdx]] ?? [];
  const captures    = captureData.map(c => c.name);

  // ── IR collection data ────────────────────────────────────────────────────
  // PROTOTYPE ONLY: This hardcoded data simulates what the filesystem provides.
  // JUCE IMPLEMENTATION: Do NOT hardcode this data. Instead:
  //   1. Scan ~/NAM/IR/ with juce::RangedDirectoryIterator for subdirectories
  //      → these become the irCollections array (one entry per subfolder)
  //   2. When irCollIdx changes, scan that subfolder for *.wav files
  //      → these become the irItems array for that collection
  //   3. Reset irIdx to 0 whenever irCollIdx changes.
  //   All data is derived at runtime from the filesystem. Never store it statically.
  //   IR rows remain fully locked (disabled) when metadata.gear_type includes a cab,
  //   regardless of what the filesystem contains.
  const irCollectionData = namDirSet ? {
    "Celestion Cabinets":  ["V30 4x12 — SM57", "V30 4x12 — Off Axis", "V30 4x12 — Ribbon", "G12M 2x12 — Room"],
    "Mesa Boogie Cabs":    ["Rectifier 4x12 — SM57", "Rectifier 4x12 — MD421", "Thiele 1x12 — SM57"],
    "Marshall Cabs":       ["1960A 4x12 — SM57", "1960A 4x12 — Blended", "1960B 4x12 — Off Axis"],
    "Vintage Greenback":   ["Greenback 4x12 — Close", "Greenback 4x12 — Room", "Greenback 2x12 — Ribbon"],
    "Fender Cabs":         ["Deluxe 1x12 — SM57", "Twin 2x12 — SM57", "Bassman 4x10 — Ribbon"],
  } : { [NAM_NOT_SET]: [NAM_NOT_SET] };

  const irCollections = namDirSet ? Object.keys(irCollectionData) : [NAM_NOT_SET];
  const irItems       = irCollectionData[irCollections[irCollIdx]] ?? [NAM_NOT_SET];

  // Animate meters
  useEffect(() => {
    const id = setInterval(() => {
      setInLevel(v  => Math.max(0.1, Math.min(0.95, v  + (Math.random() - 0.5) * 0.12)));
      setOutLevel(v => Math.max(0.1, Math.min(0.95, v  + (Math.random() - 0.5) * 0.12)));
    }, 80);
    return () => clearInterval(id);
  }, []);

  const wrap = (i, arr) => (i + arr.length) % arr.length;

  const inputOptions  = ["Scarlett 2i2 — Input 1", "Scarlett 2i2 — Input 2", "Built-in Microphone"];
  const outputOptions = ["Scarlett 2i2 — Output 1/2", "Scarlett 2i2 — Output 3/4", "Built-in Output"];

  return (
    <>
      <style>{fontFace}</style>
      <style>{`
        @keyframes pulse { 0%,100% { opacity:1; } 50% { opacity:0.4; } }
        @keyframes sweepInLeft { from { opacity:0; transform: translateX(40px); } to { opacity:1; transform: translateX(0); } }
        @keyframes sweepInRight { from { opacity:0; transform: translateX(-40px); } to { opacity:1; transform: translateX(0); } }
        * { box-sizing: border-box; margin: 0; padding: 0; }
        ::-webkit-scrollbar { display: none; }
        /* Kill all browser focus chrome on every button — focus state is identical to rest state */
        button, button:focus, button:focus-visible, button:active, button::-moz-focus-inner {
          outline: none !important;
          box-shadow: none !important;
          -webkit-tap-highlight-color: transparent;
        }
        button::-moz-focus-inner { border: 0 !important; }
      `}</style>

      <div style={{
        width: 380, background: C.bg,
        border: `1px solid ${C.border}`,
        borderRadius: 4,
        padding: 10,
        display: "flex", flexDirection: "column", gap: 6,
        fontFamily: "'Rajdhani', sans-serif",
        boxShadow: "0 20px 60px rgba(0,0,0,0.8)",
        userSelect: "none",
      }}>

        {/* ── Row 1: Audio Routing ── */}
        <div style={{ display: "flex", gap: 10, alignItems: "stretch" }}>
          <SourceSelector
            label="INPUT"
            options={inputOptions}
            value={inputSrc}
            onChange={setInputSrc}
            enabled={!inMuted}
          />
          <SourceSelector
            label="OUTPUT"
            options={outputOptions}
            value={outputSrc}
            onChange={setOutputSrc}
            enabled={!outMuted}
            mirror
          />
        </div>

        {/* ── Row 2: Preset Display ── */}
        <PresetDisplay
          name={presetOrder[presetIdx]?.name ?? "EMPTY PRESET"}
          total={presetOrder.length}
          index={presetIdx}
          sweepDir={sweepDir}
          sweepKey={sweepKey}
          presetList={presetOrder}
          isDirty={isDirty}
          hasLoadedPreset={!!loadedSnapshot}
          onPrev={() => goPreset("left")}
          onNext={() => goPreset("right")}
          onMove={movePreset}
          onSave={writePreset}
          onNew={newPreset}
          onRename={renamePreset}
          onDelete={deletePreset}
        />

        {/*
          ── PROTOTYPE ONLY — REMOVE IN FINAL BUILD ──────────────────────────
          This toggle simulates the "NAM directory not set" state so it can be
          previewed in the UI concept. In the real JUCE plugin, namDirSet is
          determined by whether a valid path exists in juce::PropertiesFile.
          There is no visible toggle in the final UI — the browser rows update
          automatically based on whether the directory has been configured via
          File > Settings > Set NAM Directory.
          ────────────────────────────────────────────────────────────────────
        */}
        <div
          onClick={() => setNamDirSet(v => !v)}
          style={{
            fontFamily: "'Share Tech Mono'", fontSize: 8,
            color: C.textDim, textAlign: "center", cursor: "pointer",
            letterSpacing: "0.1em",
          }}
        >
          [PREVIEW: NAM DIR {namDirSet ? "SET" : "NOT SET"} — click to toggle]
        </div>

        {/* ── Row 3: All Browser Rows Grouped ── */}
        <div style={{
          display: "flex", flexDirection: "column", gap: 2,
          border: `1px solid ${C.border}`,
          borderRadius: 2,
          overflow: "hidden",
        }}>
          <BrowserRow
            items={collections}
            index={collIdx}
            total={collections.length}
            onPrev={() => { setCollIdx(i => wrap(i - 1, collections)); setCaptIdx(0); }}
            onNext={() => { setCollIdx(i => wrap(i + 1, collections)); setCaptIdx(0); }}
            disabled={!namDirSet}
          />
          <BrowserRow
            items={captures}
            index={captIdx}
            total={captures.length}
            onPrev={() => setCaptIdx(i => wrap(i - 1, captures))}
            onNext={() => setCaptIdx(i => wrap(i + 1, captures))}
            isActive={captureActive}
            onActivate={() => {
                // If user has browsed to a different capture than what's loaded, load it.
                const isDifferent = !captureLoaded
                  || captIdx !== loadedSnapshot?.captIdx
                  || collIdx !== loadedSnapshot?.collIdx;
                if (isDifferent) {
                  setCaptureLoaded(true);
                  setCaptureActive(true);
                  const type = captureData[captIdx]?.type ?? "amp";
                  setLoadedCaptureType(type);
                  if (gearTypeHasCab(type)) {
                    setIrActive(false);
                    setIrLoaded(false);
                  }
                  // Update the snapshot so subsequent presses toggle bypass
                  setLoadedSnapshot(s => ({ ...(s ?? {}), captIdx, collIdx }));
                } else {
                  // Same capture loaded — toggle bypass
                  setCaptureActive(v => !v);
                }
              }}
            buttonLabel="CAPTURE"
            disabled={!namDirSet}
          />
          {/* IR collection row — locked when metadata.gear_type includes a cab */}
          <BrowserRow
            items={isFullRig ? [lockedIrLabel] : irCollections}
            index={isFullRig ? 0 : irCollIdx}
            total={isFullRig ? 1 : irCollections.length}
            onPrev={() => { if (!isFullRig) { setIrCollIdx(i => wrap(i - 1, irCollections)); setIrIdx(0); } }}
            onNext={() => { if (!isFullRig) { setIrCollIdx(i => wrap(i + 1, irCollections)); setIrIdx(0); } }}
            disabled={!namDirSet || isFullRig}
          />
          {/* IR file row — locked when metadata.gear_type includes a cab */}
          <BrowserRow
            items={isFullRig ? [lockedIrLabel] : irItems}
            index={isFullRig ? 0 : irIdx}
            total={isFullRig ? 1 : irItems.length}
            onPrev={() => !isFullRig && setIrIdx(i => wrap(i - 1, irItems))}
            onNext={() => !isFullRig && setIrIdx(i => wrap(i + 1, irItems))}
            isActive={isFullRig ? false : irActive}
            onActivate={isFullRig ? undefined : irLoaded
              ? () => setIrActive(v => !v)
              : () => { setIrLoaded(true); setIrActive(true); }
            }
            buttonLabel="IR (CAB)"
            disabled={!namDirSet || isFullRig}
          />
        </div>

        {/* ── Row 4: Main Area — fills remaining space ── */}
        <div style={{ display: "flex", height: 280 }}>
          <div style={{ display: "flex", flexDirection: "column", flex: 1 }}>

            {/* 3-column strip fills all remaining height */}
            <div style={{
              background: C.bgPanel,
              border: `1px solid ${C.border}`,
              borderRadius: 2,
              padding: "10px 10px 8px",
              display: "flex", alignItems: "stretch",
              gap: 10,
              flex: 1,
            }}>

              {/* Left — input meter + level knob + mute */}
              <div style={{
                display: "flex", flexDirection: "column",
                alignItems: "center", gap: 6, flex: 1,
              }}>
                <div style={{ flex: 1, display: "flex", alignItems: "flex-end" }}>
                  <Meter label="IN" level={inMuted ? 0 : inLevel} height={120} wide />
                </div>
                <Knob label={`INPUT\nLEVEL`} value={inGain} onChange={setInGain} size={36} />
                <MuteButton active={inMuted} onClick={() => {
                  setInMuted(v => !v);
                  setInputEnabled(v => !v);
                }} />
              </div>

              {/* Center — knob groups stacked, vertically centered */}
              <div style={{
                flex: 1, display: "flex", flexDirection: "column",
                alignItems: "center", justifyContent: "space-evenly",
                gap: 0, paddingBottom: 4,
              }}>
                <KnobGroup label="TONE" active={toneActive} onToggle={() => setToneActive(v => !v)}>
                  <Knob label="BASS"   value={bass}   onChange={setBass}   size={40} />
                  <Knob label="MID"    value={mid}    onChange={setMid}    size={40} />
                  <Knob label="TREBLE" value={treble} onChange={setTreble} size={40} />
                </KnobGroup>
                <KnobGroup label="NOISE GATE" active={gateActive} onToggle={() => setGateActive(v => !v)}>
                  <Knob label="OPEN"  value={gate}     onChange={setGate}     size={40} defaultValue={0} />
                  <Knob label="CLOSE" value={gateHigh} onChange={setGateHigh} size={40} defaultValue={1} />
                </KnobGroup>
              </div>

              {/* Right — output meter + level knob + mute */}
              <div style={{
                display: "flex", flexDirection: "column",
                alignItems: "center", gap: 6, flex: 1,
              }}>
                <div style={{ flex: 1, display: "flex", alignItems: "flex-end" }}>
                  <Meter label="OUT" level={outMuted ? 0 : outLevel} height={120} wide />
                </div>
                <Knob label={`OUTPUT\nLEVEL`} value={outGain} onChange={setOutGain} size={36} />
                <MuteButton active={outMuted} onClick={() => {
                  setOutMuted(v => !v);
                  setOutputEnabled(v => !v);
                }} />
              </div>

            </div>
          </div>
        </div>

      </div>
    </>
  );
}