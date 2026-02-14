# AR Tutor (Unity + Vuforia) — Piano tutor mode

This module provides **augmented reality guidance** over the physical piano using **Unity + Vuforia**.  
It overlays visual hints directly on the keys and validates the user’s play in real time.

- Next correct key: **green**
- Wrong key: **red** (temporary)
- UI displays a “partitura” / melody sequence so users can follow progress

This multimodal feedback (visual + text + tactile + audio from the real piano) is designed to support beginners,
older adults, and users with disabilities. fileciteturn0file0

---

## Prerequisites

- Unity (version used by the project)
- Vuforia Engine package enabled
- A Vuforia license key (watermark may appear without premium features) fileciteturn0file0
- The ESP32 piano connected and sending note events (this module reads note messages over Serial)

---

## How it works

### Tracking
- Camera recognizes the piano/instrument as a Vuforia target
- When the target is visible, overlays and tutoring logic become active

### Note input bridge (ESP32 → Unity)
A Unity script acts as a bridge between ESP32 and the AR tutor:

- **`EspSerialReader`**
  - Reads serial input line-by-line
  - Normalizes note messages
  - Calls `piano.SimulatePressByName(note)` to trigger the virtual press logic
  - Can start/stop depending on target visibility events fileciteturn0file0

### Tutor logic
- **`ButtonsColoring`**
  - Maps 12 virtual key spheres to the corresponding notes (octave 4)
  - Highlights the next expected key (green)
  - Advances the sequence on correct notes
  - Temporarily highlights red for wrong notes
  - Exposes the “partitura” string through events so UI updates instantly fileciteturn0file0

### UI
- **TextMeshPro** on a Canvas
- **`PartituraLabel`** listens for events from `ButtonsColoring` and updates text like:
  - `Partitura: C4 C#4 B4 ...` fileciteturn0file0

---

## Run / demo checklist

1. Open the Unity project
2. Confirm Vuforia is enabled and the license key is set
3. Verify the target database is assigned to the correct target object
4. Connect ESP32 over USB
5. Select the correct COM port in the serial reader script/settings
6. Press notes on the piano and confirm:
   - green highlight advances the melody
   - red highlight appears on wrong press

---

## Troubleshooting (known issues)

### Target database not detected after reopening the project
Sometimes Vuforia fails to detect targets after closing/reopening.
Fix:
- Remove the image/model from the target object and re-add it
- Save the scene
- If it persists, re-enter the license key fileciteturn0file0

### Multiple Model Targets active
If multiple model targets are active, observer errors can occur.
Ensure only **one Model Target** is active at runtime. fileciteturn0file0

### Unity instability / crashes
Under high load or triggering unimplemented actions, Unity may freeze/crash.
Mitigation:
- Save frequently
- Test incrementally fileciteturn0file0

---

## Future improvements (from the report)

- Add score/accuracy/time-per-note metrics
- Automatic COM reconnection + a “READY” handshake message from ESP32
- Bidirectional Unity → ESP32 communication (e.g., status on the physical LCD) fileciteturn0file0
