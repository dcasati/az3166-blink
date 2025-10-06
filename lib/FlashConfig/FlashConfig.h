#ifndef FLASH_CONFIG_H
#define FLASH_CONFIG_H

#include <Arduino.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_flash.h"

// Default Flash configuration
#define DEFAULT_FLASH_SECTOR     FLASH_SECTOR_10
#define DEFAULT_FLASH_ADDRESS    0x080C0000

/**
 * FlashConfig - Persistent configuration storage using STM32 internal Flash
 * 
 * This library provides easy-to-use methods for storing and retrieving
 * configuration data in the STM32's internal Flash memory.
 */
class FlashConfig {
public:
    // Configuration structure that can be customized
    struct DeviceConfig {
        char magic[4];           // Magic bytes to verify valid config
        char deviceId[32];       // Device name/ID
        char model[16];          // Device model
        char location[32];       // Device location
        char ssid[32];           // WiFi SSID
        char password[64];       // WiFi password
        char mqttServer[64];     // MQTT server hostname
        int mqttPort;            // MQTT port
        char mqttTopic[64];      // MQTT topic
        uint8_t checksum;        // Simple checksum
        uint8_t padding[3];      // Padding for word alignment
    } __attribute__((packed));

    /**
     * Constructor
     * @param flashSector - STM32 flash sector to use (default: FLASH_SECTOR_10)
     * @param flashAddress - Flash memory address (default: 0x080C0000)
     * @param magicBytes - 4-character magic identifier (default: "AZ31")
     */
    FlashConfig(uint32_t flashSector = DEFAULT_FLASH_SECTOR, 
                uint32_t flashAddress = DEFAULT_FLASH_ADDRESS,
                const char* magicBytes = "AZ31");

    /**
     * Initialize with default configuration
     * @param config - Default configuration to use
     */
    void setDefaults(const DeviceConfig& config);

    /**
     * Load configuration from Flash memory
     * @return true if valid configuration loaded, false otherwise
     */
    bool load();

    /**
     * Save configuration to Flash memory
     * @return true if saved successfully, false otherwise
     */
    bool save();

    /**
     * Get current configuration
     * @return Reference to current configuration
     */
    DeviceConfig& getConfig();

    /**
     * Calculate checksum for configuration
     * @return Calculated checksum value
     */
    uint8_t calculateChecksum();

    /**
     * Check if configuration is valid (magic bytes match)
     * @return true if valid, false otherwise
     */
    bool isValid();

    /**
     * Reset to default configuration
     */
    void resetToDefaults();

private:
    DeviceConfig config;
    uint32_t flashSector;
    uint32_t flashAddress;
    char magic[4];

    uint8_t calculateChecksumInternal(const DeviceConfig* cfg);
};

#endif // FLASH_CONFIG_H
