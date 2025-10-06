# FlashConfig Library

A persistent configuration storage library for STM32-based Arduino boards using internal Flash memory.

## Features

- Persistent storage using STM32 internal Flash
- Automatic checksum validation
- Simple load/save API
- Configurable Flash sector and address
- Magic byte validation
- Support for custom configuration structures

## Usage

### Basic Example

```cpp
#include "FlashConfig.h"

// Create FlashConfig instance
FlashConfig config;

void setup() {
    Serial.begin(115200);
    
    // Try to load configuration from Flash
    if (!config.load()) {
        // No valid config found, set defaults
        FlashConfig::DeviceConfig defaultConfig = {
            {'A','Z','3','1'},           // magic bytes
            "MyDevice",                  // deviceId
            "az3166",                    // model
            "Office",                    // location
            "MyWiFi",                    // ssid
            "MyPassword",                // password
            "mqtt.example.com",          // mqttServer
            1883,                        // mqttPort
            "sensors/data",              // mqttTopic
            0,                           // checksum (calculated automatically)
            {0, 0, 0}                    // padding
        };
        config.setDefaults(defaultConfig);
        config.save();
    }
    
    // Access configuration
    FlashConfig::DeviceConfig& cfg = config.getConfig();
    Serial.print("Device ID: ");
    Serial.println(cfg.deviceId);
}
```

### Modifying Configuration

```cpp
// Get configuration reference
FlashConfig::DeviceConfig& cfg = config.getConfig();

// Modify values
strcpy(cfg.deviceId, "NewDeviceID");
strcpy(cfg.ssid, "NewWiFiNetwork");
cfg.mqttPort = 8883;

// Save to Flash
if (config.save()) {
    Serial.println("Configuration saved!");
} else {
    Serial.println("Failed to save configuration");
}
```

### Custom Flash Location

```cpp
// Use custom Flash sector and address
FlashConfig config(FLASH_SECTOR_11, 0x080E0000);

// Use custom magic bytes
FlashConfig config(FLASH_SECTOR_10, 0x080C0000, "MYID");
```

## API Reference

### Constructor

```cpp
FlashConfig(uint32_t flashSector = FLASH_SECTOR_10, 
            uint32_t flashAddress = 0x080C0000,
            const char* magicBytes = "AZ31");
```

### Methods

- `void setDefaults(const DeviceConfig& config)` - Initialize with default configuration
- `bool load()` - Load configuration from Flash (returns true if successful)
- `bool save()` - Save configuration to Flash (returns true if successful)
- `DeviceConfig& getConfig()` - Get reference to current configuration
- `uint8_t calculateChecksum()` - Calculate checksum for current config
- `bool isValid()` - Check if magic bytes match
- `void resetToDefaults()` - Reset to empty configuration

### Configuration Structure

```cpp
struct DeviceConfig {
    char magic[4];           // Magic bytes for validation
    char deviceId[32];       // Device identifier
    char model[16];          // Device model
    char location[32];       // Device location
    char ssid[32];           // WiFi SSID
    char password[64];       // WiFi password
    char mqttServer[64];     // MQTT server hostname
    int mqttPort;            // MQTT port
    char mqttTopic[64];      // MQTT topic
    uint8_t checksum;        // Checksum (automatically calculated)
    uint8_t padding[3];      // Padding for alignment
} __attribute__((packed));
```

## Important Notes

- The library uses STM32 HAL Flash functions
- Flash sector must be erased before writing
- Checksum is automatically calculated during save
- Magic bytes are used to verify valid configuration
- All strings are null-terminated and size-limited

## License

MIT License
