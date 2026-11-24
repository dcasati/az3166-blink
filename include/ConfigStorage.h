#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

struct DeviceConfig {
  char magic[4];
  char deviceId[32];
  char model[16];
  char location[32];
  char ssid[32];
  char password[64];
  char mqttServer[64];
  int32_t mqttPort;
  char mqttTopic[64];
  uint8_t checksum;
  uint8_t padding[3];
} __attribute__((packed));

struct OperatorAuth {
  uint8_t salt[16];
  uint8_t verifier[32];
  uint32_t iterations;
};

enum class ConfigLoad { Loaded, Migrated, LegacyOnly, Empty, Failed };

inline bool validDeviceConfig(const DeviceConfig& c) {
  return memcmp(c.magic, "AZ31", 4) == 0 &&
      memchr(c.deviceId, 0, sizeof(c.deviceId)) && c.deviceId[0] &&
      memchr(c.model, 0, sizeof(c.model)) &&
      memchr(c.location, 0, sizeof(c.location)) &&
      memchr(c.ssid, 0, sizeof(c.ssid)) &&
      memchr(c.password, 0, sizeof(c.password)) &&
      memchr(c.mqttServer, 0, sizeof(c.mqttServer)) && c.mqttServer[0] &&
      memchr(c.mqttTopic, 0, sizeof(c.mqttTopic)) && c.mqttTopic[0] &&
      c.mqttPort > 0 && c.mqttPort <= 65535;
}

inline bool validLegacyConfig(const DeviceConfig& config) {
  uint8_t checksum = 0;
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&config);
  for (size_t i = 0; i < offsetof(DeviceConfig, checksum); ++i) checksum ^= bytes[i];
  return validDeviceConfig(config) && config.checksum == checksum;
}
ConfigLoad loadStoredConfig(DeviceConfig& config);
bool saveStoredConfig(const DeviceConfig& config);
bool storedConfigDurable();
bool operatorConfigured();
bool setOperatorPassword(const char* password);
bool checkOperatorPassword(const char* password);
bool secureRandom(uint8_t* output, unsigned length);
bool otaPartitionsValid();
