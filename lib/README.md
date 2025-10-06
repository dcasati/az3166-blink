# AZ3166 Arduino Libraries

A collection of modular, reusable libraries for the Azure IoT DevKit (AZ3166), allowing you to use the board as a regular Arduino device without Azure dependencies.

## Libraries Included

### 1. **FlashConfig** - Persistent Configuration Storage
Store and retrieve device configuration in STM32 internal Flash memory.

**Key Features:**
- Persistent storage across power cycles
- Automatic checksum validation
- WiFi and MQTT configuration
- Magic byte verification

[Full Documentation](lib/FlashConfig/README.md)

### 2. **SensorHub** - Unified Sensor Management
Access all AZ3166 onboard sensors through a single, simple interface.

**Supported Sensors:**
- HTS221 - Temperature & Humidity
- LPS22HB - Barometric Pressure
- LSM6DSL - Accelerometer & Gyroscope
- LIS2MDL - Magnetometer

[Full Documentation](lib/SensorHub/README.md)

### 3. **SimpleMQTT** - Lightweight MQTT Client
Pure Arduino MQTT implementation with no external dependencies.

**Key Features:**
- MQTT 3.1 protocol
- QoS 0 publishing
- DNS hostname caching
- Binary & text payloads

[Full Documentation](lib/SimpleMQTT/README.md)

### 4. **DisplayHelper** - Display & LED Control
Simplified control for the OLED display and RGB LED.

**Key Features:**
- 4-line OLED display
- Printf-style formatting
- RGB LED with presets
- Status indicators

[Full Documentation](lib/DisplayHelper/README.md)

## Quick Start

### Installation

1. Copy the `lib/` folder to your PlatformIO project
2. Libraries will be automatically detected

### Basic Example

```cpp
#include <Arduino.h>
#include "FlashConfig.h"
#include "SensorHub.h"
#include "SimpleMQTT.h"
#include "DisplayHelper.h"

FlashConfig config;
SensorHub sensors;
SimpleMQTT mqtt;

void setup() {
    Serial.begin(115200);
    
    // Initialize display
    DisplayHelper::begin();
    DisplayHelper::print(0, "AZ3166 Station");
    
    // Load configuration
    if (!config.load()) {
        // Set defaults if no config found
        FlashConfig::DeviceConfig defaults = {
            {'A','Z','3','1'},
            "MyDevice",
            "az3166",
            "Office",
            "MyWiFi",
            "password",
            "mqtt.example.com",
            1883,
            "sensors/data",
            0, {0,0,0}
        };
        config.setDefaults(defaults);
        config.save();
    }
    
    // Initialize sensors
    if (!sensors.begin()) {
        DisplayHelper::showError("Sensor failed!");
        while(1);
    }
    
    // Connect WiFi
    FlashConfig::DeviceConfig& cfg = config.getConfig();
    WiFi.begin(cfg.ssid, cfg.password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    DisplayHelper::showWiFiConnected(WiFi.localIP().toString().c_str());
    
    // Connect MQTT
    mqtt.setServer(cfg.mqttServer, cfg.mqttPort);
    mqtt.setClientId(cfg.deviceId);
    if (mqtt.connect()) {
        DisplayHelper::showMQTTConnected();
    }
}

void loop() {
    // Read sensors
    SensorHub::SensorData data;
    if (sensors.readAll(data)) {
        // Display on screen
        DisplayHelper::showSensorData(data.temperature, data.humidity, data.pressure);
        
        // Create JSON
        char json[512];
        FlashConfig::DeviceConfig& cfg = config.getConfig();
        sensors.toJSON(json, sizeof(json), cfg.deviceId, cfg.model, cfg.location);
        
        // Publish to MQTT
        if (mqtt.connected()) {
            mqtt.publish(cfg.mqttTopic, json);
            DisplayHelper::setLED(DisplayHelper::GREEN);
            delay(100);
            DisplayHelper::ledOff();
        }
    }
    
    delay(5000);
}
```

## Library Architecture

```
lib/
├── FlashConfig/          # Persistent configuration storage
│   ├── FlashConfig.h
│   ├── FlashConfig.cpp
│   └── README.md
├── SensorHub/            # Unified sensor interface
│   ├── SensorHub.h
│   ├── SensorHub.cpp
│   └── README.md
├── SimpleMQTT/           # Lightweight MQTT client
│   ├── SimpleMQTT.h
│   ├── SimpleMQTT.cpp
│   └── README.md
└── DisplayHelper/        # Display and LED control
    ├── DisplayHelper.h
    ├── DisplayHelper.cpp
    └── README.md
```

## Use Cases

### IoT Sensor Station
Collect environmental data and publish to MQTT broker:
- Temperature, humidity, pressure monitoring
- Motion detection (accelerometer)
- Persistent configuration
- Visual feedback via display

### Edge Device
Use as a standalone edge device:
- No Azure cloud dependency
- Local MQTT broker communication
- Flash-based configuration
- Sensor data aggregation

### Development Platform
Perfect for prototyping:
- Modular architecture
- Easy to extend
- Well-documented APIs
- Arduino-compatible

- Sensor data aggregation

## Platform Configuration

### PlatformIO Configuration

```ini
[env:az3166]
platform = atmelsam
board = mxchip_az3166
framework = arduino

lib_deps =
    # Libraries are auto-detected from lib/ folder
```

### Dependencies

The libraries use AZ3166-specific headers:
- `AZ3166WiFi.h` - WiFi connectivity
- `OledDisplay.h` - OLED display
- `RGB_LED.h` - RGB LED control
- `Sensor.h` - Sensor drivers
- `stm32f4xx_hal.h` - STM32 HAL for Flash

## Memory Usage

Approximate Flash/RAM usage per library:

| Library | Flash | RAM |
|---------|-------|-----|
| FlashConfig | ~2 KB | ~320 bytes |
| SensorHub | ~4 KB | ~200 bytes |
| SimpleMQTT | ~3 KB | ~200 bytes |
| DisplayHelper | ~2 KB | ~150 bytes |
| **Total** | **~11 KB** | **~870 bytes** |

## Migration Guide

### From Original main.cpp

The refactored version (`main_refactored.cpp`) shows how to use these libraries:

**Before (monolithic):**
- All code in main.cpp (~600+ lines)
- Difficult to reuse
- Hard to maintain

**After (modular):**
- Libraries in lib/ folder
- main.cpp reduced to ~200 lines
- Easy to reuse and test
- Clear separation of concerns

## Development

### Adding New Features

Each library is independent and can be extended:

```cpp
// Example: Add WiFi reconnection to SimpleMQTT
class SimpleMQTT {
public:
    bool autoReconnect(uint32_t interval);
    // ... existing methods
};
```

### Testing

Test each library independently:

```cpp
// Test FlashConfig
#include "FlashConfig.h"

void testFlashConfig() {
    FlashConfig cfg;
    // Test save/load
    // Test checksum
    // Test validation
}
```

## Examples

See `src/main_refactored.cpp` for a complete working example using all libraries.

Additional examples:
- Configuration via Serial terminal
- Sensor data logging
- MQTT telemetry
- Status display patterns

## Contributing

These libraries are designed to be:
- **Modular** - Use only what you need
- **Documented** - Every function has clear docs
- **Tested** - Validated on real hardware
- **Extensible** - Easy to add features

## License

MIT License - Feel free to use in your projects!

## Resources

- [AZ3166 DevKit Documentation](https://microsoft.github.io/azure-iot-developer-kit/)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [MQTT Protocol Specification](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)

## Benefits

- **No Azure Dependencies** - Use AZ3166 as a regular Arduino board
- **Reusable Components** - Libraries work across projects  
- **Clean Architecture** - Separation of concerns
- **Well Documented** - Complete API documentation
- **Production Ready** - Tested on real hardware
- **Easy to Extend** - Modular design for customization

---

**Happy coding with your AZ3166!**
