# AZ3166 Library Examples

This folder contains example sketches demonstrating how to use the AZ3166 Arduino libraries.

## Examples

### 1. Basic Sensor Reading
Simple example reading all sensors and printing to Serial.

```cpp
#include "SensorHub.h"

SensorHub sensors;

void setup() {
    Serial.begin(115200);
    sensors.begin();
}

void loop() {
    sensors.printAll();
    delay(2000);
}
```

### 2. Display and LED Demo
Show sensor data on OLED display with LED indicators.

```cpp
#include "SensorHub.h"
#include "DisplayHelper.h"

SensorHub sensors;

void setup() {
    DisplayHelper::begin();
    DisplayHelper::print(0, "Sensor Demo");
    sensors.begin();
}

void loop() {
    SensorHub::SensorData data;
    if (sensors.readAll(data)) {
        DisplayHelper::showSensorData(data.temperature, data.humidity, data.pressure);
        DisplayHelper::blinkLED(DisplayHelper::GREEN, 1);
    }
    delay(1000);
}
```

### 3. MQTT Publishing
Publish sensor data to MQTT broker.

```cpp
#include "AZ3166WiFi.h"
#include "SensorHub.h"
#include "SimpleMQTT.h"

SensorHub sensors;
SimpleMQTT mqtt;

void setup() {
    Serial.begin(115200);
    
    // Initialize
    sensors.begin();
    
    // Connect WiFi
    WiFi.begin("YourWiFi", "YourPassword");
    while (WiFi.status() != WL_CONNECTED) delay(500);
    
    // Connect MQTT
    mqtt.setServer("mqtt.example.com", 1883);
    mqtt.setClientId("MyDevice");
    mqtt.connect();
}

void loop() {
    char json[512];
    if (sensors.toJSON(json, sizeof(json), "MyDevice", "az3166", "Office")) {
        mqtt.publish("sensors/data", json);
        Serial.println(json);
    }
    delay(5000);
}
```

### 4. Flash Configuration
Save and load device configuration.

```cpp
#include "FlashConfig.h"

FlashConfig config;

void setup() {
    Serial.begin(115200);
    
    // Try to load config
    if (!config.load()) {
        Serial.println("No config found, creating new one");
        
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
    
    // Print config
    FlashConfig::DeviceConfig& cfg = config.getConfig();
    Serial.print("Device: "); Serial.println(cfg.deviceId);
    Serial.print("Location: "); Serial.println(cfg.location);
    Serial.print("WiFi: "); Serial.println(cfg.ssid);
    Serial.print("MQTT: "); Serial.println(cfg.mqttServer);
}

void loop() {
    delay(1000);
}
```

### 5. Complete IoT Station
Full example using all libraries together.

See `src/main_refactored.cpp` for the complete implementation.

## Running Examples

### PlatformIO

1. Copy example code to `src/main.cpp`
2. Build and upload:
   ```bash
   pio run -t upload
   ```

### Serial Monitor

View output:
```bash
pio device monitor
```

## Customization Tips

### Change Sensor Read Interval
```cpp
// Read every 10 seconds instead of 5
if (now - lastSensorRead > 10000) {
    // read sensors
}
```

### Add Custom Topics
```cpp
// Publish to multiple topics
mqtt.publish("sensors/temperature", String(data.temperature).c_str());
mqtt.publish("sensors/humidity", String(data.humidity).c_str());
mqtt.publish("sensors/pressure", String(data.pressure).c_str());
```

### Custom Display Layout
```cpp
DisplayHelper::print(0, "My App v1.0");
DisplayHelper::printf(1, "Temp: %.1f C", temp);
DisplayHelper::printf(2, "Hum: %.0f %%", humidity);
DisplayHelper::showStatus("Running...");
```

### LED Patterns
```cpp
// Success pattern
DisplayHelper::blinkLED(DisplayHelper::GREEN, 3, 100);

// Warning pattern
DisplayHelper::blinkLED(DisplayHelper::YELLOW, 5, 200);

// Error pattern
DisplayHelper::blinkLED(DisplayHelper::RED, 10, 100);
```

## Troubleshooting

### Sensors Not Initializing
- Check I2C connections
- Verify sensor power
- Try resetting the board

### WiFi Connection Fails
- Verify SSID and password
- Check WiFi signal strength
- Ensure 2.4GHz network (not 5GHz)

### MQTT Connection Fails
- Verify broker address and port
- Check network connectivity
- Add hostname mapping for faster connection

### Flash Save Fails
- Ensure Flash sector is not write-protected
- Verify configuration size fits in sector
- Check for Flash wear

## More Resources

- [FlashConfig Documentation](../lib/FlashConfig/README.md)
- [SensorHub Documentation](../lib/SensorHub/README.md)
- [SimpleMQTT Documentation](../lib/SimpleMQTT/README.md)
- [DisplayHelper Documentation](../lib/DisplayHelper/README.md)
