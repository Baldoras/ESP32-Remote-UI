/**
 * UITextBox.cpp
 */

#include "include/UITextBox.h"

UITextBox::UITextBox(int16_t x, int16_t y, int16_t w, int16_t h)
    : UIElement(x, y, w, h), scrollY(0), lineHeight(16), fontSize(2), 
      wordWrap(true), padding(5), scrolling(false), lastTouchY(0), scrollStartY(0) {
    
    style.bgColor = COLOR_BLACK;
    style.borderColor = COLOR_WHITE;
    style.textColor = COLOR_WHITE;
    
    maxVisibleLines = calculateMaxVisibleLines();
}

UITextBox::~UITextBox() {
}

void UITextBox::draw(TFT_eSPI* tft) {
    if (!visible) return;
    
    drawTextBox(tft);
    needsRedraw = false;
}

void UITextBox::handleTouch(int16_t tx, int16_t ty, bool isPressed) {
    if (!visible || !enabled) return;
    
    bool inside = isPointInside(tx, ty);
    
    if (isPressed && inside) {
        if (!scrolling) {
            // Scrolling starten
            scrolling = true;
            scrollStartY = ty;
            lastTouchY = ty;
        } else {
            // Scrolling fortsetzen
            int deltaY = ty - lastTouchY;
            
            if (abs(deltaY) > lineHeight / 2) {
                int scrollLines = deltaY / (lineHeight / 2);
                if (scrollLines > 0) {
                    scrollUp(scrollLines);
                } else if (scrollLines < 0) {
                    scrollDown(-scrollLines);
                }
                lastTouchY = ty;
            }
        }
    } else {
        // Touch beendet
        if (scrolling) {
            scrolling = false;
            
            // Click-Event wenn kaum gescrollt wurde
            if (inside && abs(ty - scrollStartY) < 10) {
                EventData data = {tx, ty, 0, 0, nullptr};
                eventHandler.trigger(EventType::CLICK, &data);
            }
        }
    }
}

void UITextBox::setText(const char* text) {
    lines.clear();
    scrollY = 0;
    
    if (wordWrap) {
        wrapText(text, lines);
    } else {
        // Text direkt als Zeilen speichern
        String str(text);
        int start = 0;
        int pos = 0;
        
        while (pos < str.length()) {
            if (str[pos] == '\n') {
                lines.push_back(str.substring(start, pos));
                start = pos + 1;
            }
            pos++;
        }
        
        // Letzte Zeile
        if (start < str.length()) {
            lines.push_back(str.substring(start));
        }
    }
    
    needsRedraw = true;
}

void UITextBox::appendLine(const char* line) {
    if (wordWrap) {
        std::vector<String> wrappedLines;
        wrapText(line, wrappedLines);
        for (const auto& l : wrappedLines) {
            lines.push_back(l);
        }
    } else {
        lines.push_back(String(line));
    }
    needsRedraw = true;
}

void UITextBox::appendText(const char* text) {
    std::vector<String> newLines;
    
    if (wordWrap) {
        wrapText(text, newLines);
    } else {
        String str(text);
        int start = 0;
        int pos = 0;
        
        while (pos < str.length()) {
            if (str[pos] == '\n') {
                newLines.push_back(str.substring(start, pos));
                start = pos + 1;
            }
            pos++;
        }
        
        if (start < str.length()) {
            newLines.push_back(str.substring(start));
        }
    }
    
    for (const auto& line : newLines) {
        lines.push_back(line);
    }
    
    needsRedraw = true;
}

void UITextBox::clear() {
    lines.clear();
    scrollY = 0;
    needsRedraw = true;
}

void UITextBox::scrollToBottom() {
    scrollY = getMaxScroll();
    needsRedraw = true;
}

void UITextBox::scrollToTop() {
    scrollY = 0;
    needsRedraw = true;
}

void UITextBox::setWordWrap(bool wrap) {
    wordWrap = wrap;
    needsRedraw = true;
}

void UITextBox::setLineHeight(int height) {
    lineHeight = height;
    maxVisibleLines = calculateMaxVisibleLines();
    needsRedraw = true;
}

void UITextBox::setFontSize(uint8_t size) {
    if (size >= 1 && size <= 4) {
        fontSize = size;
        lineHeight = fontSize * 8 + 4;
        maxVisibleLines = calculateMaxVisibleLines();
        needsRedraw = true;
    }
}

void UITextBox::setPadding(int pad) {
    padding = pad;
    maxVisibleLines = calculateMaxVisibleLines();
    needsRedraw = true;
}

void UITextBox::scrollUp(int scrollLines) {
    if (scrollY > 0) {
        scrollY -= scrollLines;
        if (scrollY < 0) scrollY = 0;
        needsRedraw = true;
    }
}

void UITextBox::scrollDown(int scrollLines) {
    int maxScroll = getMaxScroll();
    if (scrollY < maxScroll) {
        scrollY += scrollLines;
        if (scrollY > maxScroll) scrollY = maxScroll;
        needsRedraw = true;
    }
}

int UITextBox::getMaxScroll() const {
    int totalLines = lines.size();
    int maxScroll = totalLines - maxVisibleLines;
    return maxScroll > 0 ? maxScroll : 0;
}

void UITextBox::drawTextBox(TFT_eSPI* tft) {
    // Hintergrund
    fillRoundRect(tft, x, y, width, height, style.cornerRadius, style.bgColor);
    
    // Rahmen
    drawRoundRect(tft, x, y, width, height, style.cornerRadius, style.borderColor);
    
    // Clipping-Bereich setzen (damit Text nicht über Rahmen geht)
    tft->setViewport(x + padding, y + padding, 
                     width - 2 * padding, height - 2 * padding);
    
    // Text zeichnen
    tft->setTextSize(fontSize);
    tft->setTextColor(style.textColor, style.bgColor);
    tft->setTextDatum(TL_DATUM);
    
    int textY = 0;  // FIX: Nach setViewport() relativ!
    int endLine = scrollY + maxVisibleLines;
    if (endLine > lines.size()) endLine = lines.size();
    
    for (int i = scrollY; i < endLine; i++) {
        if (textY + lineHeight > height - 2 * padding) break;
        
        tft->drawString(lines[i].c_str(), 0, textY);  // FIX: Relativ zum Viewport!
        textY += lineHeight;
    }
    
    // Viewport zurücksetzen
    tft->resetViewport();
    
    // Scroll-Indikator zeichnen (falls mehr Zeilen vorhanden)
    if (lines.size() > maxVisibleLines) {
        int indicatorHeight = (height - 2 * padding) * maxVisibleLines / lines.size();
        if (indicatorHeight < 10) indicatorHeight = 10;
        
        int indicatorY = y + padding + ((height - 2 * padding - indicatorHeight) * scrollY) / getMaxScroll();
        
        tft->fillRect(x + width - padding - 3, indicatorY, 3, indicatorHeight, style.borderColor);
    }
}

void UITextBox::wrapText(const char* text, std::vector<String>& outLines) {
    // Temporäres TFT-Objekt für Breiten-Berechnung (nur wenn nötig)
    // Annahme: Font ist bereits gesetzt
    
    String str(text);
    int maxCharsPerLine = (width - 2 * padding) / (fontSize * 6);
    
    int start = 0;
    while (start < str.length()) {
        // Suche nach Zeilenumbruch
        int newlinePos = str.indexOf('\n', start);
        int end;
        
        if (newlinePos == -1) {
            end = str.length();
        } else {
            end = newlinePos;
        }
        
        // Zeile in Chunks aufteilen falls zu lang
        while (start < end) {
            int chunkEnd = start + maxCharsPerLine;
            if (chunkEnd > end) chunkEnd = end;
            
            // Versuche an Leerzeichen zu trennen
            if (chunkEnd < end) {
                int spacePos = str.lastIndexOf(' ', chunkEnd);
                if (spacePos > start) {
                    chunkEnd = spacePos + 1;
                }
            }
            
            outLines.push_back(str.substring(start, chunkEnd));
            start = chunkEnd;
            
            // Überspringe Leerzeichen am Anfang der neuen Zeile
            while (start < end && str[start] == ' ') start++;
        }
        
        // Überspringe Zeilenumbruch
        if (newlinePos != -1) start = newlinePos + 1;
    }
}

int UITextBox::calculateMaxVisibleLines() {
    return (height - 2 * padding) / lineHeight;
}

int UITextBox::getCharWidth(TFT_eSPI* tft) {
    return fontSize * 6;  // Approximation
}

int UITextBox::getMaxCharsPerLine(TFT_eSPI* tft) {
    return (width - 2 * padding) / getCharWidth(tft);
}