# ESP32-S3 Remote Control with Multi-Page UI

**Professionelle Fernsteuerung für Kettenfahrzeuge mit ESP-NOW, Touch-Display und SD-Logging**

---

## 📋 Projekt-Übersicht

Vollständig ausgestattete, batteriegetriebene Fernsteuerung für Kettenfahrzeuge. Hardware basiert auf **ESP32-S3-N16R8** mit **4" TFT-Touchdisplay (ST7796)**, analogem Joystick und umfassendem SD-Logging. Kommunikation via **ESP-NOW** (2.4 GHz).

### Hauptmerkmale

- **Multi-Page Touch-UI** mit zentralem Header/Footer-System
- **ESP-NOW Kommunikation** mit TLV-Protokoll und kontinuierlicher Datenübertragung
- **2S LiPo Batterie-Monitoring** mit Auto-Shutdown-Schutz
- **Analoger 2-Achsen Joystick** mit Deadzone & Center-Kalibrierung
- **SD-Karte Logging** (Boot, Battery, Connection, Errors)
- **JSON-Konfiguration** via SD-Karte mit Runtime-Management
- **Serial Command Interface** für Debugging und System-Management

---

## 🔧 Hardware

### Hauptkomponenten

| Komponente | Modell | Beschreibung |
|------------|--------|--------------|
| **Microcontroller** | ESP32-S3-N16R8 | 240MHz Dual-Core, 16MB Flash, 8MB PSRAM |
| **Display** | ST7796 4" TFT | 480x320 Pixel, 65K Farben, SPI |
| **Touch** | XPT2046 | Resistiver Touch-Controller, IRQ-Support |
| **SD-Karte** | MicroSD | Logging & Konfiguration |
| **Joystick** | 2-Achsen analog | 12-Bit ADC (0-4095), Taster |
| **Batterie** | 2S LiPo | 7.4V nominal (6.6V - 8.4V) |
| **Spannungssensor** | 0-25V Modul | Batterie-Monitoring mit ADC |
| **Backlight** | 2N2222A NPN + 2N3906 PNP | PWM-gesteuert (0-255), normale Logik |
| **Step-Down** | 2x XL4015 | 5V + 3.3V Schienen |

### Pinbelegung

#### Display & Touch (HSPI)
```
TFT_CS    = GPIO10   | TOUCH_CS  = GPIO5
TFT_DC    = GPIO9    | TOUCH_IRQ = GPIO6
TFT_MOSI  = GPIO11   | Shared MOSI/MISO/SCK
TFT_MISO  = GPIO13   | (HSPI Bus)
TFT_SCK   = GPIO12   |
TFT_BL    = GPIO16   | PWM via NPN+PNP Schaltung
```

#### SD-Karte (VSPI - separater Bus!)
```
SD_CS   = GPIO38
SD_MOSI = GPIO40
SD_MISO = GPIO41
SD_SCK  = GPIO39
```

#### Joystick & Sensoren
```
JOY_X         = GPIO1  (ADC)
JOY_Y         = GPIO2  (ADC)
JOY_BTN       = GPIO42 (Digital)
VOLTAGE_SENSE = GPIO4  (ADC, 0-25V Modul)
```

---

## 💻 Software-Architektur

### Modulares Design

```
ESP32-Remote-Control.ino
├── Core System
│   ├── DisplayHandler           // TFT + Backlight + UIManager
│   ├── TouchManager             // XPT2046 mit IRQ & Kalibrierung
│   ├── BatteryMonitor           // Spannungsmessung + Auto-Shutdown
│   ├── JoystickHandler          // 2-Achsen ADC mit Deadzone
│   ├── SDCardHandler            // SD-Karte Mount + Operations
│   ├── LogHandler               // JSON-Logging mit Kategorien
│   ├── PowerManager             // Deep-Sleep + Wake-up
│   └── SerialCommandHandler     // USB Debug-Interface
├── Communication
│   ├── ESPNowManager            // Basis ESP-NOW Framework
│   ├── ESPNowRemoteController   // Remote-spezifische Implementierung
│   └── ESPNowPacket             // TLV-Protokoll Handler
├── Configuration
│   ├── ConfigManager            // JSON Config laden/speichern
│   ├── UserConfig               // Runtime Config-Management
│   ├── setupConf.h              // Hardware-Konstanten
│   └── userConf.h               // User-Defaults
├── UI System
│   ├── UIManager                // Widget-Management
│   ├── PageManager              // Multi-Page Navigation
│   ├── GlobalUI (Globals.cpp)   // Zentrales Header/Footer
│   └── Widgets: Button, Label, Slider, ProgressBar, CheckBox, etc.
└── Pages
    ├── HomePage                 // Startseite mit Navigation
    ├── RemoteControlPage        // Joystick-Steuerung
    ├── ConnectionPage           // ESP-NOW Pairing
    ├── SettingsPage             // System-Einstellungen
    └── InfoPage                 // System-Informationen
```

### Konfigurationssystem

- **setupConf.h**: Hardware-Konstanten (GPIO-Pins, Display-Settings) - NICHT ÄNDERN
- **userConf.h**: User-Defaults (Backlight, Touch-Kalibrierung, ESP-NOW)
- **config.json**: Runtime-Config auf SD-Karte (überschreibt userConf.h)
- **UserConfig-Klasse**: Runtime Config-Management mit Schema-Validierung

### Multi-Threading (FreeRTOS)

```
Core 0: WiFi/ESP-NOW
└── ESP-NOW Hardware-Callbacks

Core 1: Main Loop
├── Display & UI Updates (primär)
├── Touch Event Handling
├── Joystick-Auslesen (kontinuierlich, 20ms)
├── Battery Monitoring (1s Intervall)
└── ESP-NOW Queue Processing
```

**Wichtig:** Worker-Task entfernt - ESP-NOW nutzt Hardware-Callbacks für optimale Performance.

---

## 📡 ESP-NOW Kommunikation

### TLV-Protokoll

```
[MAIN_CMD 1B] [TOTAL_LEN 1B] [SUB_CMD 1B] [LEN 1B] [DATA...] ...
```

**Beispiel - Joystick-Daten senden:**
```cpp
EspNowPacket packet;
packet.begin(MainCmd::DATA_REQUEST)
      .addInt16(DataCmd::JOYSTICK_X, joyX)
      .addInt16(DataCmd::JOYSTICK_Y, joyY)
      .addByte(DataCmd::JOYSTICK_BTN, btnState);

espnow.send(peerMac, packet);
```

### Kontinuierliche Übertragung

**Kritisch:** Joystick-Daten werden kontinuierlich gesendet, nicht nur bei Änderungen. Dies verhindert, dass das Fahrzeug mit alten Kommandos weiterfährt, wenn der Joystick in Neutralstellung zurückkehrt.

### Vordefinierte Commands

| MainCmd | Beschreibung |
|---------|--------------|
| `HEARTBEAT` | Keep-Alive alle 500ms |
| `DATA_REQUEST` | Joystick/Sensor-Daten |
| `DATA_RESPONSE` | Telemetrie vom Fahrzeug |

| DataCmd | Typ | Beschreibung |
|---------|-----|--------------|
| `JOYSTICK_X/Y` | int16_t | -100 bis +100 |
| `MOTOR_LEFT/RIGHT` | int16_t | -100 bis +100 |
| `BATTERY_VOLTAGE` | uint16_t | mV |
| `BATTERY_PERCENT` | uint8_t | 0-100% |
| `BUTTON_STATE` | uint8_t | Bitmask |

---

## 💾 SD-Karte Features

### Logging (Linux-style Format)

```
// boot.log
[2024-12-21 14:32:01] [INFO] [BOOT] Boot started: reason=PowerOn
[2024-12-21 14:32:02] [INFO] [BOOT] Init Display: OK

// battery.log
[2024-12-21 14:33:00] [INFO] [BATTERY] voltage=7.85V, percent=78%

// connection.log
[2024-12-21 14:32:05] [INFO] [ESPNOW] Peer connected: AA:BB:CC:DD:EE:FF

// error.log
[2024-12-21 14:35:00] [ERROR] [Touch] XPT2046 timeout (code=2)
```

### Konfiguration (config.json)

```json
{
  "backlight_default": 128,
  "touch_min_x": 100,
  "touch_max_x": 4000,
  "touch_min_y": 100,
  "touch_max_y": 4000,
  "touch_threshold": 600,
  "espnow_heartbeat": 500,
  "espnow_timeout": 2000,
  "autoshutdown": true,
  "debug_serial": true
}
```

---

## 🎮 UI-Seiten

### 1. HomePage
- Willkommensbildschirm mit Navigation
- Live-Status: Remote-Battery & ESP-NOW Connection
- Buttons: Remote Control, Connection, Settings, System Info

### 2. RemoteControlPage
- **Joystick-Visualisierung** (2D-Kreis mit Position)
- **X/Y-Werte** live (-100 bis +100)
- **Connection-Status** (Connected/Disconnected)
- **Fahrzeug-Battery** (ProgressBar + Spannung)

### 3. ConnectionPage
- **ESP-NOW Pairing/Unpairing**
- **MAC-Adressen** (Remote + Peer)
- **Status**: Disconnected / Paired / Connected
- **Buttons**: PAIR, DISCONNECT

### 4. SettingsPage
- **Backlight-Slider** (PWM 0-255, live)
- **Auto-Shutdown** (CheckBox)
- **Joystick-Kalibrierung** (Center-Button)

### 5. InfoPage
- **System-Info**: Hardware, Display, Battery, SD-Karte
- **ESP-NOW**: Status, Connected
- **Joystick**: X/Y (raw + mapped), Neutral-Status
- **System**: Free Heap, Uptime
- **Refresh-Button**

---

## 🔋 Batterie-Management

### Auto-Shutdown

```cpp
if (currentVoltage <= VOLTAGE_SHUTDOWN) {  // 6.6V = 3.3V/Zelle
    logger.logError("Battery", ERR_BATTERY_CRITICAL);
    esp_deep_sleep_start();  // ESP32 ausschalten
}
```

### Warnstufen

| Spannung | Status | Aktion |
|----------|--------|--------|
| 8.4V - 7.0V | ✅ OK | Grün |
| 7.0V - 6.6V | ⚡ LOW | Orange, Warnung |
| ≤ 6.6V | ⚠️ CRITICAL | Rot, **Auto-Shutdown** |

---

## 🚀 Installation & Setup

### 1. Arduino IDE Vorbereitung

```bash
# ESP32 Board Package URL:
https://espressif.github.io/arduino-esp32/package_esp32_index.json

# Board Manager: "esp32" by Espressif (v3.0.0+)

# Libraries (via Library Manager):
- TFT_eSPI (v2.5.43+)
- XPT2046_Touchscreen (v1.4+)
- ArduinoJson (v7.x)
```

### 2. Board-Einstellungen

```
Board: "ESP32S3 Dev Module"
Flash Size: 16MB (128Mb)
PSRAM: "OPI PSRAM"
Partition Scheme: "16M Flash (3MB APP/9.9MB FATFS)"
Upload Speed: 921600
```

### 3. Hardware Verkabelung

#### Backlight-Schaltung (NPN+PNP)

```
GPIO16 → 1kΩ → NPN-Basis (2N2222A)
NPN-Emitter → GND
NPN-Kollektor → PNP-Emitter (2N3906)
PNP-Basis → 10kΩ → +3.3V
PNP-Kollektor → TFT_BL+ (Display Backlight)
```

**Funktionsweise:** GPIO16 HIGH → Backlight AN (normale Logik)

---

## 📊 Debugging & Monitoring

### Serial Monitor (115200 Baud)

```
╔════════════════════════════════════════╗
║   ESP32-S3 Remote Control Startup      ║
╚════════════════════════════════════════╝

→ SD-Card... OK
→ Display... OK
→ Touch... OK
→ Battery... OK (7.85V, 78%)
→ Joystick... OK
→ ESP-NOW... OK (MAC: AA:BB:CC:DD:EE:FF)

Setup complete! (1234 ms)
```

### SerialCommandHandler

**Verfügbare Befehle:**
```bash
logs                    # Log-Dateien auflisten
read <file>            # Log-Datei lesen
tail <file> <n>        # Letzte N Zeilen
sysinfo                # System-Informationen
battery                # Battery-Status
config get <key>       # Config-Wert abrufen
config set <key> <val> # Config-Wert setzen
```

**Beispiel:**
```
> tail boot.log 5
[2024-12-21 14:32:05] [INFO] [BOOT] Init ESP-NOW: OK
[2024-12-21 14:32:05] [INFO] [BOOT] Boot complete: 2345ms

> battery
Battery Status:
  Voltage: 7.85V
  Percent: 78%
  Status: OK
```

---

## 📐 Technische Details

### Memory Management

```cpp
Flash:  16MB (Code + SPIFFS)
PSRAM:  8MB  (UI-Widgets, Buffers)
SRAM:   512KB (Stack, Heap)

Optimierungen:
- GlobalUI: 1x Header/Footer (zentral, nicht pro Page)
- UI-Widgets: Managed via UIManager
- ESP-NOW: Hardware-Callbacks (keine Worker-Threads)
- JSON: ArduinoJson V7
```

### Power Consumption (gemessen)

```
Display Backlight (max): ~200mA @ 3.3V
ESP32-S3 Active:         ~100mA @ 5V
Display + Touch:         ~50mA @ 5V
-------------------------------------------
TOTAL (ohne ESP-NOW):    ~130mA @ 8.4V

2S LiPo 3000mAh:
Laufzeit: ~16-17 Stunden (gemessen)
```

### Joystick Deadzone

```cpp
Raw ADC:    0 ───── 2048 ───── 4095
Mapped:   -100 ───── 0 ───── +100
Deadzone:       [-10 ... +10] → 0

// Verhindert Drift durch ADC-Rauschen
```

---

## 🎯 Verwendung

### Erstinbetriebnahme

1. **Remote einschalten** → HomePage
2. **Connection** → Eigene MAC notieren
3. **Fahrzeug einschalten**
4. **Connection** → Peer MAC eingeben
5. **PAIR** drücken
6. Warten auf "Connected"
7. **Remote Control** → Fahrzeug steuern

### Troubleshooting

| Problem | Lösung |
|---------|--------|
| **Display schwarz** | Backlight-Schaltung prüfen, GPIO16 |
| **Touch reagiert nicht** | TOUCH_CS = GPIO5? Kalibrierung |
| **Joystick driftet** | Center-Kalibrierung, Deadzone |
| **ESP-NOW disconnected** | MAC korrekt? Kanal 0? |
| **SD-Karte Error** | FAT32? CS-Pin? |
| **Auto-Shutdown** | LiPo laden! (< 6.6V) |

---

## 🔧 Anpassungen

### Joystick-Empfindlichkeit

```cpp
joystick.setUpdateInterval(20);  // 20ms = 50Hz (Standard)
joystick.setUpdateInterval(50);  // 50ms = 20Hz (langsamer)
```

### ESP-NOW Heartbeat

```cpp
// userConf.h oder config.json
#define ESPNOW_HEARTBEAT_INTERVAL 500  // ms
#define ESPNOW_TIMEOUT_MS 2000         // ms
```

---

## 📜 Lizenz

MIT License - Siehe [LICENSE](LICENSE)

---

## 🙏 Credits

- **TFT_eSPI** by Bodmer
- **XPT2046_Touchscreen** by Paul Stoffregen
- **ArduinoJson** by Benoit Blanchon
- **ESP-NOW** Framework by Espressif

---

## 📧 Kontakt

**Entwickelt für ein Kettenfahrzeug-Projekt**

Issues & Verbesserungsvorschläge willkommen!