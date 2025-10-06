# DisplayHelper Library

A simplified display and LED control library for the AZ3166 IoT DevKit.

## Features

- Easy OLED display control (4 lines)
- RGB LED color management
- Pre-defined color constants
- Status indication helpers
- Formatted text printing (printf-style)
- LED blink patterns
- Built-in WiFi/MQTT status displays

## Usage

### Basic Example

```cpp
#include "DisplayHelper.h"

void setup() {
    // Initialize display and LED
    DisplayHelper::begin();
    
    // Print text to display lines (0-3)
    DisplayHelper::print(0, "AZ3166 Device");
    DisplayHelper::print(1, "Status: Ready");
    
    // Set LED color
    DisplayHelper::setLED(DisplayHelper::GREEN);
    delay(1000);
    DisplayHelper::ledOff();
}
```

### Formatted Text

```cpp
// Printf-style formatting
float temp = 25.5;
int humidity = 60;
DisplayHelper::printf(1, "T:%.1fC H:%d%%", temp, humidity);

// Status messages
DisplayHelper::showStatus("System OK");
```

### LED Control

```cpp
// Color presets
DisplayHelper::setLED(DisplayHelper::RED);
DisplayHelper::setLED(DisplayHelper::GREEN);
DisplayHelper::setLED(DisplayHelper::BLUE);
DisplayHelper::setLED(DisplayHelper::YELLOW);
DisplayHelper::setLED(DisplayHelper::CYAN);
DisplayHelper::setLED(DisplayHelper::MAGENTA);
DisplayHelper::setLED(DisplayHelper::WHITE);
DisplayHelper::ledOff();

// Custom RGB colors
DisplayHelper::setLED(255, 128, 0);  // Orange

// Blink patterns
DisplayHelper::blinkLED(DisplayHelper::GREEN, 3, 200);  // 3 blinks, 200ms delay
```

### Status Helpers

```cpp
// WiFi status
DisplayHelper::showWiFiConnecting();
DisplayHelper::showWiFiConnected("192.168.1.100");

// MQTT status
DisplayHelper::showMQTTConnecting();
DisplayHelper::showMQTTConnected();

// Error display
DisplayHelper::showError("Connection failed");
```

### Sensor Data Display

```cpp
float temp = 25.5;
float humidity = 60.0;
float pressure = 1013.25;

DisplayHelper::showSensorData(temp, humidity, pressure);
// Line 1: "T:25.5C H:60%"
// Line 2: "P:1013mbar"
```

### Clear Display

```cpp
DisplayHelper::clear();  // Clear all lines
```

## API Reference

### Initialization

```cpp
static void begin();
```

### Display Methods

- `static void clear()` - Clear entire display
- `static void print(int line, const char* text, bool clearLine = true)` - Print to line 0-3
- `static void printf(int line, const char* format, ...)` - Printf-style formatted output
- `static void showStatus(const char* status)` - Show status on line 3
- `static void showSensorData(float temp, float humidity, float pressure)` - Display sensor readings

### LED Methods

- `static void setLED(Color color)` - Set LED using color preset
- `static void setLED(uint8_t r, uint8_t g, uint8_t b)` - Set LED with RGB values (0-255)
- `static void ledOff()` - Turn LED off
- `static void blinkLED(Color color, int times = 3, int delayMs = 200)` - Blink LED pattern

### Status Helpers

- `static void showWiFiConnecting()` - Show WiFi connecting message + yellow LED
- `static void showWiFiConnected(const char* ip)` - Show IP address + green blink
- `static void showMQTTConnecting()` - Show MQTT connecting message + cyan LED
- `static void showMQTTConnected()` - Show MQTT connected + green blink
- `static void showError(const char* error)` - Show error message + red blink

### Color Constants

```cpp
enum Color {
    OFF = 0,
    RED,
    GREEN,
    BLUE,
    YELLOW,
    CYAN,
    MAGENTA,
    WHITE
};
```

## Display Layout

The AZ3166 OLED display has 4 lines (0-3):

```
Line 0: Device name / Title
Line 1: Primary data
Line 2: Secondary data
Line 3: Status messages
```

Recommended usage:
- **Line 0**: Device identifier or application name
- **Line 1**: Primary sensor data
- **Line 2**: Secondary sensor data or metrics
- **Line 3**: Connection status or error messages

## Examples

### Complete Status Display

```cpp
void setup() {
    DisplayHelper::begin();
    DisplayHelper::print(0, "Sensor Station");
    
    // WiFi connection
    DisplayHelper::showWiFiConnecting();
    // ... connect to WiFi ...
    DisplayHelper::showWiFiConnected("192.168.1.50");
    
    // MQTT connection
    DisplayHelper::showMQTTConnecting();
    // ... connect to MQTT ...
    DisplayHelper::showMQTTConnected();
}

void loop() {
    // Read sensors
    float temp = readTemperature();
    float humidity = readHumidity();
    float pressure = readPressure();
    
    // Display sensor data
    DisplayHelper::showSensorData(temp, humidity, pressure);
    
    // Heartbeat LED
    DisplayHelper::setLED(DisplayHelper::BLUE);
    delay(100);
    DisplayHelper::ledOff();
    
    delay(5000);
}
```

### Error Handling

```cpp
if (!connectWiFi()) {
    DisplayHelper::showError("WiFi failed!");
    while(1) {
        DisplayHelper::blinkLED(DisplayHelper::RED, 1);
        delay(1000);
    }
}

if (!connectMQTT()) {
    DisplayHelper::showError("MQTT failed!");
    delay(5000);
}
```

### Custom Progress Indicator

```cpp
for (int i = 0; i < 3; i++) {
    DisplayHelper::printf(2, "Loading... %d/3", i+1);
    DisplayHelper::setLED(DisplayHelper::YELLOW);
    delay(500);
    DisplayHelper::ledOff();
    delay(500);
}
DisplayHelper::print(2, "Ready!");
DisplayHelper::blinkLED(DisplayHelper::GREEN, 2);
```

## Hardware

### OLED Display
- 128x64 pixel monochrome display
- I2C interface
- 4 text lines maximum

### RGB LED
- Common anode RGB LED
- PWM-controlled brightness
- Full color range (0-255 per channel)

## License

MIT License
