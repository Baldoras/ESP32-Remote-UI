/**
 * UIEventHandler.cpp
 */

#include "include/UIEventHandler.h"

UIEventHandler::UIEventHandler() {
    clear();
}

UIEventHandler::~UIEventHandler() {
    clear();
}

void UIEventHandler::on(EventType type, EventCallback callback) {
    int idx = eventToIndex(type);
    if (idx >= 0 && idx < 11) {
        callbacks[idx] = callback;
    }
}

void UIEventHandler::off(EventType type) {
    int idx = eventToIndex(type);
    if (idx >= 0 && idx < 11) {
        callbacks[idx] = nullptr;
    }
}

void UIEventHandler::clear() {
    for (int i = 0; i < 11; i++) {
        callbacks[i] = nullptr;
    }
}

void UIEventHandler::trigger(EventType type, EventData* data) {
    int idx = eventToIndex(type);
    if (idx >= 0 && idx < 11 && callbacks[idx]) {
        callbacks[idx](data);
    }
}

bool UIEventHandler::hasHandler(EventType type) {
    int idx = eventToIndex(type);
    return (idx >= 0 && idx < 11 && callbacks[idx] != nullptr);
}

int UIEventHandler::eventToIndex(EventType type) {
    return static_cast<int>(type);
}