/*
 * SUPER EASY AZ3166 Example for Kids!
 * 
 * This example shows you how easy it is to:
 * 1. Show text on the screen
 * 2. Read temperature
 * 3. Change LED colors
 * 4. Detect motion
 * 
 * Made for 8-year-olds to learn programming!
 */

#include <Arduino.h>
#include "EasyDisplay.h"
#include "EasySensors.h"

void setup() {
    // Start the serial monitor so we can see messages
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Hello! The board is starting...");
    
    // Show a message on the screen
    EasyDisplay::write(1, "Hello!");
    EasyDisplay::write(2, "I'm ready!");
    
    // Make the LED green for 2 seconds
    EasyDisplay::setLED("green");
    delay(2000);
    EasyDisplay::ledOff();
    
    // Show a rainbow!
    EasyDisplay::rainbow();
}

void loop() {
    // Read the temperature
    float temp = EasySensors::getTemperature();
    
    // Show it on the screen
    EasyDisplay::write(1, "Temperature:");
    EasyDisplay::writeNumber(2, "Temp: ", temp);
    
    // Print it to the computer
    Serial.print("Temperature is: ");
    Serial.print(temp);
    Serial.println(" degrees");
    
    // If it's hot, turn LED red
    if (temp > 25) {
        EasyDisplay::setLED("red");
        EasyDisplay::write(3, "It's hot!");
    }
    // If it's cold, turn LED blue
    else if (temp < 20) {
        EasyDisplay::setLED("blue");
        EasyDisplay::write(3, "It's cold!");
    }
    // If it's just right, turn LED green
    else {
        EasyDisplay::setLED("green");
        EasyDisplay::write(3, "Just right!");
    }
    
    // Check if someone is moving the board
    if (EasySensors::isMoving()) {
        Serial.println("Hey! Someone is moving me!");
        EasyDisplay::blink("yellow", 2);
    }
    
    // Wait 2 seconds before checking again
    delay(2000);
}
