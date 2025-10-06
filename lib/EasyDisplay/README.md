# Kid-Friendly Libraries for AZ3166

Super simple libraries designed for 8-year-olds to program the AZ3166 like an Arduino!

## Libraries

### EasyDisplay
Control the screen and LED with simple commands.

**Functions:**
- `write(line, text)` - Write text on screen (lines 1-4)
- `writeNumber(line, text, number)` - Write text with a number
- `setLED(color)` - Change LED color ("red", "green", "blue", etc.)
- `ledOff()` - Turn LED off
- `blink(color, times)` - Blink the LED
- `showHappy()` - Happy pattern
- `showSad()` - Sad pattern
- `rainbow()` - Rainbow effect

### EasySensors
Read temperature, humidity, and detect motion.

**Functions:**
- `getTemperature()` - Get temperature in Celsius
- `getHumidity()` - Get humidity percentage
- `getPressure()` - Get air pressure
- `isMoving()` - Check if board is moving
- `isTilted()` - Check if board is tilted
- `getTiltAngle()` - Get tilt angle in degrees
- `showOnDisplay()` - Show all sensors on screen
- `printAll()` - Print all sensors to Serial

### EasyMQTT
Send messages to the internet (needs WiFi).

**Functions:**
- `connect(server, deviceName)` - Connect to internet messaging
- `isConnected()` - Check if connected
- `sendMessage(topic, message)` - Send a text message
- `sendNumber(topic, number)` - Send a number
- `sendSensorData(topic, deviceName)` - Send all sensor data

## Quick Start

```cpp
#include <Arduino.h>
#include "EasyDisplay.h"
#include "EasySensors.h"

void setup() {
    Serial.begin(115200);
    EasyDisplay::write(1, "Hello!");
    EasyDisplay::setLED("green");
}

void loop() {
    float temp = EasySensors::getTemperature();
    EasyDisplay::writeNumber(2, "Temp: ", temp);
    delay(1000);
}
```

## Examples

- **01_SuperEasy** - Learn basics (temperature, LED, display)
- **02_WiFiAndInternet** - Connect to WiFi and send data online
- **03_MotionGame** - Fun game counting movements

## For Parents

These libraries hide complexity and provide:
- Simple, intuitive function names
- No pointers or memory management
- Clear error messages
- Safe defaults
- Educational value

Perfect for introducing kids to:
- Programming logic
- Sensors and IoT
- Conditional statements (if/else)
- Variables and data types
- Internet connectivity

See KIDS_GUIDE.md for the complete tutorial for children.
