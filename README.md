# ESP32-S3 Remote Control with Multi-Page UI

**Dieses Projekt ist in Bearbeitung (WIP)**

**Das Projekt läuft mit ESP32 Core 3.3.3+, das nach derzeitigem Stand nur von der Arduino-IDE unterstützt wird!**


**Professionelle Fernsteuerung für Kettenfahrwerke mit ESP-NOW, Touch-Display und SD-Logging**

---

## 📋 Projekt-Übersicht

Vollständig ausgestattete, batteriegetriebene Fernsteuerung für Kettenfahrzeuge. Hardware basiert auf **ESP32-S3-N16R8** mit **4" TFT-Touchdisplay (ST7796)**, analogem Joystick und umfassendem SD-Logging. Kommunikation via **ESP-NOW** (2.4 GHz).

### Hauptmerkmale

- **Multi-Page Touch-UI** mit zentralem UILayout (Header/Footer-System)
- **ESP-NOW Kommunikation** mit TLV-Protokoll
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
ESP32-Remote-Control/
├── ESP32-Remote-Control.ino      # Hauptprogramm
├── include/                       # Alle Header-Dateien
│   ├── setupConf.h               # Hardware-Konstanten (NICHT ÄNDERN!)
│   ├── userConf.h                # User-Defaults (überschreibbar)
│   ├── Globals.h                 # Globale Objekt-Definitionen
│   ├── Core System Headers
│   │   ├── DisplayHandler.h
│   │   ├── TouchManager.h
│   │   ├── BatteryMonitor.h
│   │   ├── JoystickHandler.h
│   │   ├── SDCardHandler.h
│   │   ├── LogHandler.h
│   │   ├── PowerManager.h
│   │   └── SerialCommandHandler.h
│   ├── Communication Headers
│   │   ├── ESPNowManager.h
│   │   ├── ESPNowRemoteController.h
│   │   └── ESPNowPacket.h
│   ├── Configuration Headers
│   │   ├── ConfigManager.h
│   │   └── UserConfig.h
│   ├── UI System Headers
│   │   ├── UIManager.h
│   │   ├── PageManager.h
│   │   ├── UILayout.h
│   │   ├── UIPage.h
│   │   ├── UIElement.h
│   │   └── UI-Widgets (UIButton.h, UILabel.h, etc.)
│   └── Pages Headers
│       ├── HomePage.h
│       ├── RemoteControlPage.h
│       ├── ConnectionPage.h
│       ├── SettingsPage.h
│       └── InfoPage.h
└── *.cpp                         # ALLE .cpp im Root (Arduino-IDE!)
    ├── DisplayHandler.cpp
    ├── TouchManager.cpp
    ├── BatteryMonitor.cpp
    ├── JoystickHandler.cpp
    ├── ... (alle Implementation-Dateien)
```

**Wichtig für Arduino-IDE:** 
- Alle `.h` Dateien in `/include/`
- Alle `.cpp` Dateien im **Root-Verzeichnis** (Arduino-IDE erlaubt keine Unterordner für .cpp)

### Konfigurationssystem

- **setupConf.h**: Hardware-Konstanten (GPIO-Pins, Display-Settings, SPI-Frequenzen) - **NICHT ÄNDERN**
- **userConf.h**: User-Defaults (Backlight, Touch-Kalibrierung, ESP-NOW, Joystick)
- **config.json**: Runtime-Config auf SD-Karte (überschreibt userConf.h)
- **UserConfig-Klasse**: Runtime Config-Management mit Validierung

### UI-Architektur

```
PageManager
├── UILayout (einmalig erstellt)
│   ├── Header (0-40px)    → Zurück-Button, Titel, Battery-Icon
│   ├── Content (40-280px) → Dynamischer Bereich für Pages
│   └── Footer (280-320px) → Status-Text
├── UIManager (Widget-Verwaltung)
└── Pages (nur Content-Bereich)
    ├── HomePage
    ├── RemoteControlPage
    ├── ConnectionPage
    ├── SettingsPage
    └── InfoPage
```

**Wichtig:** 
- Header/Footer werden EINMAL vom UILayout erstellt
- Pages verwalten NUR den Content-Bereich (40-300px)
- PageManager besitzt UILayout und koordiniert alles

### Multi-Threading (FreeRTOS)

```
Core 0: WiFi/ESP-NOW
└── ESP-NOW Hardware-Callbacks (RX/TX)

Core 1: Main Loop
├── Display & UI Updates
├── Touch Event Handling
├── Joystick-Auslesen (kontinuierlich, 100ms)
├── Battery Monitoring (1s Intervall)
└── ESP-NOW Queue Processing
```

**Wichtig:** Kein separater Worker-Task - ESP-NOW nutzt Hardware-Callbacks direkt.

---

## 📡 ESP-NOW Kommunikation

### TLV-Protokoll

```
[MAIN_CMD 1B] [TOTAL_LEN 1B] [SUB_CMD 1B] [LEN 1B] [DATA...] ...
```

**Beispiel - Joystick-Daten senden:**
```cpp
RemoteESPNowPacket packet;
packet.begin(MainCmd::DATA_REQUEST)
      .addJoystick(joyX, joyY, btnPressed);

espNow.send(peerMac, packet);
```

### Kontinuierliche Übertragung

Joystick-Daten werden **kontinuierlich** gesendet (alle 100ms), nicht nur bei Änderungen. Dies verhindert, dass das Fahrzeug mit alten Kommandos weiterfährt, wenn der Joystick zurück in Neutralstellung geht.

### Vordefinierte Commands

| MainCmd | Beschreibung |
|---------|--------------|
| `HEARTBEAT` | Keep-Alive (alle 500ms) |
| `DATA_REQUEST` | Joystick/Sensor-Daten |
| `DATA_RESPONSE` | Telemetrie vom Fahrzeug |

| DataCmd | Typ | Beschreibung |
|---------|-----|--------------|
| `JOYSTICK_X/Y` | int16_t | -100 bis +100 |
| `JOYSTICK_BTN` | uint8_t | 0/1 |
| `JOYSTICK_ALL` | struct | X, Y, Button |
| `MOTOR_LEFT/RIGHT` | int16_t | -100 bis +100 |
| `MOTOR_ALL` | struct | Left, Right |
| `BATTERY_VOLTAGE` | uint16_t | mV |
| `BATTERY_PERCENT` | uint8_t | 0-100% |

---

## 💾 SD-Karte Features

### Logging (Linux-style Format)

Alle Logs im Verzeichnis `/logs/`:

```
// /logs/boot.log
[2024-12-21 14:32:01] [INFO] [BOOT] Boot started: reason=PowerOn
[2024-12-21 14:32:02] [INFO] [BOOT] Init Display: OK

// /logs/battery.log
[2024-12-21 14:33:00] [INFO] [BATTERY] voltage=7.85V, percent=78%

// /logs/connection.log
[2024-12-21 14:32:05] [INFO] [ESPNOW] Peer connected: AA:BB:CC:DD:EE:FF

// /logs/error.log
[2024-12-21 14:35:00] [ERROR] [Touch] XPT2046 timeout (code=2)
```

### Konfiguration (config.json)

Speicherort: Root der SD-Karte

```json
{
  "backlight_default": 128,
  "touch_min_x": 300,
  "touch_max_x": 3800,
  "touch_min_y": 300,
  "touch_max_y": 3800,
  "touch_threshold": 40,
  "espnow_heartbeat": 500,
  "espnow_timeout": 30000,
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
- **Joystick**: X/Y (raw + mapped), Neutral
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

# Board Manager: "esp32" by Espressif (v3.3.3+)

# Libraries (via Library Manager):
- TFT_eSPI (v2.5.43+)
- XPT2046_Touchscreen (v1.4+)
- ArduinoJson (v7.x)
```

### 2. Board-Einstellungen

```
Board: "4D Systems gen4-ESP32 Modules"
Flash Size: 16MB (128Mb)
PSRAM: "OPI PSRAM"
Partition Scheme: "16M Flash (3MB APP/9.9MB FATFS)"
Upload Speed: 921600
Core Debug Level: "None" (oder "Info" für Debugging)
```

### 3. Hardware Verkabelung

#### Backlight-Schaltung (NPN+PNP)

**Wichtig:** NPN+PNP Kombination für **normale Logik** (HIGH = AN)

```
GPIO16 → 1kΩ → NPN-Basis (2N2222A)
NPN-Emitter → GND
NPN-Kollektor → PNP-Emitter (2N3906)
PNP-Basis → 10kΩ → +3.3V
PNP-Kollektor → TFT_BL+ (Display Backlight)
TFT_BL- → GND
10kΩ Pull-Up (PNP-Basis → 3.3V)
220Ω Strombegrenzung (optional, PNP-Kollektor)
```

**Funktionsweise:**
- GPIO16 HIGH → NPN leitet → PNP-Basis LOW → **Backlight AN**
- GPIO16 LOW → NPN sperrt → PNP-Basis HIGH → **Backlight AUS**
- PWM auf GPIO16 → Helligkeitssteuerung (0-255)

**Siehe Schaltplan im Repo:** [esp32_backlight_npn_pnp1.jpg](esp32_backlight_npn_pnp1.jpg)

### 4. Code hochladen

```bash
Sketch → Upload
```

### 5. SD-Karte vorbereiten

```bash
# 1. FAT32 formatieren (max. 32GB)
# 2. Optional: config.json im Root erstellen
# 3. In SD-Slot einlegen
# 4. Beim ersten Boot werden /logs/ automatisch erstellt
```

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
# Log-Management
logs                    # Log-Dateien auflisten
read <file>            # Log-Datei lesen
tail <file> <n>        # Letzte N Zeilen
head <file> <n>        # Erste N Zeilen
clear <file>           # Log-Datei löschen
clearall               # ALLE Logs löschen

# System-Informationen
sysinfo                # Hardware/System-Info
battery                # Battery-Status
espnow                 # ESP-NOW Status

# Konfiguration
config                 # Komplette Config anzeigen
config list            # Alle Config-Keys
config get <key>       # Config-Wert abrufen
config set <key> <val> # Config-Wert setzen
config save            # Config auf SD speichern
config reset           # Standard-Config laden

# Hilfe
help                   # Alle Befehle anzeigen
```

**Beispiel:**
```
> tail /logs/boot.log 5
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
- UILayout: 1x Header/Footer (zentral via PageManager)
- Pages: Nur Content-Bereich (40-280px)
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
Deadzone:       [-5 ... +5] → 0 (5% default)

// Verhindert Drift durch ADC-Rauschen
// Einstellbar in userConf.h: JOY_DEADZONE_PERCENT
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
| **Display schwarz** | Backlight-Schaltung prüfen (NPN+PNP), GPIO16 |
| **Touch reagiert nicht** | TOUCH_CS = GPIO5? Kalibrierung in config.json |
| **Joystick driftet** | Center-Kalibrierung, Deadzone erhöhen |
| **ESP-NOW disconnected** | MAC korrekt? Kanal in userConf.h (Standard: 2) |
| **SD-Karte Error** | FAT32? CS-Pin (GPIO38)? |
| **Auto-Shutdown** | LiPo laden! (< 6.6V) |
| **UI crasht** | PSRAM aktiviert? Heap-Speicher prüfen |
| **Kompilier-Fehler** | ESP32 Core 3.0.0 - 3.3.3+ installiert? |

---

## 🔧 Anpassungen

### Joystick-Empfindlichkeit

```cpp
// userConf.h
#define JOY_UPDATE_INTERVAL  20    // 20ms = 50Hz (Standard)
#define JOY_DEADZONE_PERCENT 5     // 5% Deadzone (Standard)
```

### ESP-NOW Heartbeat

```cpp
// userConf.h
#define ESPNOW_HEARTBEAT_INTERVAL 500   // ms
#define ESPNOW_TIMEOUT            30000 // ms (30s)
#define ESPNOW_CHANNEL            2     // WiFi-Kanal
```

### Display-Helligkeit

```cpp
// userConf.h
#define BACKLIGHT_DEFAULT 20  // 0-255 (20 = niedrig für Stromsparen)
```

---

## 📂 Projektstruktur (Arduino-IDE kompatibel)

```
ESP32-Remote-Control/
├── ESP32-Remote-Control.ino      # Hauptprogramm
├── include/                       # Alle Header-Dateien
│   ├── setupConf.h
│   ├── userConf.h
│   ├── Globals.h
│   └── [alle anderen .h Dateien]
├── BatteryMonitor.cpp            # ⚠️ ALLE .cpp im Root!
├── ConfigManager.cpp
├── ConnectionPage.cpp
├── DisplayHandler.cpp
├── ESPNowManager.cpp
├── ESPNowPacket.cpp
├── ESPNowRemoteController.cpp
├── Globals.cpp
├── HomePage.cpp
├── InfoPage.cpp
├── JoystickHandler.cpp
├── LogHandler.cpp
├── PageManager.cpp
├── PowerManager.cpp
├── RemoteControlPage.cpp
├── SDCardHandler.cpp
├── SerialCommandHandler.cpp
├── SettingsPage.cpp
├── TouchManager.cpp
├── UI*.cpp                       # Alle UI-Widget .cpp
├── UserConfig.cpp
├── README.md
└── LICENSE
```

**Wichtig:** Arduino-IDE erlaubt keine Unterordner für `.cpp` Dateien!

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
