/*
 * Example 05: LED Strip Fun
 * 
 * Control your WS2812B LED strip with easy functions!
 * This example shows different colors and effects.
 * 
 * What you need:
 * - WS2812B LED strip connected to Pin 0
 * - For 1-5 LEDs: Can power from AZ3166 (safe!)
 * - For many LEDs: Need external 5V power supply
 * 
 * This example uses 3 LEDs which is safe to power
 * from the board without external power.
 * 
 * What you'll learn:
 * - How to control LED strips
 * - How to make colorful effects
 * - How to create light shows
 */

#include <Arduino.h>
#include <EasyLEDStrip.h>

// How many LEDs on your strip?
const int NUM_LEDS = 3;  // 3 LEDs is safe to power from the board!

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=================================");
    Serial.println("  LED Strip Fun Starting!");
    Serial.println("=================================\n");
    
    // Start the LED strip
    // PB_3 is the pin that works with your WS2812B!
    EasyLEDStrip::begin(PB_3, NUM_LEDS);
    
    // Set brightness to 100% since we only have 3 LEDs
    // (3 LEDs is safe to power from the board)
    EasyLEDStrip::setBrightness(255);
    
    Serial.println("LED Strip is ready!");
    Serial.println("Using pin PB_3");
    Serial.println("Watch the colorful show!\n");
}

void loop() {
    Serial.println("Red...");
    EasyLEDStrip::setAll("red");
    delay(2000);
    
    Serial.println("Green...");
    EasyLEDStrip::setAll("green");
    delay(2000);
    
    Serial.println("Blue...");
    EasyLEDStrip::setAll("blue");
    delay(2000);
    
    Serial.println("Yellow...");
    EasyLEDStrip::setAll("yellow");
    delay(2000);
    
    Serial.println("Purple...");
    EasyLEDStrip::setAll("purple");
    delay(2000);
    
    Serial.println("Cyan...");
    EasyLEDStrip::setAll("cyan");
    delay(2000);
    
    Serial.println("Rainbow!");
    EasyLEDStrip::rainbow();
    delay(3000);
    
    Serial.println("Rainbow Cycle!");
    EasyLEDStrip::rainbowCycle(5);  // The number controls speed
    
    Serial.println("Theater Chase - Red");
    EasyLEDStrip::theaterChase("red");
    
    Serial.println("Theater Chase - Blue");
    EasyLEDStrip::theaterChase("blue");
    
    Serial.println("Blink White");
    EasyLEDStrip::blink("white", 5);
    
    Serial.println("Off for a moment...");
    EasyLEDStrip::clear();
    delay(1000);
    
    Serial.println("\nStarting over!\n");
}

/*
 * THINGS TO TRY:
 * 
 * 1. Change NUM_LEDS to use more LEDs:
 *    const int NUM_LEDS = 10;  // Try 10 LEDs
 *    (But remember: 10+ LEDs need external power!)
 * 
 * 2. Try different colors:
 *    EasyLEDStrip::setAll("orange");
 *    EasyLEDStrip::setAll("pink");
 * 
 * 3. Change the delays to make it faster or slower:
 *    delay(500);   // Faster color changes
 *    delay(5000);  // Slower color changes
 * 
 * 4. Skip the rainbow and just show colors:
 *    Comment out the rainbow lines with //
 * 
 * WIRING FOR 1-5 LEDs (SAFE - NO EXTERNAL POWER NEEDED):
 * LED Strip:
 *   5V  → AZ3166 3.3V or 5V pin (up to 5 LEDs is safe!)
 *   GND → AZ3166 GND
 *   DIN → AZ3166 Pin PB_3 (this is the one that works!)
 * 
 * WIRING FOR MANY LEDs (NEEDS EXTERNAL POWER):
 * LED Strip:
 *   5V  → External 5V power supply (+)
 *   GND → External power supply (-) AND AZ3166 GND
 *   DIN → AZ3166 Pin PB_3
 * 
 * POWER GUIDE:
 * - 1-3 LEDs: Safe from board (~60-180mA)
 * - 4-5 LEDs: Safe at full brightness (~240mA)
 * - 6-10 LEDs: Use lower brightness or external power
 * - 10+ LEDs: MUST use external power supply
 */
