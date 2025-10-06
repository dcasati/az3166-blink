/*
 * WS2812B LED Test - Find the Right Pin!
 * 
 * This program will test different pins to find which one works
 * with your WS2812B LED. Watch the LED and Serial Monitor.
 */

#include <Arduino.h>
#include <EasyLEDStrip.h>

// Pins to test
int testPins[] = {PA_15, PB_0, PB_5, PA_8, PC_6, PB_4, PA_5, PB_3};
String pinNames[] = {"PA_15", "PB_0", "PB_5", "PA_8", "PC_6", "PB_4", "PA_5", "PB_3"};
int numPins = 8;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n========================================");
    Serial.println("  WS2812B LED Pin Finder");
    Serial.println("========================================");
    Serial.println("\nThis will test different pins.");
    Serial.println("Watch your LED - it should turn RED when");
    Serial.println("we find the right pin!\n");
    delay(2000);
}

void loop() {
    for(int i = 0; i < numPins; i++) {
        Serial.print("\n--- Testing pin ");
        Serial.print(pinNames[i]);
        Serial.println(" ---");
        
        // Initialize with this pin
        EasyLEDStrip::begin(testPins[i], 1);
        
        // Try RED
        Serial.println("Sending RED...");
        EasyLEDStrip::setAll("red");
        delay(2000);
        
        // Try GREEN
        Serial.println("Sending GREEN...");
        EasyLEDStrip::setAll("green");
        delay(2000);
        
        // Try BLUE
        Serial.println("Sending BLUE...");
        EasyLEDStrip::setAll("blue");
        delay(2000);
        
        // Turn off
        Serial.println("OFF");
        EasyLEDStrip::clear();
        delay(1000);
        
        Serial.println("\nDid you see colors? If YES, this is your pin!");
        Serial.println("If NO, trying next pin...\n");
        delay(2000);
    }
    
    Serial.println("\n\n========================================");
    Serial.println("Finished testing all pins.");
    Serial.println("Did any of them work?");
    Serial.println("========================================\n");
    Serial.println("If none worked, the issue might be:");
    Serial.println("1. Wiring - check connections");
    Serial.println("2. Power - is LED getting 5V?");
    Serial.println("3. LED might be faulty");
    Serial.println("4. Timing needs adjustment");
    Serial.println("\nRestarting test in 5 seconds...\n");
    
    delay(5000);
}
