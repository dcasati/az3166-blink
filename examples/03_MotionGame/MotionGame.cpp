/*
 * Motion Detector Game!
 * 
 * This fun example:
 * 1. Detects when you move the board
 * 2. Counts how many times you moved it
 * 3. Shows fun colors and messages
 * 
 * Try shaking the board and see what happens!
 */

#include <Arduino.h>
#include "EasyDisplay.h"
#include "EasySensors.h"

int moveCount = 0;
bool wasMoving = false;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Motion Detector Game Starting!");
    
    EasyDisplay::write(1, "Motion Game!");
    EasyDisplay::write(2, "Shake me!");
    EasyDisplay::rainbow();
}

void loop() {
    // Check if the board is moving RIGHT NOW
    bool isMovingNow = EasySensors::isMoving();
    
    // Did we just START moving? (we weren't moving before, but now we are)
    if (isMovingNow && !wasMoving) {
        moveCount++;  // Count this movement
        
        Serial.print("Movement detected! Count: ");
        Serial.println(moveCount);
        
        // Show the count on screen
        EasyDisplay::write(3, "Moves:");
        EasyDisplay::writeNumber(4, "Count: ", moveCount);
        
        // Flash different colors based on count
        if (moveCount % 3 == 0) {
            EasyDisplay::setLED("red");
        } else if (moveCount % 3 == 1) {
            EasyDisplay::setLED("green");
        } else {
            EasyDisplay::setLED("blue");
        }
        
        // Every 10 moves, show a rainbow!
        if (moveCount % 10 == 0) {
            EasyDisplay::write(2, "10 moves!");
            EasyDisplay::rainbow();
        }
    }
    
    // Turn LED off when not moving
    if (!isMovingNow) {
        EasyDisplay::ledOff();
    }
    
    // Remember if we were moving for next time
    wasMoving = isMovingNow;
    
    // Check tilt angle
    float angle = EasySensors::getTiltAngle();
    if (angle > 45) {
        EasyDisplay::write(2, "Tilted!");
    } else {
        EasyDisplay::write(2, "Flat");
    }
    
    // Small delay
    delay(100);
}
