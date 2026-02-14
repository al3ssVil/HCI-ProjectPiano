# Piano HCI Project

An interactive piano learning system that evolved from a simple IoT prototype into a multi-module platform:
- ESP32 physical piano (keys + buzzer + LCD + Wi‑Fi)
- Web companion app (Angular) with **SSE live mode** + guided melodies
- AR tutor (Unity + Vuforia) with step‑by‑step overlays
- Azure AI melody recognition (record → cloud → detected song → LCD)

Project documentation details the intended HCI flow, multimodal feedback, and module responsibilities. fileciteturn0file0

## Modules
- [Code (ESP32)](/Piano-Code/README-Code.md)
- [Web App](/Piano-Web/README-Web.md)
- [AR Tutor](/Piano-AR/README-AR.md)
- [Azure Backend](/Piano-Azure/README-Azure.md)


## System architecture

**ESP32 → (SSE) → Web**  
**ESP32 → (HTTPS) → Azure API → AI → LCD**  
**ESP32 → (Serial) → Unity AR**

---

## Core interaction ideas

- **Immediate feedback**: sound (buzzer) + text (LCD) + visual overlays (Web/AR)
- **Guided learning**: melody step index, tolerant mistakes, clear next-action cues
- **Accessibility**: large visual elements, tolerant error handling, reduced cognitive load
- 
---

## Goals
- Real‑time interaction
- Visual learning assistance
- Modular design
