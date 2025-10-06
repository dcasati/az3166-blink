# AZ3166 For Kids - Super Easy Programming Guide!

## Welcome Young Programmer!

This guide will help you program your AZ3166 board just like an Arduino. It's super easy and fun!

## What Can You Do?

- Show messages on the screen
- Change LED colors (red, green, blue, yellow, purple, and more!)
- Read temperature and humidity
- Detect when someone moves the board
- Connect to WiFi
- Send messages to the internet

## Easy Libraries for Kids

### 1. EasyDisplay - Control the Screen and LED

Show text on the screen:
```cpp
EasyDisplay::write(1, "Hello!");
EasyDisplay::write(2, "I'm learning!");
```

Change LED colors:
```cpp
EasyDisplay::setLED("red");      // Red light
EasyDisplay::setLED("green");    // Green light
EasyDisplay::setLED("blue");     // Blue light
EasyDisplay::setLED("yellow");   // Yellow light
EasyDisplay::setLED("purple");   // Purple light
EasyDisplay::ledOff();           // Turn off
```

Make it blink:
```cpp
EasyDisplay::blink("green", 3);  // Blink green 3 times
EasyDisplay::showHappy();        // Happy pattern
EasyDisplay::showSad();          // Sad pattern
EasyDisplay::rainbow();          // Rainbow colors!
```

### 2. EasySensors - Read Temperature and Motion

Get the temperature:
```cpp
float temp = EasySensors::getTemperature();
Serial.print("Temperature: ");
Serial.println(temp);
```

Check if someone is moving the board:
```cpp
if (EasySensors::isMoving()) {
    Serial.println("I'm moving!");
    EasyDisplay::blink("yellow", 2);
}
```

Check if the board is tilted:
```cpp
if (EasySensors::isTilted()) {
    Serial.println("I'm tilted!");
}
```

Show all sensors on the screen:
```cpp
EasySensors::showOnDisplay();
```

### 3. EasyMQTT - Send Messages to the Internet

Connect to the internet messaging system:
```cpp
EasyMQTT::connect("mqtt.example.com", "MyDevice");
```

Send a message:
```cpp
EasyMQTT::sendMessage("hello", "Hi from my board!");
```

Send a number:
```cpp
float temp = EasySensors::getTemperature();
EasyMQTT::sendNumber("temperature", temp);
```

Send all sensor data:
```cpp
EasyMQTT::sendSensorData("sensors", "MyDevice");
```

### 4. EasyWebServer - Control from a Web Browser

Start a web server (after connecting to WiFi):
```cpp
EasyWebServer::begin();
```

Let people visit your web page in loop():
```cpp
void loop() {
    EasyWebServer::handleRequests();
}
```

Show the web address on the screen:
```cpp
EasyWebServer::showURL();
```

Get the web address to print it:
```cpp
String address = EasyWebServer::getURL();
Serial.println(address);
```

What can visitors do on your web page?
- Click colorful buttons to change the LED
- Type messages to show on the display
- Make rainbow effects
- Make the LED blink
- Clear the display

## Fun Project Ideas

### 1. Temperature Alert
Make the LED turn red when it's hot and blue when it's cold!

### 2. Motion Counter
Count how many times someone shakes the board!

### 3. Weather Station
Read temperature and send it to the internet every minute!

### 4. Tilt Detector
Make different colors when you tilt the board different ways!

### 5. Secret Message Sender
Send secret messages to the internet when you shake the board 3 times!

### 6. Web-Controlled LED Light
Turn your board into a web server and control the LED colors from your phone or tablet!

### 7. Remote Message Display
Let your friends send messages to your board's display through a web browser!

## Complete Example

Here's a complete program that does everything:

```cpp
#include <Arduino.h>
#include "EasyDisplay.h"
#include "EasySensors.h"

void setup() {
    Serial.begin(115200);
    
    // Say hello on the screen
    EasyDisplay::write(1, "Hello!");
    EasyDisplay::write(2, "I'm ready!");
    
    // Show a rainbow
    EasyDisplay::rainbow();
}

void loop() {
    // Get temperature
    float temp = EasySensors::getTemperature();
    
    // Show it
    EasyDisplay::writeNumber(1, "Temp: ", temp);
    
    // If hot, make it red
    if (temp > 25) {
        EasyDisplay::setLED("red");
    }
    // If cold, make it blue
    else {
        EasyDisplay::setLED("blue");
    }
    
    // If moving, blink yellow
    if (EasySensors::isMoving()) {
        EasyDisplay::blink("yellow", 2);
    }
    
    // Wait 2 seconds
    delay(2000);
}
```

## Screen Lines

The screen has 4 lines you can write on:
- Line 1 (top)
- Line 2
- Line 3
- Line 4 (bottom)

## LED Colors You Can Use

- "red" - Red light
- "green" - Green light  
- "blue" - Blue light
- "yellow" - Yellow light
- "cyan" - Light blue
- "purple" - Purple light
- "white" - White light
- "off" - Turn off

## Temperature Tips

- Normal room temperature: 20-25 degrees Celsius
- Hot day: above 25 degrees
- Cold day: below 15 degrees
- You can use this to make a thermometer!

## Motion Detection Tips

- Shake the board to trigger motion
- Tilt it to detect tilting
- Make a game that counts shakes!

## Need Help?

1. Ask your parent to help with WiFi passwords
2. Check the Serial Monitor to see messages
3. Try the example programs first
4. Have fun and experiment!

## Examples to Try

1. **SuperEasy.cpp** - Learn the basics (temperature, LED, display)
2. **WiFiAndInternet.cpp** - Connect to WiFi and internet (needs parent help)
3. **MotionGame.cpp** - Fun game that counts movements
4. **WebControl.cpp** - Control your board from a web browser! (needs parent help for WiFi)

## Safety Rules

1. Don't connect to WiFi without asking your parent
2. Don't share WiFi passwords with others
3. Only use approved MQTT servers
4. When using the web server, only let people you trust visit the web address
5. The web server only works on your home WiFi network (not the internet)
6. Have fun but be careful!

## Happy Coding!

Remember:
- Start simple
- Try the examples
- Change the numbers and see what happens
- Make your own projects
- Have FUN!

You're now a programmer! Keep learning and creating awesome projects!
