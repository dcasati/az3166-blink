#ifndef EASY_DISPLAY_H
#define EASY_DISPLAY_H

#include <Arduino.h>
#include "DisplayHelper.h"

/**
 * EasyDisplay - Super simple display control for kids!
 * 
 * Show text and control the LED with easy functions.
 */
class EasyDisplay {
public:
    /**
     * Initialize the display (optional - called automatically)
     */
    static void begin();

    /**
     * Write text on the screen
     * @param line - Line number (1, 2, 3, or 4)
     * @param text - What to show
     */
    static void write(int line, const char* text);

    /**
     * Write text with a number
     * @param line - Line number (1, 2, 3, or 4)
     * @param text - Text to show
     * @param number - Number to show after text
     */
    static void writeNumber(int line, const char* text, float number);

    /**
     * Clear the screen
     */
    static void clear();

    /**
     * Set LED color
     * @param color - "red", "green", "blue", "yellow", "purple", "cyan", "white", or "off"
     */
    static void setLED(const char* color);

    /**
     * Set LED with RGB values (0-255)
     */
    static void setLED(int red, int green, int blue);

    /**
     * Turn LED off
     */
    static void ledOff();

    /**
     * Blink the LED
     * @param color - Color name
     * @param times - How many times to blink
     */
    static void blink(const char* color, int times = 3);

    /**
     * Show a smiley face pattern with the LED
     */
    static void showHappy();

    /**
     * Show a sad face pattern with the LED
     */
    static void showSad();

    /**
     * Rainbow LED effect
     */
    static void rainbow();

private:
    static bool initialized;
    static void init();
    static DisplayHelper::Color getColorFromName(const char* colorName);
};

#endif // EASY_DISPLAY_H
