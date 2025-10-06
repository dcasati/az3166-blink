/*
 * Example 04: Web Control Panel
 * 
 * This program turns your AZ3166 board into a web server!
 * You can control the LED colors and display messages from
 * any web browser on your phone, tablet, or computer.
 * 
 * What you'll learn:
 * - How to create a web server on your board
 * - How to control your board from a browser
 * - How to make interactive web pages
 * 
 * Instructions:
 * 1. Change YOUR_WIFI_NAME and YOUR_PASSWORD below
 * 2. Upload this program to your board
 * 3. Open the Serial Monitor to see the web address
 * 4. Type that address in your web browser
 * 5. Click buttons to control the LED and display!
 * 
 * Make sure your computer/phone is on the same WiFi network!
 */

#include <Arduino.h>
#include <AZ3166WiFi.h>
#include <EasyDisplay.h>
#include <EasyWebServer.h>

// CHANGE THESE TO YOUR WIFI SETTINGS
char WIFI_NAME[] = "YOUR_WIFI_NAME";
char WIFI_PASSWORD[] = "YOUR_PASSWORD";

void setup() {
    // Start serial communication
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=================================");
    Serial.println("  Web Control Panel Starting!");
    Serial.println("=================================\n");
    
    // Initialize the display
    EasyDisplay::begin();
    EasyDisplay::clear();
    EasyDisplay::write(1, "Web Control");
    EasyDisplay::write(2, "Connecting WiFi...");
    EasyDisplay::setLED("yellow");
    
    // Connect to WiFi
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_NAME);
    
    WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        if (attempts > 60) {  // 30 seconds timeout
            Serial.println("\nFailed to connect!");
            Serial.println("Check your WiFi name and password!");
            EasyDisplay::write(2, "WiFi Failed!");
            EasyDisplay::write(3, "Check settings");
            EasyDisplay::setLED("red");
            while(1) { delay(1000); }  // Stop here
        }
    }
    
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Update display
    EasyDisplay::clear();
    EasyDisplay::write(1, "WiFi Connected!");
    EasyDisplay::setLED("green");
    delay(1500);
    
    // Start the web server
    Serial.println("\nStarting web server...");
    EasyDisplay::write(2, "Starting web...");
    
    EasyWebServer::begin();
    
    // Show the web address
    EasyDisplay::clear();
    EasyDisplay::write(1, "Ready!");
    EasyDisplay::write(2, "Open browser:");
    EasyDisplay::write(3, "Go to:");
    EasyWebServer::showURL();
    
    Serial.println("\n=================================");
    Serial.println("  SERVER IS READY!");
    Serial.println("=================================");
    Serial.print("\nOpen your web browser and go to:\n");
    Serial.println(EasyWebServer::getURL());
    Serial.println("\nYou can now control the board from your browser!");
    Serial.println("Try clicking the color buttons or typing messages!\n");
}

void loop() {
    // Keep checking for web requests
    EasyWebServer::handleRequests();
    
    // That's it! The web server handles everything else.
    // Your board is now a web server that can be controlled
    // from any device on your WiFi network!
}

/*
 * TROUBLESHOOTING:
 * 
 * Problem: Can't connect to WiFi
 * Solution: Double-check your WiFi name and password are correct
 * 
 * Problem: Can't access the web page
 * Solution: Make sure your computer/phone is on the same WiFi network
 * 
 * Problem: IP address doesn't work
 * Solution: The IP might change when you restart the board. Check Serial Monitor
 * 
 * Problem: Web page loads slowly
 * Solution: This is normal - the board is small and serves pages slowly
 * 
 * THINGS TO TRY:
 * 
 * 1. Open the web page on your phone and computer at the same time
 * 2. Try all the different LED colors
 * 3. Type your name on line 1 of the display
 * 4. Make the LED do the rainbow effect
 * 5. Have a friend control the board from their phone while you watch!
 * 
 * WHAT'S HAPPENING:
 * 
 * Your board is running a tiny web server! When you visit the web address:
 * - Your browser asks the board for a web page
 * - The board sends HTML code (the web page)
 * - Your browser shows the colorful buttons
 * - When you click, your browser sends a command to the board
 * - The board receives the command and changes the LED or display
 * 
 * It's like the board has its own website that you can visit!
 */
