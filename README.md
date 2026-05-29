![Gluco-Monitor](https://github.com/user-attachments/assets/543aca45-7e8a-4507-a81c-04a6f4bc0d6f)

# Gluco-Monitor (DIY)

**Gluco-Monitor** is an open-source DIY glucose monitor built on an ESP32-S3 with a 320×480 touchscreen. It retrieves real-time CGM data from **FreeStyle Libre**, **Dexcom**, or **NightScout** and displays it on a dedicated always-on screen — no phone needed.

> ⚠️ **Not a medical device.** Do not use for medical decisions. Always rely on your official CGM device and your healthcare provider's advice.

---

## ✨ Features

| Category | Details |
|----------|---------|
| 📡 **Data sources** | FreeStyle Libre (LibreLinkUp API), Dexcom Share, NightScout REST API |
| 🖥️ **Display** | Real-time glucose value, trend arrow, age indicator, 8-hour bar chart; tap the glucose value to cycle through three layouts: Normal (bar chart), large gauge (altView_01), and colour-coded full-screen (altView_02) |
| 📐 **Units** | mg/dL or mmol/L (user-selectable) |
| 🎯 **Thresholds** | Configurable target range, warning levels, gauge scale |
| 🌐 **Languages** | English, Français, Deutsch, Español, Italiano, Polski |
| 📊 **Web dashboard** | Real-time view at `http://<device-ip>/` from any browser on your network; dark-themed unified navigation across all pages |
| ⚙️ **Web configuration** | Full device configuration at `http://<device-ip>/Settings` — sensor credentials, display settings, glucose thresholds, layout, brightness, MQTT — no cable or on-screen keyboard needed |
| 🏠 **Home Assistant** | MQTT auto-discovery: screen on/off switch, brightness controls, layout selector; state published as retained JSON; configurable via `/Settings` |
| 🔄 **OTA updates** | Firmware update via web interface — no cable needed |
| 🚀 **First-boot setup** | On-screen wizard **or** Wi-Fi AP captive portal (mobile-friendly) |
| 💾 **Persistence** | All settings saved to LittleFS — survive power cycles |
| 💰 **Cost** | ~$25 USD for components |

---

## 🛠️ Hardware

| Component | Specification |
|-----------|---------------|
| 🔲 **Microcontroller** | **ESP32-S3** (esp32-s3-devkitc-1) |
| 🗄️ **Flash** | 16 MB QIO |
| 🧠 **PSRAM** | OPI mode |
| 📺 **Display** | **320×480 AXS15231B** TFT touchscreen |
| 👆 **Touch** | I2C controller (address 0x3B) |
| 📶 **Wi-Fi** | 802.11 b/g/n (built-in) |
| ⚡ **Power** | USB or battery |

👉 Full hardware assembly guide: **https://f1atb.fr/gluco-monitor-diy/**

---

## 📦 Installation

### Prerequisites
- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- ESP32-S3 board with 16 MB flash and OPI PSRAM

### Build & Flash

```bash
# Clone the repository
git clone https://github.com/F1ATB/Gluco-Monitor.git
cd Gluco-Monitor

# Build firmware
pio run

# Build and flash to device
pio run -t upload

# Monitor serial output (115200 baud)
pio device monitor
```

> **Note:** GFX Library for Arduino v1.6.1+ is incompatible with this project — it is pinned to `^1.5.3` in `platformio.ini`.

The build produces `Gluco-Monitor_<version>.bin`. A merged full-chip image (`Gluco-Monitor_<version>.factory.bin`, including bootloader and partition table) is also generated for first-time flashing with `esptool`.

---

## 🚀 First-Boot Setup

On first power-on (no Wi-Fi configured), the device offers two setup methods:

**Option A — On-Screen Setup**
1. Select your Wi-Fi network from the on-screen list
2. Enter the password with the on-screen keyboard
3. Choose language, timezone, sensor type, and credentials
4. Device connects and starts displaying glucose data

**Option B — AP (Captive Portal) Mode**
1. Device broadcasts a temporary Wi-Fi network: `GlucoMonit-xxxxx` (password: `monitor1`)
2. Connect with your phone — the setup page opens automatically
3. Fill in: Wi-Fi credentials, sensor type & credentials, glucose thresholds, language, timezone
4. Tap Save → device restarts in normal mode

---

## 📡 Supported Sensors

| Sensor | API | Authentication |
|--------|-----|----------------|
| **FreeStyle Libre** | LibreLinkUp | Email + password + region |
| **Dexcom** | Dexcom Share | Username + password + region (US / Non-US / JP) |
| **NightScout** | REST API v1 | URL + access token (token optional for public instances) |

Switch between sensors at any time in the **Account** screen or via the `/Settings` web page. Glucose history is cleared automatically when switching sources.

---

## 🖥️ Home Screen Layouts

Tap the glucose value to cycle through three display layouts:

| Layout | Description |
|--------|-------------|
| **Normal** | Semicircle gauge, 8-hour colour-coded bar chart, trend arrow, time, age indicator |
| **altView_01** | Large semicircle gauge (1.5× radius) with needle, no bar chart — easier to read at a glance |
| **altView_02** | Full-screen background colour reflects current glucose range (🔴 below target / 🟢 in range / 🟠 above target / 🟣 very high), large glucose value, no gauge or needle |

A 400 ms touch-suppression window is applied after every layout or page transition, preventing ghost-taps on incoming buttons.

---

## 🌐 Web Dashboard

Open `http://<device-ip>/` from any browser on the same Wi-Fi network for a real-time glucose view. All pages share a unified dark-theme design with a common navigation bar (Glucose | Settings | Data | Update | Restart | Erase).

Key endpoints:

| Endpoint | Description |
|----------|-------------|
| `GET /` | Main dashboard — real-time glucose, trend, history chart |
| `GET /Settings` | Full device configuration UI (requires on-device authorization) |
| `GET /ajaxSettings` | Current device config as JSON (no passwords) |
| `POST /saveSettings` | Apply configuration changes immediately |
| `POST /testConnection` | Test sensor credentials from the browser |
| `GET /ajaxGlycemie` | Current reading (JSON, polled by UI) |
| `GET /dataGly` | 8-hour glucose history (binary blob) |
| `GET /Brute` | Collapsible JSON data view with syntax highlighting |
| `GET /OTA` | OTA firmware upload (requires on-device authorization) |
| `GET /Restart` | Restart confirmation page |
| `POST /Restart` | Trigger actual device reboot |
| `GET /eraseConfig` | Erase all saved configuration and restart |

---

## 🏠 Home Assistant / MQTT Integration

Gluco-Monitor supports **MQTT auto-discovery** for Home Assistant. Once configured, the device publishes its state and registers four entities automatically — no manual YAML configuration needed.

### Setup

1. Open `http://<device-ip>/Settings` in your browser (requires on-device authorization)
2. Scroll to the **MQTT** section
3. Enter broker address, port (default 1883), and optional credentials
4. Click **Test Connection** to verify
5. Click **Save** — the device connects and publishes discovery messages immediately

### Topics

| Topic | Direction | Description |
|-------|-----------|-------------|
| `gluco_monitor/<MAC6>/state` | Device → HA | Retained JSON: `screen`, `brightness`, `brightness_night`, `layout` |
| `gluco_monitor/<MAC6>/cmd/screen` | HA → Device | `ON` / `OFF` — turn display on or off |
| `gluco_monitor/<MAC6>/cmd/brightness` | HA → Device | `0–255` — daytime backlight level |
| `gluco_monitor/<MAC6>/cmd/brightness_night` | HA → Device | `0–255` — night-mode backlight level |
| `gluco_monitor/<MAC6>/cmd/layout` | HA → Device | `0` Normal / `1` altView_01 / `2` altView_02 |

`<MAC6>` is the last 6 hex digits of the device's Wi-Fi MAC address (e.g. `A1B2C3`).

### Discovered Entities

| Entity | HA type | Notes |
|--------|---------|-------|
| Screen | `switch` | Maps to display on/off |
| Brightness | `number` | Daytime backlight (0–255) |
| Brightness night | `number` | Night-mode backlight (0–255) |
| Layout | `select` | Normal / altView_01 / altView_02 |

The device auto-reconnects with a 5-second back-off if the broker becomes unavailable.

---

## 🔄 OTA Firmware Update

1. Open `http://<device-ip>/OTA` in your browser
2. Authorize the update on the physical display (touch prompt, valid for 3 minutes)
3. Select and upload `Gluco-Monitor_<version>.bin`
4. Device restarts automatically with the new firmware

---

## 🔧 Configuration Reference

All settings are stored in `/parametres.json` on the device's LittleFS filesystem and persist across reboots.

Key glucose thresholds (defaults in mg/dL):

| Setting | Default | Description |
|---------|---------|-------------|
| `glucoseRangeMin` | 0 mg/dL | Gauge scale minimum |
| `targetLow` | 70 mg/dL | Lower bound of green target range |
| `targetHigh` | 180 mg/dL | Upper bound of green target range |
| `glucoseWarn` | 300 mg/dL | Upper warning threshold |
| `glucoseRangeMax` | 400 mg/dL | Gauge scale maximum |

---

## 🏗️ Architecture Overview

```
src/
├── main.cpp              # Setup, loop, watchdog, sensor dispatch
├── Config.h / Config.cpp # All global state (extern declarations + definitions)
├── Stock.cpp             # LittleFS persistence (/parametres.json)
├── Libreview.cpp         # FreeStyle Libre API client
├── Dexcom.cpp            # Dexcom Share API client
├── NightScout.cpp        # NightScout REST API client
├── Server.cpp            # AsyncWebServer (port 80) — all HTTP endpoints
├── MQTT.cpp              # MQTT client: HA Discovery, state publish, command handling
├── Heure.cpp             # NTP time sync, timezone, brightness control
├── Ecran/
│   ├── Gestion.cpp       # Display driver init, touch, page routing
│   ├── pageAccueil.cpp   # Home screen — three tap-selectable layouts: Normal (bar chart),
│   │                     #   altView_01 (large semicircle gauge + needle),
│   │                     #   altView_02 (colour-coded full-screen, no gauge)
│   ├── pageCompte.cpp    # Sensor account configuration
│   ├── pageClavier.cpp   # On-screen QWERTY/AZERTY keyboard
│   └── page*.cpp         # Other configuration screens
├── Langues/              # Language strings as JSON (en/fr/de/es/it/pl)
└── HTML/                 # Embedded web assets (C string literals in .h files)
    ├── pageSettings.h    # /Settings — full browser-based device configuration
    ├── pageBrute.h       # /Brute — collapsible JSON data viewer
    ├── pageOTA.h         # /OTA — firmware update
    └── pageMain.h        # / and /Restart pages
```

Pages 0–2 are the rotating home / config / messages trio. Pages 10+ are fixed full-screen pages defined in `Ecran/Gestion.h`. Add new pages by creating a `page*.h/.cpp` pair and a `#define` constant.

---

## 🤝 Contributing

Contributions are welcome!

- Open issues for bugs or feature requests
- Submit pull requests
- Add new language translations — see `src/Langues/` (6 language files + update `Langue.cpp` and `Server.cpp`)
- Port to other displays or boards

---

## 📄 License

This project is open-source. See the [LICENSE](LICENSE) file for details.

---

## ⚠️ Disclaimer

This project is **not a medical device** and must not be used for medical decisions.

Always rely on your official CGM device and your healthcare provider's advice for diabetes management.

---

## ❤️ Acknowledgments

- The CGM and open-source diabetes community
- Projects like xDrip+, Nightscout, and LibreLink Up
- All contributors and testers

---

## ⭐ Support the Project

If this project is useful to you:
- ⭐ Star the repository
- Share it with the diabetes DIY community
- Build your own and contribute improvements!
