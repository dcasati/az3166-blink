#include "FlashConfig.h"

FlashConfig::FlashConfig(uint32_t sector, uint32_t address, const char* magicBytes)
    : flashSector(sector), flashAddress(address) {
    memcpy(magic, magicBytes, 4);
    memset(&config, 0, sizeof(DeviceConfig));
}

void FlashConfig::setDefaults(const DeviceConfig& defaultConfig) {
    memcpy(&config, &defaultConfig, sizeof(DeviceConfig));
    memcpy(config.magic, magic, 4);
    config.checksum = calculateChecksum();
}

bool FlashConfig::load() {
    Serial.println("[FlashConfig] Loading configuration from Flash...");
    
    // Read configuration from Flash
    DeviceConfig* flashConfig = (DeviceConfig*)flashAddress;
    
    // Check magic bytes
    if (memcmp(flashConfig->magic, magic, 4) != 0) {
        Serial.println("[FlashConfig] No valid magic bytes found");
        return false;
    }
    
    // Calculate and verify checksum
    uint8_t calculatedChecksum = calculateChecksumInternal(flashConfig);
    
    if (calculatedChecksum != flashConfig->checksum) {
        Serial.println("[FlashConfig] Checksum mismatch");
        return false;
    }
    
    // Copy valid configuration from Flash to RAM
    memcpy(&config, flashConfig, sizeof(DeviceConfig));
    Serial.println("[FlashConfig] Configuration loaded successfully");
    return true;
}

bool FlashConfig::save() {
    // Update checksum before saving
    config.checksum = calculateChecksum();
    memcpy(config.magic, magic, 4);
    memset(config.padding, 0, sizeof(config.padding));
    
    Serial.println("[FlashConfig] Saving configuration to Flash...");
    
    // Unlock Flash memory
    HAL_FLASH_Unlock();
    
    // Erase the sector
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;
    
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector = flashSector;
    EraseInitStruct.NbSectors = 1;
    
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK) {
        Serial.println("[FlashConfig] Flash erase failed!");
        HAL_FLASH_Lock();
        return false;
    }
    
    // Write configuration data word by word
    uint32_t* configData = (uint32_t*)&config;
    uint32_t address = flashAddress;
    int numWords = (sizeof(DeviceConfig) + 3) / 4;
    
    for (int i = 0; i < numWords; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, configData[i]) != HAL_OK) {
            Serial.print("[FlashConfig] Flash write failed at word ");
            Serial.println(i);
            HAL_FLASH_Lock();
            return false;
        }
        address += 4;
    }
    
    HAL_FLASH_Lock();
    Serial.println("[FlashConfig] Configuration saved successfully");
    return true;
}

FlashConfig::DeviceConfig& FlashConfig::getConfig() {
    return config;
}

uint8_t FlashConfig::calculateChecksum() {
    return calculateChecksumInternal(&config);
}

uint8_t FlashConfig::calculateChecksumInternal(const DeviceConfig* cfg) {
    uint8_t checksum = 0;
    
    // Hash each field manually to avoid struct padding issues
    for (int i = 0; i < 4; i++) checksum ^= cfg->magic[i];
    for (int i = 0; i < 32; i++) checksum ^= cfg->deviceId[i];
    for (int i = 0; i < 16; i++) checksum ^= cfg->model[i];
    for (int i = 0; i < 32; i++) checksum ^= cfg->location[i];
    for (int i = 0; i < 32; i++) checksum ^= cfg->ssid[i];
    for (int i = 0; i < 64; i++) checksum ^= cfg->password[i];
    for (int i = 0; i < 64; i++) checksum ^= cfg->mqttServer[i];
    
    uint8_t* portBytes = (uint8_t*)&cfg->mqttPort;
    for (int i = 0; i < 4; i++) checksum ^= portBytes[i];
    
    for (int i = 0; i < 64; i++) checksum ^= cfg->mqttTopic[i];
    
    return checksum;
}

bool FlashConfig::isValid() {
    return memcmp(config.magic, magic, 4) == 0;
}

void FlashConfig::resetToDefaults() {
    memset(&config, 0, sizeof(DeviceConfig));
    memcpy(config.magic, magic, 4);
}
