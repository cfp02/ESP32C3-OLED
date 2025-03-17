#include "OLEDScreen.h"

OLEDScreen::OLEDScreen() : u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5) {
    verticalScrollEnabled = false;
    horizontalScrollEnabled = false;
    scrollX = 0;
    scrollY = 0;
    lastScrollY = 0;
    scrollBufferIndex = 0;
}

OLEDScreen::OLEDScreen(uint8_t clockPin, uint8_t dataPin) 
    : u8g2(U8G2_R0, U8X8_PIN_NONE, clockPin, dataPin) {
    verticalScrollEnabled = false;
    horizontalScrollEnabled = false;
    scrollX = 0;
    scrollY = 0;
    lastScrollY = 0;
    scrollBufferIndex = 0;
}

void OLEDScreen::begin() {
    u8g2.begin();
    u8g2.setContrast(255);
    u8g2.setBusClock(400000);
}

void OLEDScreen::setContrast(uint8_t contrast) {
    u8g2.setContrast(contrast);
}

void OLEDScreen::setBusClock(uint32_t clockSpeed) {
    u8g2.setBusClock(clockSpeed);
}

void OLEDScreen::setFont(const uint8_t* font) {
    u8g2.setFont(font);
}

void OLEDScreen::clear() {
    u8g2.clearBuffer();
}

void OLEDScreen::update() {
    u8g2.sendBuffer();
}

void OLEDScreen::print(const char* text) {
    u8g2.print(text);
}

void OLEDScreen::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    u8g2.printf(format, args);
    va_end(args);
}

void OLEDScreen::setCursor(int16_t x, int16_t y) {
    u8g2.setCursor(xOffset + x, yOffset + y);
}

void OLEDScreen::drawFrame(int16_t x, int16_t y, int16_t w, int16_t h) {
    u8g2.drawFrame(xOffset + x, yOffset + y, w, h);
}

void OLEDScreen::drawBox(int16_t x, int16_t y, int16_t w, int16_t h) {
    u8g2.drawBox(xOffset + x, yOffset + y, w, h);
}

void OLEDScreen::drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    u8g2.drawLine(xOffset + x1, yOffset + y1, xOffset + x2, yOffset + y2);
}

void OLEDScreen::enableVerticalScroll(bool enable) {
    verticalScrollEnabled = enable;
    if (!enable) {
        scrollY = 0;
        lastScrollY = 0;
    }
}

void OLEDScreen::enableHorizontalScroll(bool enable) {
    horizontalScrollEnabled = enable;
    if (!enable) {
        scrollX = 0;
    }
}

void OLEDScreen::setScrollOffset(int16_t x, int16_t y) {
    scrollX = x;
    scrollY = y;
}

void OLEDScreen::scrollText(const char* text, int16_t y) {
    if (strlen(text) > 71) {
        // Text is too long, truncate it
        strncpy(scrollBuffer[scrollBufferIndex], text, 71);
        scrollBuffer[scrollBufferIndex][71] = '\0';
    } else {
        strcpy(scrollBuffer[scrollBufferIndex], text);
    }
    
    scrollBufferIndex = (scrollBufferIndex + 1) % MAX_SCROLL_LINES;
}

void OLEDScreen::scrollTextVertical(const char* text) {
    if (!verticalScrollEnabled) return;
    
    // Add new text to buffer
    scrollText(text, 0);
    
    // Clear display
    clear();
    
    // Calculate which lines to show based on scroll position
    int startLine = scrollY / 10;  // Assuming 10 pixels per line
    int visibleLines = ScreenHeight / 10;
    
    // Draw visible lines
    for (int i = 0; i < visibleLines; i++) {
        int bufferIndex = (scrollBufferIndex - visibleLines + i + MAX_SCROLL_LINES) % MAX_SCROLL_LINES;
        setCursor(0, i * 10);
        print(scrollBuffer[bufferIndex]);
    }
    
    update();
}

void OLEDScreen::scrollTextHorizontal(const char* text) {
    if (!horizontalScrollEnabled) return;
    
    clear();
    int textWidth = strlen(text) * 6;  // Assuming 6 pixels per character
    int scrollPosition = scrollX % textWidth;
    
    // Draw text at scroll position
    setCursor(-scrollPosition, 0);
    print(text);
    
    // If text is longer than screen, draw it again to create continuous scroll
    if (textWidth > ScreenWidth) {
        setCursor(-scrollPosition + textWidth, 0);
        print(text);
    }
    
    update();
}

void OLEDScreen::clearScrollBuffer() {
    scrollBufferIndex = 0;
    for (int i = 0; i < MAX_SCROLL_LINES; i++) {
        scrollBuffer[i][0] = '\0';
    }
} 