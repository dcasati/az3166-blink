#ifndef DISPLAY_HELPER_H
#define DISPLAY_HELPER_H

#include <Arduino.h>
#include "OledDisplay.h"
#include "RGB_LED.h"

/**
 * DisplayHelper - Simplified display and LED control for AZ3166
 * 
 * This library provides easy-to-use methods for:
 * - OLED display text output (4 lines)
 * - RGB LED color control
 * - Status indication patterns
 */
class DisplayHelper {
public:
    /**
     * LED color presets
     */
    enum Color {
        OFF = 0,
        RED,
        GREEN,
        BLUE,
        YELLOW,
        CYAN,
        MAGENTA,
        WHITE
    };

    /**
     * Initialize display and LED
     */
    static void begin();

    /**
     * Clear the OLED display
     */
    static void clear();

    /**
     * Print text to a specific line on OLED
     * @param line - Line number (0-3)
     * @param text - Text to display
     * @param clearLine - Clear line before printing (default: true)
     */
    static void print(int line, const char* text, bool clearLine = true);

    /**
     * Print formatted text to a line
     * @param line - Line number (0-3)
     * @param format - Printf-style format string
     * @param ... - Variable arguments
     */
    static void printf(int line, const char* format, ...);

    /**
     * Show a status message on the display
     * @param status - Status text to show on line 3
     */
    static void showStatus(const char* status);

    /**
     * Show sensor data on display
     * @param temp - Temperature in Celsius
     * @param humidity - Humidity percentage
     * @param pressure - Pressure in mbar
     */
    static void showSensorData(float temp, float humidity, float pressure);

    /**
     * Set RGB LED color using preset
     * @param color - Color preset
     */
    static void setLED(Color color);

    /**
     * Set RGB LED color using RGB values
     * @param r - Red (0-255)
     * @param g - Green (0-255)
     * @param b - Blue (0-255)
     */
    static void setLED(uint8_t r, uint8_t g, uint8_t b);

    /**
     * Turn off RGB LED
     */
    static void ledOff();

    /**
     * Blink RGB LED
     * @param color - Color to blink
     * @param times - Number of blinks
     * @param delayMs - Delay between blinks in ms
     */
    static void blinkLED(Color color, int times = 3, int delayMs = 200);

    /**
     * Show WiFi connecting animation
     */
    static void showWiFiConnecting();

    /**
     * Show WiFi connected status
     * @param ip - IP address string
     */
    static void showWiFiConnected(const char* ip);

    /**
     * Show MQTT connecting status
     */
    static void showMQTTConnecting();

    /**
     * Show MQTT connected status
     */
    static void showMQTTConnected();

    /**
     * Show error message
     * @param error - Error message
     */
    static void showError(const char* error);

private:
    static RGB_LED led;
    static bool initialized;
    static char lineBuffer[4][32];  // Buffer for 4 lines
};

#endif // DISPLAY_HELPER_H
