#include "EasyDisplay.h"

// Static member initialization
bool EasyDisplay::initialized = false;

void EasyDisplay::begin() {
    init();
}

void EasyDisplay::init() {
    if (!initialized) {
        DisplayHelper::begin();
        initialized = true;
    }
}

void EasyDisplay::write(int line, const char* text) {
    init();
    // Convert to 0-based index
    DisplayHelper::print(line - 1, text);
}

void EasyDisplay::writeNumber(int line, const char* text, float number) {
    init();
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s%.1f", text, number);
    DisplayHelper::print(line - 1, buffer);
}

void EasyDisplay::clear() {
    init();
    DisplayHelper::clear();
}

DisplayHelper::Color EasyDisplay::getColorFromName(const char* colorName) {
    String color = String(colorName);
    color.toLowerCase();
    
    if (color == "red") return DisplayHelper::RED;
    if (color == "green") return DisplayHelper::GREEN;
    if (color == "blue") return DisplayHelper::BLUE;
    if (color == "yellow") return DisplayHelper::YELLOW;
    if (color == "cyan") return DisplayHelper::CYAN;
    if (color == "purple" || color == "magenta") return DisplayHelper::MAGENTA;
    if (color == "white") return DisplayHelper::WHITE;
    return DisplayHelper::OFF;
}

void EasyDisplay::setLED(const char* color) {
    init();
    DisplayHelper::setLED(getColorFromName(color));
}

void EasyDisplay::setLED(int red, int green, int blue) {
    init();
    DisplayHelper::setLED(red, green, blue);
}

void EasyDisplay::ledOff() {
    init();
    DisplayHelper::ledOff();
}

void EasyDisplay::blink(const char* color, int times) {
    init();
    DisplayHelper::blinkLED(getColorFromName(color), times);
}

void EasyDisplay::showHappy() {
    init();
    // Happy pattern: blink green 3 times
    DisplayHelper::blinkLED(DisplayHelper::GREEN, 3, 150);
}

void EasyDisplay::showSad() {
    init();
    // Sad pattern: blink red slowly 3 times
    DisplayHelper::blinkLED(DisplayHelper::RED, 3, 500);
}

void EasyDisplay::rainbow() {
    init();
    // Rainbow cycle
    DisplayHelper::setLED(DisplayHelper::RED);
    delay(200);
    DisplayHelper::setLED(DisplayHelper::YELLOW);
    delay(200);
    DisplayHelper::setLED(DisplayHelper::GREEN);
    delay(200);
    DisplayHelper::setLED(DisplayHelper::CYAN);
    delay(200);
    DisplayHelper::setLED(DisplayHelper::BLUE);
    delay(200);
    DisplayHelper::setLED(DisplayHelper::MAGENTA);
    delay(200);
    DisplayHelper::ledOff();
}
