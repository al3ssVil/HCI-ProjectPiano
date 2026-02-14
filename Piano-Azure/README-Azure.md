# Azure AI melody recognition (ESP32 → HTTPS → API → AI → LCD)

This module turns the ESP32 piano into a connected IoT assistant:
the user records a short melody, the ESP32 sends it over **HTTPS** to an **Azure-hosted API**,
and the API returns the closest matching song name, which is displayed on the **16×2 LCD**. fileciteturn0file0

---

## User interaction (two‑press logic)

A single **Record/Play** button (GPIO26) controls the workflow: fileciteturn0file0

1) **Press once** → start recording  
- `is_recording = true`  
- clear note buffer (`recorded_count = 0`)  
- start timestamp (`record_start_time`)  
- each key press stores:
  - note index (0–11)
  - computed frequency
  - relative start time
  - duration (computed on release)

2) **Press again** → stop + send to Azure  
- `is_recording = false`  
- `is_playing_back = true` (local playback mode)  
- `send_to_azure = true` triggers a dedicated Azure task  
- API returns JSON → parse song name → display on LCD fileciteturn0file0

---

## Minimal hardware change

To enable Azure recognition, the report specifies only one addition: fileciteturn0file0
- **Record/Play button** on **GPIO26**

Everything else stays the same (12 keys via GPIO + PCF8574, buzzer, LCD, potentiometer, ESP32 Wi‑Fi). fileciteturn0file0

---

## Firmware-side networking details (ESP32)

The report describes the HTTPS setup and payload format: fileciteturn0file0

- Wi‑Fi Station mode (connect + auto reconnect)
- TLS CA store via `esp_tls_init_global_ca_store()`
- HTTP client using SSL transport (`HTTP_TRANSPORT_OVER_SSL`)
- Timeout: `15000ms`
- Dev simplification: `skip_cert_common_name_check = true` (use with care)

A **dedicated Azure task** exists so network calls don’t block key scanning / buzzer / LCD tasks. fileciteturn0file0

---

## Request payload (ESP32 → API)

The ESP32 builds a compact JSON using `cJSON` with these fields: fileciteturn0file0

- `deviceId`: `"piano_esp32"`
- `message`: human-readable melody string with note names joined by `-`
- `sensorType`: `"melody"`
- `value`: `recorded_count`
- `unit`: `"notes"`

Example: fileciteturn0file0
```json
{
  "deviceId": "piano_esp32",
  "message": "Melody recorded: C4-D4-E4-G4 (Total 4 notes)",
  "sensorType": "melody",
  "value": 4,
  "unit": "notes"
}
```

Notes:
- the melody in `message` is limited (e.g., max ~20 notes) to keep payload small fileciteturn0file0

---

## Expected API response

Firmware parses JSON and extracts the song name from the **`response`** field: fileciteturn0file0

```json
{ "response": "Twinkle Twinkle Little Star" }
```

The ESP32 prints `Detected song: <name>` and in the full integration shows it on the LCD. fileciteturn0file0

---

## Azure-side behavior (contract)

The report states the AI is guided by a fixed instruction so responses are deterministic and LCD-friendly: fileciteturn0file0
- behave as a music expert
- identify the closest known song from the note sequence
- respond with **only the song name** (no extra text)

### Recommended endpoint shape
Your Azure API can be implemented as:
- **Azure Functions (HTTP Trigger)** or
- **App Service** REST endpoint

Suggested route (match your repo):
- `POST /api/recognize` (or similar)

---

## Local / manual testing

### cURL
```bash
curl -X POST "https://<your-azure-host>/api/<route>"   -H "Content-Type: application/json"   -d '{"deviceId":"piano_esp32","message":"Melody recorded: C4-D4-E4-G4 (Total 4 notes)","sensorType":"melody","value":4,"unit":"notes"}'
```

### What to verify
- HTTP 200
- response JSON contains `response`
- no extra text (LCD constraint)

---

## Deployment checklist (Azure)

- Set app settings / secrets in Azure (keys, endpoints)
- Enable HTTPS-only
- Add logging for requests/responses (redact secrets)
- Consider enabling CORS if the web module calls the same API

---

## Troubleshooting

- **TLS / certificate errors**: ensure CA store is configured correctly; avoid disabling checks outside dev.
- **Timeouts**: verify endpoint latency; keep AI prompt constrained; ensure `timeout_ms` fits.
- **Bad JSON parse**: log the raw response on ESP32; ensure the API always returns valid JSON with `response`.
