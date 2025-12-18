/**
 * UIEventHandler.h
 * 
 * Event-System für UI-Elemente
 * Verwaltet Callbacks für verschiedene Event-Types
 */

#ifndef UI_EVENT_HANDLER_H
#define UI_EVENT_HANDLER_H

#include <Arduino.h>
#include <functional>

// Event-Daten Struktur
struct EventData {
    int16_t x;          // X-Koordinate (bei Touch)
    int16_t y;          // Y-Koordinate (bei Touch)
    int value;          // Wert (bei onChange)
    int oldValue;       // Alter Wert (bei onChange)
    void* userData;     // Optionale Benutzerdaten
};

// Event-Types
enum class EventType {
    NONE = 0,
    CLICK,          // Element wurde geklickt
    PRESS,          // Touch begonnen
    RELEASE,        // Touch beendet
    HOVER,          // Touch über Element (während gedrückt)
    LEAVE,          // Touch verlässt Element
    VALUE_CHANGED,         // Wert geändert (Slider, ProgressBar)
    DRAG_START,     // Drag begonnen (Slider)
    DRAG_END,       // Drag beendet (Slider)
    FOCUS,          // Element fokussiert
    BLUR            // Element verliert Fokus
};

// Callback-Typ
typedef std::function<void(EventData*)> EventCallback;

class UIEventHandler {
public:
    UIEventHandler();
    ~UIEventHandler();

    void on(EventType type, EventCallback callback);
    void off(EventType type);
    void clear();
    void trigger(EventType type, EventData* data = nullptr);
    bool hasHandler(EventType type);

private:
    EventCallback callbacks[11];  // NONE bis BLUR = 11 Events
    int eventToIndex(EventType type);
};

#endif // UI_EVENT_HANDLER_H