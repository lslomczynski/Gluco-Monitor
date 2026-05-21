# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Gluco-Monitor is ESP32-S3 firmware (Arduino/PlatformIO) for a DIY glucose monitor display. It retrieves real-time glucose data from FreeStyle Libre (via LibreLinkUp API) or Dexcom (via Dexcom Share API) and renders it on a 320×480 AXS15231B TFT touchscreen. A built-in AsyncWebServer exposes a web dashboard on port 80.

## Build & Flash Commands

```bash
# Build firmware
pio run

# Build and flash to device
pio run -t upload

# Monitor serial output (115200 baud)
pio device monitor

# Build + flash + monitor in one step
pio run -t upload && pio device monitor
```

- The target board is `esp32-s3-devkitc-1` (16 MB flash, OPI PSRAM, QIO flash).
- `extra_script.py` renames the output binary to `Gluco-Monitor_<version>.bin`.
- `merge_bin.py` / `build_merged.py` produce a merged firmware image (bootloader + partitions + app) for full-chip flashing.
- Version string comes from `custom_prog_version` in `platformio.ini`; build date from `$DATE` environment variable at compile time.

> **Note:** GFX Library for Arduino v1.6.1 is broken for this project — pin at `^1.5.3` (see comment in `Ecran/Gestion.cpp`).

OTA updates are also available at `http://<device-ip>/OTA` after authorizing access on the physical display.

## Architecture

### Global State (`src/Config.h` / `src/Config.cpp`)
All shared variables (WiFi credentials, sensor credentials, glucose thresholds, current readings, timing counters) are declared `extern` in `Config.h` and defined in `Config.cpp`. This is the single source of truth for runtime state. New cross-module variables go here.

Key enums: `SensorType` (LIBRE/DEXCOM), `GlucoseUnit` (mg/dL / mmol/L), `GlucoseColor` (white/colour).

### Persistence (`src/Stock.cpp`)
Configuration is stored in LittleFS at `/parametres.json`. `ReadFichierParametres()` deserializes it into global variables on boot; `RecordFichierParametres()` serializes them back. `DeserializeConfiguration()` / `SerializeConfiguration()` handle the JSON mapping. Call `RecordFichierParametres()` whenever a setting changes.

### Sensor Data Acquisition
- **FreeStyle Libre** — `src/Libreview.h` / `.cpp`: Calls LibreLinkUp API (`loginLibreLinkUp()`, `LectureGlycemie()`). Polled every `RecurrenceGlycemie` (2 min) from `loop()`.
- **Dexcom** — `src/Dexcom.h` / `.cpp`: Calls Dexcom Share API (`loginDexcomShare()`, `getDexcomReadings()`, `LectureDexcom()`). Same polling interval.

Both populate `glucoseValues[]`, `glucoseHeure[]`, `GlycemieVal`, `TrendArrow`, `lastGlyUnixTime`, and the JSON strings `LoginJSON`, `GraphJSON`, `ConnectionJSON` (stored in PSRAM).

`TrendArrow` values: -1=DoubleDown, 1=Down, 2=DownRight, 3=Right, 4=UpRight, 5=Up, 6=DoubleUp.

### Display System (`src/Ecran/`)
- `Gestion.h/.cpp` — display driver init (AXS15231B via QSPI), touch controller (I2C 0x3B), canvas management, button/radio-button rendering helpers, and `loopEcran()` (called every `loop()` iteration).
- Uses layered `Arduino_Canvas` objects: `CanvaBase` (always present), `CanvaAccueil`, `CanvaMessage`, `CanvaConfig` (allocated on demand).
- `PageActu` (int16) selects the active page. Pages 0–2 are the rotating home/config/messages trio; pages 10+ are fixed full-screen pages (see `#define` list in `Gestion.h`).
- Each page has a `*Setup()` function (called once on entry) and a `handleTouch_*()` function. Add new pages by creating a `page*.h/.cpp` pair and adding the `#define` constant in `Gestion.h`.
- `QuestionConfiguration(question, callbackFn)` blocks until the user answers a yes/no prompt on-screen, then calls `callbackFn`.

### Web Server (`src/Server.cpp`)
ESPAsyncWebServer on port 80. Key endpoints:
- `GET /` — main dashboard HTML
- `GET /ajaxGlycemie` — current reading JSON (polled by the web UI)
- `GET /dataGly` — binary blob of glucose history array (`glucoseHeure[]` + `glucoseValues[]`)
- `GET /JS_Traduction` — active language JSON for the web UI
- `GET /JS_Commun` — shared JS constants including thresholds
- `GET /OTA` / `POST /update` — OTA firmware update (gated by `AutorisationPageBrute`)
- `GET /Brute` — raw JSON debug view (gated)
- `GET /Restart` — triggers `ESP.restart()`

All HTML/JS assets are embedded as C string literals in `src/HTML/*.h` files.

### Multilingual Support (`src/Langues/`)
Each language is a JSON object in a `const char*` in its `.h` file (e.g., `LangEN`, `LangFR`). `T(key)` (in `Langue.cpp`) deserializes the active language JSON and returns the translated string. Supported: EN, FR, DE, ES, IT, PL. Add a new language by creating `xx.h`, including it in `Langue.h`, adding a `LANG_XX` constant, and extending the switch in both `Langue.cpp` and `Server.cpp`'s `/JS_Traduction` handler.

### Time (`src/Heure.h` / `.cpp`)
NTP sync via `DefFuseauHoraire()` (called after WiFi connect). `CheckNTPSync()` retries every 5 s until `HeureValide` is set. `FormatteHeureDate()` formats `DATE`, `HEURE`, `Hmn` strings from the system clock.

### Watchdog & Safety (`main.cpp`)
10-minute hardware watchdog (`esp_task_wdt`). Reset in `loop()` only when WiFi is connected. `AlertePasdeGlycemie()` is called if no valid glucose reading arrives for 20 minutes, or if the last reading timestamp is >30 minutes old.

### PSRAM Usage
Large buffers declared with `EXT_RAM_BSS_ATTR` (e.g., `MessageEcran[8192]`, `LoginJSON`, `GraphJSON`, `ConnectionJSON`). PSRAM is OPI-mode; `BOARD_HAS_PSRAM` and `CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY` build flags are required.

## Glucose Threshold Variables
All stored in mg/dL internally, converted for display when `glucoseUnit == GLUCOSE_UNIT_MMOLL`:
- `glucoseRangeMin` / `glucoseRangeMax` — gauge scale endpoints
- `targetLow` / `targetHigh` — green target range (defaults: 70 / 180)
- `glucoseWarn` — upper warning threshold (default: 300)

`formatGlucoseValue(mgdl)` converts to the appropriate unit string; `getGlucoseUnitLabel()` returns "mg/dL" or "mmol/L".
