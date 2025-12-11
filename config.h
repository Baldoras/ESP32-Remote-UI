/**
 * config.h
 * 
 * Konfigurationsdatei für ESP32-S3 mit ST7796 Display, XPT2046 Touch und SD-Karte
 * 
 * Hardware:
 * - ESP32-S3-N16R8
 * - ST7796 Display (480x320) auf HSPI
 * - XPT2046 Touch Controller auf HSPI
 * - SD-Karte Reader auf VSPI
 * - PN2222A NPN Transistor für Backlight-Steuerung
 */

#ifndef CONFIG_H
#define CONFIG_H

// ═══════════════════════════════════════════════════════════════════════════
// 🖥️ ST7796 DISPLAY PINS (HSPI)
// ═══════════════════════════════════════════════════════════════════════════

#define TFT_CS      10    // Display Chip Select (LOW = aktiv)
#define TFT_DC      9     // Display Data/Command (LOW = Command, HIGH = Data)
#define TFT_RST     -1    // Display Reset (direkt an 3.3V - kein GPIO!)
#define TFT_MOSI    11    // Display MOSI (HSPI, shared mit Touch)
#define TFT_MISO    13    // Display MISO (HSPI, shared mit Touch)
#define TFT_SCK     12    // Display Clock (HSPI, shared mit Touch)
#define TFT_BL      16    // Display Backlight (PWM via PN2222A Transistor)

// ═══════════════════════════════════════════════════════════════════════════
// 👆 XPT2046 TOUCH PINS (HSPI)
// ═══════════════════════════════════════════════════════════════════════════

#define TOUCH_CS    5     // Touch Chip Select (LOW = aktiv)
#define TOUCH_IRQ   6    // Touch Interrupt (LOW = Touch erkannt)
// Touch teilt MOSI, MISO, SCK mit Display (HSPI Bus)!
#define TOUCH_MOSI  TFT_MOSI  // = GPIO11 (HSPI)
#define TOUCH_MISO  TFT_MISO  // = GPIO13 (HSPI)
#define TOUCH_CLK   TFT_SCK   // = GPIO12 (HSPI)

// ═══════════════════════════════════════════════════════════════════════════
// 💾 SD-KARTE PINS (VSPI - eigener Bus!)
// ═══════════════════════════════════════════════════════════════════════════

#define SD_CS       38    // SD-Karte Chip Select (LOW = aktiv)
#define SD_MOSI     40    // SD MOSI (VSPI)
#define SD_MISO     41    // SD MISO (VSPI)
#define SD_SCK      39    // SD SCK (VSPI)

// ═══════════════════════════════════════════════════════════════════════════
// ⚡ DISPLAY EINSTELLUNGEN
// ═══════════════════════════════════════════════════════════════════════════

#define DISPLAY_WIDTH     480   // Display Breite in Pixeln
#define DISPLAY_HEIGHT    320   // Display Höhe in Pixeln
#define DISPLAY_ROTATION    3      // Display-Rotation (0-3)

// SPI-Geschwindigkeiten
#define TFT_SPI_FREQUENCY    27000000  // 27 MHz für Display
#define TOUCH_SPI_FREQUENCY  2500000   // 2,5 MHz für Touch
#define SD_SPI_FREQUENCY     25000000  // 25 MHz für SD

// ═══════════════════════════════════════════════════════════════════════════
// 🔋 SPANNUNGSSENSOR (0-25V Modul, 2S LiPo Messung mit Auto-Shutdown)
// ═══════════════════════════════════════════════════════════════════════════

#define VOLTAGE_SENSOR_PIN    4     // Analog OUT vom Spannungssensor-Modul (GPIO4)
#define VOLTAGE_RANGE_MAX     25.0  // Modul-Maximum (Hardware-Limit)
#define VOLTAGE_BATTERY_MIN   6.6   // 2S LiPo leer (3.3V/Zelle)
#define VOLTAGE_BATTERY_MAX   8.4   // 2S LiPo voll (4.2V/Zelle)
#define VOLTAGE_BATTERY_NOM   7.4   // 2S LiPo nominal (3.7V/Zelle)
#define VOLTAGE_ALARM_LOW     7.0   // Warnung bei <7.0V (3.5V/Zelle)
#define VOLTAGE_SHUTDOWN      6.6   // AUTO-SHUTDOWN bei 6.6V! ⚠️
#define VOLTAGE_CALIBRATION_FACTOR  0.7 // Calibration factor, da das messmodul von 12V ausgeht
#define VOLTAGE_CHECK_INTERVAL 1000 // Spannungs-Check alle 1000ms

// ═══════════════════════════════════════════════════════════════════════════
// ⚡ BACKLIGHT EINSTELLUNGEN (PWM)
// ═══════════════════════════════════════════════════════════════════════════

#define BACKLIGHT_PWM_CHANNEL  0      // PWM-Kanal für Backlight
#define BACKLIGHT_PWM_FREQ     25000   // PWM-Frequenz 5kHz
#define BACKLIGHT_PWM_RES      8      // 8-Bit Auflösung (0-255)
#define BACKLIGHT_DEFAULT      20     // Standard-Helligkeit (0-255)
#define BACKLIGHT_MAX          255    // Maximale Helligkeit
#define BACKLIGHT_MIN          20     // Minimale Helligkeit

// ═══════════════════════════════════════════════════════════════════════════
// 📡 ESP-NOW EINSTELLUNGEN
// ═══════════════════════════════════════════════════════════════════════════

#define ESPNOW_MAX_PEERS          5                   // Maximale Anzahl Peers
#define ESPNOW_CHANNEL            0                   // WiFi-Kanal (0 = auto)
#define ESPNOW_HEARTBEAT_INTERVAL 500                 // Heartbeat alle 500ms
#define ESPNOW_TIMEOUT_MS         2000                // Verbindungs-Timeout 2s
#define ESPNOW_PEER_DEVICE_MAC    "11:22:33:44:55:66" // Peer device MAC

// ═══════════════════════════════════════════════════════════════════════════
// 👆 TOUCH EINSTELLUNGEN
// ═══════════════════════════════════════════════════════════════════════════

// Touch-Kalibrierung (nach erstem Start anpassen!)
#define TOUCH_MIN_X  1
#define TOUCH_MAX_X  4095
#define TOUCH_MIN_Y  1
#define TOUCH_MAX_Y  4095

#define TOUCH_ROTATION  1      // Touch-Rotation (0-3)
#define TOUCH_THRESHOLD 40     // Mindestdruck für Touch-Erkennung

// ═══════════════════════════════════════════════════════════════════════════
// JOYSTICK EINSTELLUNGEN
// ═══════════════════════════════════════════════════════════════════════════

#define JOY_PIN_X 1
#define JOY_PIN_Y 2
#define JOY_PIN_BTN 42


// ═══════════════════════════════════════════════════════════════════════════
// 🎨 FARBEN (RGB565 Format)
// ═══════════════════════════════════════════════════════════════════════════

#define COLOR_BLACK    0x0000
#define COLOR_WHITE    0xFFFF
#define COLOR_RED      0xF800
#define COLOR_GREEN    0x07E0
#define COLOR_BLUE     0x001F
#define COLOR_CYAN     0x07FF
#define COLOR_MAGENTA  0xF81F
#define COLOR_YELLOW   0xFFE0
#define COLOR_ORANGE   0xFD20
#define COLOR_PURPLE   0x780F
#define COLOR_GRAY     0x7BEF
#define COLOR_DARKGRAY 0x39E7

// ═══════════════════════════════════════════════════════════════════════════
// 🎮 UI EINSTELLUNGEN
// ═══════════════════════════════════════════════════════════════════════════

// Status Bar (oben)
#define UI_STATUSBAR_HEIGHT     30
#define UI_STATUSBAR_COLOR      COLOR_DARKGRAY
#define UI_STATUSBAR_TEXT_COLOR COLOR_WHITE

// Battery Icon Position (Status Bar rechts)
#define UI_BATTERY_X            420
#define UI_BATTERY_Y            5
#define UI_BATTERY_WIDTH        50
#define UI_BATTERY_HEIGHT       20

// Navigation Bar (unten)
#define UI_NAVBAR_HEIGHT        40
#define UI_NAVBAR_COLOR         COLOR_DARKGRAY
#define UI_NAVBAR_BUTTON_WIDTH  100

// Content Area
#define UI_CONTENT_Y            UI_STATUSBAR_HEIGHT
#define UI_CONTENT_HEIGHT       (DISPLAY_HEIGHT - UI_STATUSBAR_HEIGHT - UI_NAVBAR_HEIGHT)

// ═══════════════════════════════════════════════════════════════════════════
// 🔧 DEBUG EINSTELLUNGEN
// ═══════════════════════════════════════════════════════════════════════════

#define DEBUG_SERIAL        true    // Debug-Ausgaben aktivieren
#define SERIAL_BAUD_RATE    115200  // Serielle Baudrate

// Debug-Makros
#if DEBUG_SERIAL
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINTF(...)  Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

// ═══════════════════════════════════════════════════════════════════════════
// ⚙️ SYSTEM EINSTELLUNGEN
// ═══════════════════════════════════════════════════════════════════════════

#define DEBOUNCE_DELAY      50     // Entprell-Zeit in ms
#define SD_MOUNT_POINT     "/sd"   // Mount-Punkt für SD-Karte
#define SD_MAX_FILES       10      // Maximale Anzahl offener Dateien

// ═══════════════════════════════════════════════════════════════════════════
// 🛡️ FEHLERCODES
// ═══════════════════════════════════════════════════════════════════════════

enum ErrorCode {
    ERR_NONE = 0,
    ERR_DISPLAY_INIT = 1,
    ERR_TOUCH_INIT = 2,
    ERR_SD_INIT = 3,
    ERR_SD_MOUNT = 4,
    ERR_FILE_OPEN = 5,
    ERR_FILE_WRITE = 6,
    ERR_FILE_READ = 7,
    ERR_BATTERY_INIT = 8,
    ERR_BATTERY_CRITICAL = 9
};

// ═══════════════════════════════════════════════════════════════════════════
// 📝 VERSION INFO
// ═══════════════════════════════════════════════════════════════════════════

#define FIRMWARE_VERSION "1.0.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

#endif // CONFIG_H
