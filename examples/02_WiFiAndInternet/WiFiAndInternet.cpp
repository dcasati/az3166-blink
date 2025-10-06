/*
 * WiFi and Internet Example for Kids!
 * 
 * This shows how to:
 * 1. Connect to WiFi
 * 2. Send messages to the internet
 * 3. Share sensor data online
 * 
 * Ask your parent to help with WiFi password!
 */

#include <Arduino.h>
#include "EasyDisplay.h"
#include "EasySensors.h"
#include "EasyMQTT.h"
#include "AZ3166WiFi.h"

// WiFi settings - ask your parents for these!
const char* WIFI_NAME = "YourWiFiName";
const char* WIFI_PASSWORD = "YourWiFiPassword";

// MQTT server - this is like a mailbox on the internet
const char* MQTT_SERVER = "mqtt.example.com";
const char* MY_DEVICE_NAME = "MyAZ3166";

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Starting up!");
    
    // Show a message
    EasyDisplay::write(1, "Connecting...");
    
    // Connect to WiFi
    Serial.println("Connecting to WiFi...");
    WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
    
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 20) {
        delay(500);
        Serial.print(".");
        tries++;
    }
    
    // Did we connect?
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        EasyDisplay::write(2, "WiFi OK!");
        EasyDisplay::showHappy();
    } else {
        Serial.println("\nWiFi failed!");
        EasyDisplay::write(2, "WiFi failed!");
        EasyDisplay::showSad();
        while(1) delay(1000); // Stop here
    }
    
    // Connect to the internet messaging system
    if (EasyMQTT::connect(MQTT_SERVER, MY_DEVICE_NAME)) {
        EasyDisplay::write(3, "Internet OK!");
        EasyDisplay::blink("green", 3);
    }
}

void loop() {
    // Read the temperature
    float temp = EasySensors::getTemperature();
    
    // Show it on the screen
    EasyDisplay::writeNumber(1, "Temp: ", temp);
    
    // Send it to the internet!
    if (EasyMQTT::isConnected()) {
        // Send just the temperature number
        EasyMQTT::sendNumber("temperature", temp);
        
        // Or send ALL sensor data at once
        EasyMQTT::sendSensorData("sensors", MY_DEVICE_NAME);
        
        EasyDisplay::write(4, "Sent!");
        EasyDisplay::setLED("green");
        delay(100);
        EasyDisplay::ledOff();
    }
    
    // Wait 5 seconds before sending again
    delay(5000);
}
