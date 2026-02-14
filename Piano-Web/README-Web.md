# Piano Web (Angular) — Live companion for the physical piano

This module is the **web-based companion app** for the physical ESP32 piano.  
It receives live key events from the ESP32 via **Server‑Sent Events (SSE)** and provides:

- **Free Play**: mirror the physical keys in real time
- **Melody Mode**: guided, step‑by‑step practice for predefined note sequences

> The web app is **hardware-driven**: it reflects what the user plays on the real instrument (not a simulated piano).

---

## How it works (data flow)

ESP32 exposes an SSE endpoint:

- `http://<ESP32_IP>/sse`  (persistent HTTP stream)

The browser connects using `EventSource` and updates the UI whenever a new event arrives.

---

## Prerequisites

- Node.js + npm (or pnpm/yarn)
- Angular CLI (optional; project can be run via npm scripts)

---

## Quick start

```bash
npm install
ng serve
```

Open:
- `http://localhost:4200/`

---

## Configure ESP32 SSE endpoint

You need the ESP32 on the same network as your computer and its IP address.

Typical options:
1. **Hardcode IP** in the service / component that creates the `EventSource`:
   ```ts
   const es = new EventSource("http://<ESP32_IP>/sse");
   ```
2. **Use environment files** (`src/environments/*.ts`) so you can switch dev/prod easily:
   ```ts
   export const environment = {
     sseUrl: "http://<ESP32_IP>/sse"
   };
   ```

Then:
```ts
new EventSource(environment.sseUrl);
```

---

## App modes

### Free Play mode
- Highlights the currently pressed key(s)
- Optionally shows “last note” and/or a short history
- No correctness checks (exploration + connectivity testing)

### Melody mode
- User selects a predefined melody (sequence of notes)
- UI highlights **the next expected key**
- Correct press advances the “step index”
- Wrong press triggers temporary feedback (no full reset)

This “tolerant” design supports learning and accessibility (beginners, older adults, users with motor impairments). fileciteturn0file0

---

## Implementation notes (what to look for in code)

Core responsibilities described in the project documentation: fileciteturn0file0

- `handleNote()` — process incoming note events from ESP32
- `startMelody(notes)` — load melody + start guided session
- `resetMelody()` — reset progress
- `switchToFreeMode()` — back to free mode
- `isActive()` / `isHighlighted()` — key highlighting logic

If you’re cleaning up the codebase, a nice structure is:
- `services/sse.service.ts` (connection + parsing)
- `models/note-event.ts` (typed event contract)
- `components/piano/` (visual + mode logic)

---

## Troubleshooting

### CORS / blocked request
If the ESP32 serves SSE without CORS headers and the web app is on a different origin, the browser may block the stream.
Fix options:
- Serve the Angular app from the same host (not typical for dev), or
- Add permissive CORS headers on the ESP32 SSE response during development.

### No events received
- Ensure ESP32 is connected to Wi‑Fi and you’re using the correct IP
- Try opening `http://<ESP32_IP>/sse` in a browser to confirm it streams
- Verify the ESP32 firmware is broadcasting events

---

## Build

```bash
ng build
```

Artifacts will be in `dist/`.
