#include "DisplayHelper.h"
#include <stdarg.h>

// Static member initialization
RGB_LED DisplayHelper::led;
bool DisplayHelper::initialized = false;
char DisplayHelper::lineBuffer[4][32];

void DisplayHelper::begin() {
    if (!initialized) {
        Screen.init();
        Screen.clean();
        initialized = true;
        
        // Clear line buffers
        for (int i = 0; i < 4; i++) {
            memset(lineBuffer[i], 0, sizeof(lineBuffer[i]));
        }
        
        Serial.println("[DisplayHelper] Initialized");
    }
}

void DisplayHelper::clear() {
    Screen.clean();
    for (int i = 0; i < 4; i++) {
        memset(lineBuffer[i], 0, sizeof(lineBuffer[i]));
    }
}

void DisplayHelper::print(int line, const char* text, bool clearLine) {
    if (line < 0 || line > 3) return;
    
    if (clearLine) {
        memset(lineBuffer[line], 0, sizeof(lineBuffer[line]));
    }
    
    strncpy(lineBuffer[line], text, sizeof(lineBuffer[line]) - 1);
    Screen.print(line, lineBuffer[line]);
}

void DisplayHelper::printf(int line, const char* format, ...) {
    if (line < 0 || line > 3) return;
    
    char buffer[32];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    print(line, buffer);
}

void DisplayHelper::showStatus(const char* status) {
    print(3, status);
}

void DisplayHelper::showSensorData(float temp, float humidity, float pressure) {
    printf(1, "T:%.1fC H:%.0f%%", temp, humidity);
    printf(2, "P:%.0fmbar", pressure);
}

void DisplayHelper::setLED(Color color) {
    switch (color) {
        case RED:     led.setColor(255, 0, 0); break;
        case GREEN:   led.setColor(0, 255, 0); break;
        case BLUE:    led.setColor(0, 0, 255); break;
        case YELLOW:  led.setColor(255, 255, 0); break;
        case CYAN:    led.setColor(0, 255, 255); break;
        case MAGENTA: led.setColor(255, 0, 255); break;
        case WHITE:   led.setColor(255, 255, 255); break;
        case OFF:
        default:      led.turnOff(); break;
    }
}

void DisplayHelper::setLED(uint8_t r, uint8_t g, uint8_t b) {
    led.setColor(r, g, b);
}

void DisplayHelper::ledOff() {
    led.turnOff();
}

void DisplayHelper::blinkLED(Color color, int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        setLED(color);
        delay(delayMs);
        ledOff();
        delay(delayMs);
    }
}

void DisplayHelper::showWiFiConnecting() {
    print(3, "WiFi connecting...");
    setLED(YELLOW);
}

void DisplayHelper::showWiFiConnected(const char* ip) {
    printf(3, "WiFi: %s", ip);
    blinkLED(GREEN, 2);
}

void DisplayHelper::showMQTTConnecting() {
    print(3, "MQTT connecting...");
    setLED(CYAN);
}

void DisplayHelper::showMQTTConnected() {
    print(3, "MQTT connected!");
    blinkLED(GREEN, 3);
}

void DisplayHelper::showError(const char* error) {
    print(3, error);
    blinkLED(RED, 5, 100);
}
