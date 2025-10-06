# EasyWebServer Library

Control your AZ3166 board from any web browser! This library creates a simple web server that lets you change LED colors and write messages to the screen.

## What Can You Do?

- Change LED colors by clicking colorful buttons
- Write custom messages to the display
- Make fun rainbow effects
- Make the LED blink
- Control everything from your phone, tablet, or computer

## How to Use It

### Step 1: Connect to WiFi

Before starting the web server, you need to connect to WiFi:

```cpp
#include <EasyWebServer.h>

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi first
    WiFi.begin("YourWiFiName", "YourPassword");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected!");
    
    // Start the web server
    EasyWebServer::begin();
}

void loop() {
    // Check for visitors to your web page
    EasyWebServer::handleRequests();
}
```

### Step 2: Find Your Board's Web Address

After connecting, the serial monitor will show something like:

```
Web server started!
Go to: http://192.168.1.100
```

Type that address in your web browser to see the control panel!

## All the Functions

### EasyWebServer::begin()
Starts the web server on port 80 (the default web port)

```cpp
EasyWebServer::begin();
```

### EasyWebServer::begin(port)
Starts the web server on a custom port

```cpp
EasyWebServer::begin(8080);  // Use port 8080 instead
```

### EasyWebServer::handleRequests()
Checks for people visiting your web page. Put this in loop()!

```cpp
void loop() {
    EasyWebServer::handleRequests();
}
```

### EasyWebServer::getURL()
Gets the web address as a string

```cpp
String address = EasyWebServer::getURL();
Serial.println(address);  // Prints: http://192.168.1.100
```

### EasyWebServer::showURL()
Shows the web address on line 4 of the display

```cpp
EasyWebServer::showURL();  // Display shows: 192.168.1.100
```

### EasyWebServer::isRunning()
Checks if the web server is running

```cpp
if (EasyWebServer::isRunning()) {
    Serial.println("Server is running!");
}
```

### EasyWebServer::stop()
Stops the web server

```cpp
EasyWebServer::stop();
```

## What's on the Web Page?

The control panel has three sections:

### 1. LED Colors
Click colorful circles to change the LED:
- Red, Green, Blue, Yellow, Cyan, Purple, White
- OFF button to turn the LED off

### 2. Write Messages
Type messages and send them to any of the 3 lines on the display:
- Line 1
- Line 2
- Line 3

### 3. Fun Effects
- Rainbow: Makes the LED cycle through colors
- Blink Red/Green/Blue: Makes the LED flash
- Clear Display: Erases all messages

## Complete Example

```cpp
#include <EasyDisplay.h>
#include <EasyWebServer.h>

void setup() {
    Serial.begin(115200);
    
    EasyDisplay::begin();
    EasyDisplay::write(1, "Connecting...");
    
    // Connect to WiFi
    WiFi.begin("MyHomeWiFi", "MyPassword123");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    EasyDisplay::clear();
    EasyDisplay::write(1, "WiFi OK!");
    EasyDisplay::write(2, "Starting web...");
    delay(1000);
    
    // Start web server
    EasyWebServer::begin();
    
    EasyDisplay::clear();
    EasyDisplay::write(1, "Ready!");
    EasyDisplay::write(2, "Open browser:");
    EasyWebServer::showURL();  // Shows IP on line 4
    
    EasyDisplay::setLED("green");
}

void loop() {
    EasyWebServer::handleRequests();
}
```

## Tips for Parents

1. Make sure your computer/phone is on the same WiFi network as the board
2. The IP address might change if the board restarts
3. Only one person can control the board at a time, but multiple people can view the page
4. The board remembers the last color/message even after closing the browser

## How It Works

The library creates a tiny web server on the board. When someone visits the web address:

1. The board sends them a colorful control panel (HTML page)
2. When they click a button, their browser sends a command to the board
3. The board receives the command and changes the LED or display
4. They can send as many commands as they want

It's like having a remote control for your board that works through the internet!
