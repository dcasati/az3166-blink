// Completely bypass Azure framework - pure STM32 + WiFi + MQTT + Sensors
#include <Arduino.h>
#include "AZ3166WiFi.h"
#include "AZ3166WiFiUdp.h"
#include "OledDisplay.h"
#include "RGB_LED.h"
#include "Sensor.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_flash.h"
#include "rtos.h"
#include "Thread.h"

// Firmware version
#define FIRMWARE_VERSION "1.0.0"

// Web server
WiFiServer webServer(80);

// Thread for web server
rtos::Thread *webServerThread_ptr = NULL;

// Flash storage for configuration (using STM32 internal flash)
#define CONFIG_FLASH_SECTOR     FLASH_SECTOR_10    // Use sector 10 for config (128KB sector)
#define CONFIG_FLASH_ADDRESS    0x080C0000         // Start of sector 10

// Configuration structure (stored in Flash memory)
struct DeviceConfig {
  char magic[4];           // "AZ31" - magic bytes to verify valid config
  char deviceId[32];       // Device name/ID
  char model[16];          // Device model (e.g., "az3166")
  char location[32];       // Device location (e.g., "Garage")
  char ssid[32];          // WiFi SSID
  char password[64];      // WiFi password
  char mqttServer[64];    // MQTT server hostname
  int mqttPort;           // MQTT port
  char mqttTopic[64];     // MQTT topic
  uint8_t checksum;       // Simple checksum
  uint8_t padding[3];     // Explicit padding to ensure word alignment
} __attribute__((packed));

// Default configuration
DeviceConfig config = {
  {'A','Z','3','1'},      // magic
  "SensorStation_01",     // deviceId
  "az3166",               // model
  "Garage",               // location
  "YourWiFiNetwork",      // ssid
  "YourWiFiPassword",     // password
  "mqtt.example.com",     // mqttServer
  1883,                   // mqttPort
  "sensors/az3166",       // mqttTopic
  0,                      // checksum (calculated later)
  {0, 0, 0}              // padding
};

// Flash storage functions for persistent configuration
// Note: This implementation uses STM32 HAL Flash functions for real persistence
bool saveConfigToFlash() {
  // Clear checksum and padding before calculating
  config.checksum = 0;
  memset(config.padding, 0, sizeof(config.padding));
  
  // Calculate checksum over specific fields manually to avoid struct padding issues
  uint8_t calculatedChecksum = 0;
  
  // Hash magic bytes (4 bytes)
  for (int i = 0; i < 4; i++) {
    calculatedChecksum ^= config.magic[i];
  }
  
  // Hash device ID (32 bytes)
  for (int i = 0; i < 32; i++) {
    calculatedChecksum ^= config.deviceId[i];
  }
  
  // Hash model (16 bytes)
  for (int i = 0; i < 16; i++) {
    calculatedChecksum ^= config.model[i];
  }
  
  // Hash location (32 bytes)
  for (int i = 0; i < 32; i++) {
    calculatedChecksum ^= config.location[i];
  }
  
  // Hash SSID (32 bytes)
  for (int i = 0; i < 32; i++) {
    calculatedChecksum ^= config.ssid[i];
  }
  
  // Hash password (64 bytes)
  for (int i = 0; i < 64; i++) {
    calculatedChecksum ^= config.password[i];
  }
  
  // Hash MQTT server (64 bytes)
  for (int i = 0; i < 64; i++) {
    calculatedChecksum ^= config.mqttServer[i];
  }
  
  // Hash MQTT port (4 bytes)
  uint8_t* portBytes = (uint8_t*)&config.mqttPort;
  for (int i = 0; i < 4; i++) {
    calculatedChecksum ^= portBytes[i];
  }
  
  // Hash MQTT topic (64 bytes)
  for (int i = 0; i < 64; i++) {
    calculatedChecksum ^= config.mqttTopic[i];
  }
  
  config.checksum = calculatedChecksum;
  
  Serial.println("Saving configuration to Flash...");
  Serial.print("Calculated checksum: 0x");
  Serial.println(config.checksum, 16);
  Serial.print("Config size: ");
  Serial.println(sizeof(DeviceConfig));
  
  // Unlock the Flash memory
  HAL_FLASH_Unlock();
  
  // Erase the sector
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t SectorError = 0;
  
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Sector = CONFIG_FLASH_SECTOR;
  EraseInitStruct.NbSectors = 1;
  
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK) {
    Serial.println("Flash erase failed!");
    HAL_FLASH_Lock();
    return false;
  }
  
  // Write the configuration data word by word
  uint32_t* configData = (uint32_t*)&config;
  uint32_t address = CONFIG_FLASH_ADDRESS;
  int numWords = (sizeof(DeviceConfig) + 3) / 4;  // Round up to nearest word
  
  Serial.print("Writing ");
  Serial.print(numWords);
  Serial.println(" words to Flash...");
  
  for (int i = 0; i < numWords; i++) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, configData[i]) != HAL_OK) {
      Serial.print("Flash write failed at word ");
      Serial.println(i);
      HAL_FLASH_Lock();
      return false;
    }
    address += 4;
  }
  
  // Lock the Flash memory
  HAL_FLASH_Lock();
  
  Serial.println("Configuration saved successfully to Flash!");
  return true;
}

bool loadConfigFromFlash() {
  Serial.println("Loading configuration from Flash...");
  
  // Read configuration from Flash
  DeviceConfig* flashConfig = (DeviceConfig*)CONFIG_FLASH_ADDRESS;
  
  // Check magic bytes
  Serial.print("Magic bytes from Flash: ");
  for (int i = 0; i < 4; i++) {
    Serial.print("0x");
    Serial.print(flashConfig->magic[i], 16);
    Serial.print(" ");
  }
  Serial.println();
  
  if (flashConfig->magic[0] != 'A' || flashConfig->magic[1] != 'Z' || 
      flashConfig->magic[2] != '3' || flashConfig->magic[3] != '1') {
    Serial.println("No valid magic bytes found in Flash");
    return false;
  }
  
  // Calculate checksum using the same field-by-field method as save
  uint8_t calculatedChecksum = 0;
  
  // Hash magic bytes (4 bytes)
  for (int i = 0; i < 4; i++) {
    calculatedChecksum ^= flashConfig->magic[i];
  }
  
  // Hash device ID (32 bytes)
  for (int i = 0; i < 32; i++) {
    calculatedChecksum ^= flashConfig->deviceId[i];
  }
  
  // Hash model (16 bytes)
  for (int i = 0; i < 16; i++) {
    calculatedChecksum ^= flashConfig->model[i];
  }
  
  // Hash location (32 bytes)
  for (int i = 0; i < 32; i++) {
    calculatedChecksum ^= flashConfig->location[i];
  }
  
  // Hash SSID (32 bytes)
  for (int i = 0; i < 32; i++) {
    calculatedChecksum ^= flashConfig->ssid[i];
  }
  
  // Hash password (64 bytes)
  for (int i = 0; i < 64; i++) {
    calculatedChecksum ^= flashConfig->password[i];
  }
  
  // Hash MQTT server (64 bytes)
  for (int i = 0; i < 64; i++) {
    calculatedChecksum ^= flashConfig->mqttServer[i];
  }
  
  // Hash MQTT port (4 bytes)
  uint8_t* portBytes = (uint8_t*)&flashConfig->mqttPort;
  for (int i = 0; i < 4; i++) {
    calculatedChecksum ^= portBytes[i];
  }
  
  // Hash MQTT topic (64 bytes)
  for (int i = 0; i < 64; i++) {
    calculatedChecksum ^= flashConfig->mqttTopic[i];
  }
  
  Serial.print("Stored checksum: 0x");
  Serial.println(flashConfig->checksum, 16);
  Serial.print("Calculated checksum: 0x");
  Serial.println(calculatedChecksum, 16);
  
  if (calculatedChecksum != flashConfig->checksum) {
    Serial.println("Flash configuration checksum mismatch");
    return false;
  }
  
  // Copy valid configuration from Flash to RAM
  memcpy(&config, flashConfig, sizeof(DeviceConfig));
  Serial.println("Valid configuration loaded from Flash!");
  return true;
}

// Legacy variables for compatibility
// DNS and MQTT globals
WiFiClient mqttWifiClient;
IPAddress cachedMqttIP;
bool mqttIPResolved = false;

// Sensor variables  
DevI2C *i2c;
HTS221Sensor *ht_sensor;
LPS22HBSensor *pressure_sensor;
LSM6DSLSensor *acc_gyro;
LIS2MDLSensor *magnetometer;

// RGB LED instance
RGB_LED rgbLED;

// Control states for web interface
bool ledEnabled = true;
bool displayEnabled = true;
bool mqttConnected = false;  // Global MQTT connection status
bool wifiLedEnabled = false;  // WiFi LED control
bool azureLedEnabled = false; // Azure LED control
bool userLedEnabled = false;  // User LED control

// Debounce variables to prevent rapid toggling
unsigned long lastLedChange = 0;
unsigned long lastDisplayChange = 0;
const unsigned long DEBOUNCE_DELAY = 1000; // 1 second minimum between changes

// WiFi retry management
unsigned long lastWifiCheck = 0;
unsigned long lastWifiRetry = 0;
bool webServerStarted = false;
const unsigned long WIFI_CHECK_INTERVAL = 5000;  // Check WiFi every 5 seconds
const unsigned long WIFI_RETRY_INTERVAL = 30000; // Retry WiFi every 30 seconds

// Network watchdog - reboot if no network activity for 15 minutes
unsigned long lastSuccessfulNetworkActivity = 0;
const unsigned long NETWORK_WATCHDOG_TIMEOUT = 15 * 60 * 1000; // 15 minutes in milliseconds
bool watchdogEnabled = true;

// Global sensor values for web display
float lastTemperature = 0.0;
float lastHumidity = 0.0;
float lastPressure = 0.0;
float lastAccelX = 0.0, lastAccelY = 0.0, lastAccelZ = 0.0;
float lastGyroX = 0.0, lastGyroY = 0.0, lastGyroZ = 0.0;
float lastMagX = 0.0, lastMagY = 0.0, lastMagZ = 0.0;

// Sensor calibration offsets
// Adjust these values to match your local conditions
const float PRESSURE_OFFSET = 141.0; // mbar offset to correct sensor reading
const float TEMPERATURE_OFFSET = -2.0; // °C offset to correct temperature reading

// Force front-panel status LEDs (Wi-Fi, Azure, User) off
void disableStatusLedsOnce()
{
  pinMode(LED_WIFI,  OUTPUT);
  pinMode(LED_AZURE, OUTPUT);
  pinMode(LED_USER,  OUTPUT);
  digitalWrite(LED_WIFI,  LOW);
  digitalWrite(LED_AZURE, LOW);
  digitalWrite(LED_USER,  LOW);
}

// System reboot function using STM32 HAL
void systemReboot() {
  Serial.println("\n========================================");
  Serial.println("NETWORK WATCHDOG: Initiating system reboot...");
  Serial.println("========================================\n");
  Serial.flush(); // Ensure message is sent before reboot
  
  // Visual indication before reboot
  rgbLED.setColor(255, 0, 0); // Red
  delay(1000);
  rgbLED.turnOff();
  
  // Perform system reset using STM32 HAL
  NVIC_SystemReset();
}

// Network watchdog check - monitors network activity and reboots if needed
void checkNetworkWatchdog() {
  if (!watchdogEnabled) {
    return;
  }
  
  unsigned long now = millis();
  
  // Handle millis() rollover (occurs every ~49 days)
  if (now < lastSuccessfulNetworkActivity) {
    lastSuccessfulNetworkActivity = now;
    return;
  }
  
  // Check if we have WiFi connection
  bool hasNetworkActivity = false;
  
  if (WiFi.status() == WL_CONNECTED) {
    // We have WiFi, this counts as network activity
    hasNetworkActivity = true;
  }
  
  // Update last successful activity timestamp if we have connectivity
  if (hasNetworkActivity) {
    lastSuccessfulNetworkActivity = now;
  } else {
    // No network activity - check if timeout exceeded
    unsigned long timeSinceLastActivity = now - lastSuccessfulNetworkActivity;
    
    if (timeSinceLastActivity > NETWORK_WATCHDOG_TIMEOUT) {
      // Timeout exceeded - log and reboot
      Serial.println("\n!!! NETWORK WATCHDOG TIMEOUT !!!");
      Serial.print("No network activity for ");
      Serial.print(timeSinceLastActivity / 1000);
      Serial.println(" seconds");
      Serial.print("Threshold: ");
      Serial.print(NETWORK_WATCHDOG_TIMEOUT / 1000);
      Serial.println(" seconds");
      
      if (displayEnabled) {
        Screen.clean();
        Screen.print(0, "WATCHDOG TIMEOUT");
        Screen.print(1, "Rebooting...");
      }
      
      delay(2000); // Give time to display message
      systemReboot();
    } else {
      // Log warning every minute when disconnected
      static unsigned long lastWarning = 0;
      if (now - lastWarning > 60000) {
        lastWarning = now;
        Serial.print("WARNING: No network activity for ");
        Serial.print(timeSinceLastActivity / 1000);
        Serial.print(" seconds (timeout in ");
        Serial.print((NETWORK_WATCHDOG_TIMEOUT - timeSinceLastActivity) / 1000);
        Serial.println(" seconds)");
      }
    }
  }
}

// Configuration management functions
uint8_t calculateChecksum(const DeviceConfig* cfg) {
  uint8_t checksum = 0;
  const uint8_t* data = (const uint8_t*)cfg;
  for (int i = 0; i < sizeof(DeviceConfig) - 1; i++) {  // -1 to exclude checksum field
    checksum ^= data[i];
  }
  return checksum;
}

void loadDefaultConfig() {
  Serial.println("Loading default configuration...");
  config.magic[0] = 'A'; config.magic[1] = 'Z'; config.magic[2] = '3'; config.magic[3] = '1';
  strcpy(config.deviceId, "SensorStation_01");
  strcpy(config.model, "az3166");
  strcpy(config.location, "Garage");
  strcpy(config.ssid, "YourWiFiNetwork");
  strcpy(config.password, "YourWiFiPassword");
  strcpy(config.mqttServer, "mqtt.example.com");
  config.mqttPort = 1883;
  strcpy(config.mqttTopic, "sensors/az3166");
  config.checksum = calculateChecksum(&config);
}

String readSerialString(const char* prompt, const char* defaultValue, int maxLength) {
  Serial.print(prompt);
  Serial.print(" [");
  Serial.print(defaultValue);
  Serial.print("]: ");
  
  String input = "";
  unsigned long startTime = millis();
  const unsigned long timeout = 30000; // 30 second timeout
  
  while (millis() - startTime < timeout) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        break;
      } else if (c == '\b' || c == 127) { // Backspace
        if (input.length() > 0) {
          input.remove(input.length() - 1);
          Serial.print("\b \b");
        }
      } else if (input.length() < maxLength - 1 && c >= 32 && c <= 126) {
        input += c;
        Serial.print(c);
      }
    }
    delay(10);
  }
  
  Serial.println();
  return input.length() > 0 ? input : String(defaultValue);
}

void configureDevice() {
  Serial.println("\n=== DEVICE CONFIGURATION ===");
  Serial.println("Configure your device settings via serial interface.");
  Serial.println("Press ENTER to keep default values, or type new values.");
  Serial.println("Configuration will timeout in 30 seconds per field.\n");
  
  // Device ID
  String newDeviceId = readSerialString("Device ID", config.deviceId, sizeof(config.deviceId));
  strcpy(config.deviceId, newDeviceId.c_str());
  
  // Device Model
  String newModel = readSerialString("Device Model", config.model, sizeof(config.model));
  strcpy(config.model, newModel.c_str());
  
  // Device Location
  String newLocation = readSerialString("Device Location", config.location, sizeof(config.location));
  strcpy(config.location, newLocation.c_str());
  
  // WiFi SSID
  String newSSID = readSerialString("WiFi SSID", config.ssid, sizeof(config.ssid));
  strcpy(config.ssid, newSSID.c_str());
  
  // WiFi Password
  String newPassword = readSerialString("WiFi Password", config.password, sizeof(config.password));
  strcpy(config.password, newPassword.c_str());
  
  // MQTT Server
  String newMqttServer = readSerialString("MQTT Server", config.mqttServer, sizeof(config.mqttServer));
  strcpy(config.mqttServer, newMqttServer.c_str());
  
  // MQTT Port
  String newMqttPort = readSerialString("MQTT Port", String(config.mqttPort).c_str(), 8);
  config.mqttPort = newMqttPort.toInt();
  if (config.mqttPort <= 0 || config.mqttPort > 65535) {
    config.mqttPort = 1883;
  }
  
  // MQTT Topic
  String newMqttTopic = readSerialString("MQTT Topic", config.mqttTopic, sizeof(config.mqttTopic));
  strcpy(config.mqttTopic, newMqttTopic.c_str());
  
  // Update checksum
  config.checksum = calculateChecksum(&config);
  
  Serial.println("\n=== CONFIGURATION SUMMARY ===");
  Serial.print("Device ID: "); Serial.println(config.deviceId);
  Serial.print("Device Model: "); Serial.println(config.model);
  Serial.print("Device Location: "); Serial.println(config.location);
  Serial.print("WiFi SSID: "); Serial.println(config.ssid);
  Serial.print("WiFi Password: "); Serial.println("***hidden***");
  Serial.print("MQTT Server: "); Serial.println(config.mqttServer);
  Serial.print("MQTT Port: "); Serial.println(config.mqttPort);
  Serial.print("MQTT Topic: "); Serial.println(config.mqttTopic);
  
  // Save configuration to Flash memory
  if (saveConfigToFlash()) {
    Serial.println("Configuration saved to Flash memory!");
  } else {
    Serial.println("Warning: Failed to save configuration to Flash!");
  }
}

bool checkForConfigurationMode() {
  Serial.println("\n=== AZ3166 Sensor Station ===");
  Serial.println("Press 'C' within 5 seconds to enter configuration mode...");
  
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'C' || c == 'c') {
        Serial.println("Entering configuration mode...");
        return true;
      }
    }
    delay(100);
  }
  
  Serial.println("Starting with current configuration...");
  return false;
}

// Simplified DNS Resolution - use direct IP for now, add full DNS later
bool resolveHostname(const char* hostname, IPAddress& ip) {
  Serial.print("Resolving hostname: ");
  Serial.println(hostname);
  
  // For now, use the direct IP we know works
  // TODO: Implement full UDP DNS when we have time
  if (strcmp(hostname, "mqtt.dcasati.net") == 0) {
    ip = IPAddress(172, 16, 5, 241);
    Serial.print("Using cached IP: ");
    Serial.println(ip);
    return true;
  }
  
  Serial.println("Hostname not in cache");
  return false;
}

// Simple MQTT Connect
bool connectMQTT() {
  // Use cached IP if available
  IPAddress targetIP;
  if (mqttIPResolved) {
    targetIP = cachedMqttIP;
    Serial.print("Using cached MQTT IP: ");
    Serial.println(targetIP);
  } else {
    Serial.println("Resolving MQTT server IP...");
    if (resolveHostname(config.mqttServer, targetIP)) {
      cachedMqttIP = targetIP;
      mqttIPResolved = true;
    } else {
      Serial.println("Failed to resolve MQTT server");
      return false;
    }
  }
  
  // Test basic connectivity first
  Serial.println("Testing basic network connectivity...");
  WiFiClient testClient;
  if (testClient.connect(targetIP, config.mqttPort)) {
    Serial.println("Basic connectivity test passed (port 1883)");
    testClient.stop();
    delay(100); // Small delay before main connection
  } else {
    Serial.println("Basic connectivity test failed (port 1883)");
  }
  
  Serial.print("Connecting to MQTT broker at ");
  Serial.print(targetIP);
  Serial.print(":");
  Serial.println(config.mqttPort);
  
  Serial.println("Attempting TCP connection...");
  Serial.print("WiFi status before connect: ");
  Serial.println(WiFi.status());
  
  if (!mqttWifiClient.connect(targetIP, config.mqttPort)) {
    Serial.println("TCP connection failed");
    Serial.print("WiFi status after failed connect: ");
    Serial.println(WiFi.status());
    Serial.print("Client connected status: ");
    Serial.println(mqttWifiClient.connected());
    return false;
  }
  
  Serial.println("TCP connected, sending MQTT CONNECT...");
  
  // Build MQTT CONNECT packet
  uint8_t packet[128];
  int pos = 0;
  
  // Fixed header
  packet[pos++] = 0x10;  // CONNECT packet type
  int lenPos = pos++;    // Length placeholder
  
  // Variable header
  packet[pos++] = 0x00; packet[pos++] = 0x06; // Protocol name length
  packet[pos++] = 'M'; packet[pos++] = 'Q'; packet[pos++] = 'I'; packet[pos++] = 's'; packet[pos++] = 'd'; packet[pos++] = 'p';
  packet[pos++] = 0x03;  // Protocol version (MQTT 3.1)
  packet[pos++] = 0x02;  // Connect flags (clean session)
  packet[pos++] = 0x00; packet[pos++] = 0x3C;  // Keep alive (60 seconds)
  
  // Payload - Client ID
  int idLen = strlen(config.deviceId);
  packet[pos++] = 0x00; packet[pos++] = idLen;
  memcpy(&packet[pos], config.deviceId, idLen);
  pos += idLen;
  
  // Set remaining length
  packet[lenPos] = pos - 2;
  
  // Send packet
  Serial.print("Sending MQTT CONNECT packet (");
  Serial.print(pos);
  Serial.print(" bytes): ");
  for (int i = 0; i < pos; i++) {
    Serial.print("0x");
    if (packet[i] < 16) Serial.print("0");
    Serial.print(packet[i], 16);  // 16 for hex format
    Serial.print(" ");
  }
  Serial.println();
  
  mqttWifiClient.write(packet, pos);
  Serial.println("Packet sent, waiting for CONNACK...");
  
  // Wait for CONNACK
  unsigned long timeout = millis() + 5000;
  Serial.println("Waiting for CONNACK response...");
  
  // Wait for at least some data
  while (millis() < timeout && mqttWifiClient.available() == 0) {
    delay(10);
  }
  
  if (mqttWifiClient.available() > 0) {
    Serial.print("Received ");
    Serial.print(mqttWifiClient.available());
    Serial.println(" bytes from broker");
    
    // Give it a moment for the complete packet to arrive
    delay(50);
    
    Serial.print("Total bytes available: ");
    Serial.println(mqttWifiClient.available());
  }
  
  if (mqttWifiClient.available() >= 4) {
    uint8_t response[4];
    mqttWifiClient.read(response, 4);
    
    Serial.print("CONNACK response: ");
    for (int i = 0; i < 4; i++) {
      Serial.print("0x");
      if (response[i] < 16) Serial.print("0");
      Serial.print(response[i], 16);
      Serial.print(" ");
    }
    Serial.println();
    
    if (response[0] == 0x20 && response[3] == 0x00) {
      Serial.println("MQTT connected successfully!");
      return true;
    } else {
      Serial.print("MQTT CONNACK failed, return code: ");
      Serial.println(response[3]);
    }
  } else if (mqttWifiClient.available() >= 1) {
    // Try to read whatever we got
    uint8_t partialResponse[4] = {0};
    int bytesRead = mqttWifiClient.read(partialResponse, mqttWifiClient.available());
    Serial.print("Received CONNACK (");
    Serial.print(bytesRead);
    Serial.println(" bytes): ");
    for (int i = 0; i < bytesRead; i++) {
      Serial.print("0x");
      if (partialResponse[i] < 16) Serial.print("0");
      Serial.print(partialResponse[i], 16);
      Serial.print(" ");
    }
    Serial.println();
    
    // If we got 0x20, that's the CONNACK message type - treat as success
    if (partialResponse[0] == 0x20) {
      Serial.println("MQTT connection successful (broker sent CONNACK 0x20)!");
      return true;
    }
  } else {
    Serial.println("MQTT CONNACK timeout - insufficient data");
  }
  
  mqttWifiClient.stop();
  return false;
}

// Simple MQTT Publish
bool publishMQTT(const char* topic, const char* payload) {
  if (!mqttWifiClient.connected()) {
    Serial.println("MQTT not connected");
    return false;
  }
  
  int topicLen = strlen(topic);
  int payloadLen = strlen(payload);
  
  // Calculate remaining length (topic length field + topic + payload)
  int remainingLength = 2 + topicLen + payloadLen;
  
  // Build PUBLISH packet
  uint8_t packet[1024];  // Increased buffer size for large JSON payloads
  int pos = 0;
  
  // Fixed header
  packet[pos++] = 0x30;  // PUBLISH packet type (QoS 0, no retain, no dup)
  
  // Variable length encoding for remaining length
  if (remainingLength < 128) {
    packet[pos++] = remainingLength;
  } else if (remainingLength < 16384) {
    packet[pos++] = (remainingLength % 128) | 0x80;
    packet[pos++] = remainingLength / 128;
  } else {
    // For very large packets, we'd need more bytes, but this should be sufficient
    Serial.println("MQTT payload too large");
    return false;
  }
  
  // Topic length (MSB, LSB)
  packet[pos++] = (topicLen >> 8) & 0xFF;
  packet[pos++] = topicLen & 0xFF;
  
  // Topic
  memcpy(&packet[pos], topic, topicLen);
  pos += topicLen;
  
  // Payload (no length field for payload in PUBLISH packets)
  memcpy(&packet[pos], payload, payloadLen);
  pos += payloadLen;
  
  // Debug output
  Serial.print("MQTT PUBLISH packet (");
  Serial.print(pos);
  Serial.print(" bytes): Topic=");
  Serial.print(topic);
  Serial.print(", Payload size=");
  Serial.println(payloadLen);
  
  size_t written = mqttWifiClient.write(packet, pos);
  Serial.print("MQTT bytes written: ");
  Serial.print(written);
  Serial.print("/");
  Serial.println(pos);
  
  if (written != pos) {
    Serial.println("MQTT write failed!");
    return false;
  }
  
  mqttWifiClient.flush();
  Serial.println("MQTT message published and flushed");
  return true;
}

// Web server functions - optimized for speed

// Standard HTTP 200 header with no-cache semantics
void sendHttpHeader(WiFiClient &client, int contentLength, const char *contentType = "text/html") {
  char header[256];
  
  int headerLen = snprintf(
    header,
    sizeof(header),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Connection: close\r\n"
    "Cache-Control: no-cache, no-store, must-revalidate\r\n"
    "Pragma: no-cache\r\n"
    "Expires: 0\r\n"
    "Content-Length: %d\r\n"
    "\r\n",
    contentType,
    contentLength
  );
  
  if (headerLen < 0) {
    headerLen = 0;
  } else if (headerLen >= (int)sizeof(header)) {
    headerLen = sizeof(header) - 1;
  }
  
  client.write((const uint8_t *)header, headerLen);
}

void sendMainPage(WiFiClient &client) {
  Serial.println("Sending main page");
  
  char body[1600];
  int bodyLen = snprintf(body, sizeof(body),
    "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>%s</title>"
    "<style>*{box-sizing:border-box}body{font-family:-apple-system,BlinkMacSystemFont,Arial,sans-serif;margin:0;padding:20px;background:#f5f5f7;max-width:500px;margin:0 auto;text-align:center}"
    ".c{background:#fff;border-radius:12px;padding:30px;margin-bottom:12px;box-shadow:0 1px 3px rgba(0,0,0,0.1)}"
    "h1{margin:0 0 8px;font-size:24px;font-weight:600}"
    ".s{font-size:13px;color:#666;margin-bottom:16px;display:flex;align-items:center;gap:8px;justify-content:center;flex-wrap:wrap}"
    ".b{padding:4px 8px;border-radius:4px;font-size:11px;font-weight:600;white-space:nowrap}"
    ".on{background:#34c759;color:#fff}.off{background:#ff3b30;color:#fff}"
    "a{display:block;width:100%%;padding:18px;border:none;border-radius:10px;text-align:center;font-weight:600;font-size:18px;cursor:pointer;margin:12px 0;text-decoration:none;transition:opacity 0.2s}"
    "a:active{opacity:0.7}"
    ".btn-b{background:#007aff;color:#fff}.btn-o{background:#ff9500;color:#fff}.ver{font-size:11px;color:#999;margin-top:16px}"
    "</style></head><body>"
    "<div class='c'><h1>%s</h1>"
    "<div class='s'><span>MQTT:</span><span class='b %s'>%s</span><span style='color:#999'>|</span><span style='color:#999'>%s</span></div>"
    "<a href='/control' class='btn-b'>CONTROL</a>"
    "<a href='/telemetry' style='background:#5856d6;color:#fff'>TELEMETRY</a>"
    "<a href='/setup' class='btn-o'>SETUP</a>"
    "<div class='ver'>v%s</div>"
    "</div></body></html>",
    config.deviceId, config.deviceId,
    mqttConnected ? "on" : "off",
    mqttConnected ? "CONNECTED" : "DISCONNECTED",
    config.mqttServer, FIRMWARE_VERSION);
  
  if (bodyLen < 0) {
    bodyLen = 0;
  } else if (bodyLen >= (int)sizeof(body)) {
    bodyLen = sizeof(body) - 1;
    Serial.print("WARNING: Main page HTML truncated! Buffer size: ");
    Serial.println(sizeof(body));
  }
  
  Serial.print("Main page HTML size: ");
  Serial.println(bodyLen);
  
  sendHttpHeader(client, bodyLen, "text/html");
  client.write((const uint8_t*)body, bodyLen);
  client.flush();
  Serial.println("Main page sent!");
}

void sendControlPage(WiFiClient &client) {
  Serial.println("Sending control page");
  
  char body[3200];
  
  // Calculate time since last network activity for watchdog display
  unsigned long timeSinceActivity = (millis() - lastSuccessfulNetworkActivity) / 1000; // seconds
  unsigned long timeoutRemaining = 0;
  if (watchdogEnabled && timeSinceActivity < (NETWORK_WATCHDOG_TIMEOUT / 1000)) {
    timeoutRemaining = (NETWORK_WATCHDOG_TIMEOUT / 1000) - timeSinceActivity;
  }
  
  int bodyLen = snprintf(body, sizeof(body),
    "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>Control - %s</title>"
    "<style>*{box-sizing:border-box}body{font-family:-apple-system,BlinkMacSystemFont,Arial,sans-serif;margin:0;padding:10px;background:#f5f5f7;max-width:500px;margin:0 auto}"
    ".c{background:#fff;border-radius:12px;padding:16px;margin-bottom:12px;box-shadow:0 1px 3px rgba(0,0,0,0.1)}"
    "h2{margin:0 0 8px;font-size:20px;font-weight:600}"
    ".s{font-size:13px;color:#666;margin-bottom:4px;display:flex;align-items:center;gap:8px;flex-wrap:wrap}"
    ".b{padding:4px 8px;border-radius:4px;font-size:11px;font-weight:600;white-space:nowrap}"
    ".on{background:#34c759;color:#fff}.off{background:#ff3b30;color:#fff}"
    ".row{display:flex;gap:8px;margin:8px 0}"
    "form{flex:1;margin:0}"
    "button,a{display:block;width:100%%;padding:14px;border:none;border-radius:10px;text-align:center;font-weight:600;font-size:16px;cursor:pointer;transition:opacity 0.2s;-webkit-tap-highlight-color:transparent;text-decoration:none;margin:0}"
    "button:active,a:active{opacity:0.7}"
    ".g{background:#007aff;color:#fff}.r{background:#8e8e93;color:#fff}.u{background:#007aff;color:#fff}.gray{background:#8e8e93;color:#fff}"
    "@media(min-width:400px){body{padding:15px}.c{padding:20px}button,a{font-size:15px}}"
    "</style></head><body>"
    "<div class='c'><h2>%s</h2>"
    "<div class='s'><span>MQTT:</span><span class='b %s'>%s</span><span style='color:#999'>|</span><span style='color:#999'>%s</span></div>"
    "<div class='s'><span>Watchdog:</span><span class='b %s'>%s</span>%s</div></div>"
    "<div class='c'>"
    "<div class='row'><form action='/led' method='GET'><input type='hidden' name='state' value='on'><button class='g'>LED ON</button></form>"
    "<form action='/led' method='GET'><input type='hidden' name='state' value='off'><button class='r'>LED OFF</button></form></div>"
    "<div class='row'><form action='/display' method='GET'><input type='hidden' name='state' value='on'><button class='g'>Display ON</button></form>"
    "<form action='/display' method='GET'><input type='hidden' name='state' value='off'><button class='r'>Display OFF</button></form></div>"
    "<div class='row'><form action='/wifiled' method='GET'><input type='hidden' name='state' value='on'><button class='g'>WiFi LED ON</button></form>"
    "<form action='/wifiled' method='GET'><input type='hidden' name='state' value='off'><button class='r'>WiFi LED OFF</button></form></div>"
    "<div class='row'><form action='/azureled' method='GET'><input type='hidden' name='state' value='on'><button class='g'>Azure LED ON</button></form>"
    "<form action='/azureled' method='GET'><input type='hidden' name='state' value='off'><button class='r'>Azure LED OFF</button></form></div>"
    "<div class='row'><form action='/userled' method='GET'><input type='hidden' name='state' value='on'><button class='g'>User LED ON</button></form>"
    "<form action='/userled' method='GET'><input type='hidden' name='state' value='off'><button class='r'>User LED OFF</button></form></div>"
    "<div class='row'><form action='/watchdog' method='GET'><input type='hidden' name='state' value='enable'><button class='g'>Watchdog ON</button></form>"
    "<form action='/watchdog' method='GET'><input type='hidden' name='state' value='disable'><button class='r'>Watchdog OFF</button></form></div>"
    "<form action='/reset' method='GET' style='margin:16px 0 8px'><button class='u'>RESET</button></form>"
    "<a href='/' class='gray'>BACK</a>"
    "</div></body></html>",
    config.deviceId,
    config.deviceId,
    mqttConnected ? "on" : "off",
    mqttConnected ? "CONNECTED" : "DISCONNECTED",
    config.mqttServer,
    watchdogEnabled ? "on" : "off",
    watchdogEnabled ? "ENABLED" : "DISABLED",
    watchdogEnabled ? (timeoutRemaining > 0 ? "<span style='color:#999'> | </span><span style='color:#999'>OK</span>" : "") : "");
  
  if (bodyLen < 0) {
    bodyLen = 0;
  } else if (bodyLen >= (int)sizeof(body)) {
    bodyLen = sizeof(body) - 1;
    Serial.print("WARNING: Control page HTML truncated! Buffer size: ");
    Serial.println(sizeof(body));
  }
  
  Serial.print("Control page HTML size: ");
  Serial.println(bodyLen);
  
  sendHttpHeader(client, bodyLen, "text/html");
  client.write((const uint8_t*)body, bodyLen);
  client.flush();
  Serial.println("Control page sent!");
}

void sendTelemetryPage(WiFiClient &client) {
  Serial.println("Sending telemetry page");
  
  char body[2560];
  int bodyLen = snprintf(body, sizeof(body),
    "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>Telemetry - %s</title>"
    "<style>*{box-sizing:border-box}body{font-family:-apple-system,BlinkMacSystemFont,Arial,sans-serif;margin:0;padding:10px;background:#f5f5f7;max-width:500px;margin:0 auto}"
    ".c{background:#fff;border-radius:12px;padding:16px;margin-bottom:12px;box-shadow:0 1px 3px rgba(0,0,0,0.1)}"
    "h2{margin:0 0 12px;font-size:20px;font-weight:600}"
    "h3{margin:16px 0 8px;font-size:16px;font-weight:600;color:#666}"
    ".row{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #f0f0f0}"
    ".row:last-child{border-bottom:none}"
    ".label{font-weight:600;color:#333}"
    ".value{color:#666}"
    "a{display:block;width:100%%;padding:14px;border:none;border-radius:10px;text-align:center;font-weight:600;font-size:16px;cursor:pointer;transition:opacity 0.2s;text-decoration:none;margin-top:12px}"
    "a:active{opacity:0.7}"
    ".gray{background:#8e8e93;color:#fff}"
    "</style></head><body>"
    "<div class='c'><h2>Telemetry Data</h2>"
    "<h3>Environment</h3>"
    "<div class='row'><span class='label'>Temperature</span><span class='value'>%.2f C</span></div>"
    "<div class='row'><span class='label'>Humidity</span><span class='value'>%.2f %%</span></div>"
    "<div class='row'><span class='label'>Pressure</span><span class='value'>%.2f mbar</span></div>"
    "<h3>Accelerometer</h3>"
    "<div class='row'><span class='label'>X-axis</span><span class='value'>%.3f g</span></div>"
    "<div class='row'><span class='label'>Y-axis</span><span class='value'>%.3f g</span></div>"
    "<div class='row'><span class='label'>Z-axis</span><span class='value'>%.3f g</span></div>"
    "<h3>Gyroscope</h3>"
    "<div class='row'><span class='label'>X-axis</span><span class='value'>%.2f dps</span></div>"
    "<div class='row'><span class='label'>Y-axis</span><span class='value'>%.2f dps</span></div>"
    "<div class='row'><span class='label'>Z-axis</span><span class='value'>%.2f dps</span></div>"
    "<h3>Magnetometer</h3>"
    "<div class='row'><span class='label'>X-axis</span><span class='value'>%.3f G</span></div>"
    "<div class='row'><span class='label'>Y-axis</span><span class='value'>%.3f G</span></div>"
    "<div class='row'><span class='label'>Z-axis</span><span class='value'>%.3f G</span></div>"
    "</div>"
    "</div>"
    "<a href='/' class='gray'>BACK</a>"
    "</body></html>",
    config.deviceId,
    lastTemperature, lastHumidity, lastPressure,
    lastAccelX, lastAccelY, lastAccelZ,
    lastGyroX, lastGyroY, lastGyroZ,
    lastMagX, lastMagY, lastMagZ);
  
  if (bodyLen < 0) {
    bodyLen = 0;
  } else if (bodyLen >= (int)sizeof(body)) {
    bodyLen = sizeof(body) - 1;
    Serial.print("WARNING: Telemetry page HTML truncated! Buffer size: ");
    Serial.println(sizeof(body));
  }
  
  Serial.print("Telemetry page HTML size: ");
  Serial.println(bodyLen);
  
  sendHttpHeader(client, bodyLen, "text/html");
  client.write((const uint8_t*)body, bodyLen);
  client.flush();
  Serial.println("Telemetry page sent!");
}

void sendSetupPage(WiFiClient &client) {
  unsigned long sendStart = millis();
  Serial.print("Sending setup page at ");
  Serial.println(sendStart);
  
  // Larger buffer for setup page with form inputs
  char body[3072];
  int bodyLen = snprintf(body, sizeof(body),
    "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>Setup - %s</title>"
    "<style>*{box-sizing:border-box}body{font-family:-apple-system,BlinkMacSystemFont,Arial,sans-serif;margin:0;padding:10px;background:#f5f5f7;max-width:500px;margin:0 auto}"
    ".c{background:#fff;border-radius:12px;padding:16px;margin-bottom:12px;box-shadow:0 1px 3px rgba(0,0,0,0.1)}"
    "h2{margin:0 0 12px;font-size:20px;font-weight:600}"
    "label{display:block;font-size:13px;font-weight:600;color:#333;margin:12px 0 4px}"
    "input{width:100%%;padding:10px;border:1px solid #ddd;border-radius:8px;font-size:15px}"
    "input:focus{outline:none;border-color:#007aff}"
    "button,a{display:block;width:100%%;padding:14px;border:none;border-radius:10px;text-align:center;font-weight:600;font-size:16px;cursor:pointer;transition:opacity 0.2s;-webkit-tap-highlight-color:transparent;text-decoration:none;margin-top:12px}"
    "button:active,a:active{opacity:0.7}"
    ".g{background:#34c759;color:#fff}.gray{background:#8e8e93;color:#fff}"
    ".note{font-size:12px;color:#999;margin-top:8px}"
    "@media(min-width:400px){body{padding:15px}.c{padding:20px}}"
    "</style></head><body>"
    "<div class='c'><h2>Device Setup</h2>"
    "<form action='/save-config' method='GET'>"
    "<label>Device ID</label><input name='deviceId' value='%s' maxlength='31'>"
    "<label>Model</label><input name='model' value='%s' maxlength='15'>"
    "<label>Location</label><input name='location' value='%s' maxlength='31'>"
    "<label>WiFi SSID</label><input name='ssid' value='%s' maxlength='31'>"
    "<label>WiFi Password</label><input name='password' type='password' value='%s' maxlength='63'>"
    "<label>MQTT Server</label><input name='mqttServer' value='%s' maxlength='63'>"
    "<label>MQTT Port</label><input name='mqttPort' type='number' value='%d' min='1' max='65535'>"
    "<label>MQTT Topic</label><input name='mqttTopic' value='%s' maxlength='63'>"
    "<button class='g'>SAVE & REBOOT</button>"
    "</form>"
    "<p class='note'>Saving will write configuration to Flash memory and reboot the device.</p>"
    "<a href='/' class='gray'>CANCEL</a>"
    "</div></body></html>",
    config.deviceId,
    config.deviceId, config.model, config.location,
    config.ssid, config.password, config.mqttServer,
    config.mqttPort, config.mqttTopic);
  
  if (bodyLen < 0) {
    bodyLen = 0;
  } else if (bodyLen >= (int)sizeof(body)) {
    bodyLen = sizeof(body) - 1;
    Serial.print("WARNING: Setup page HTML truncated! Buffer size: ");
    Serial.println(sizeof(body));
  }
  
  Serial.print("Setup page HTML size: ");
  Serial.println(bodyLen);
  
  sendHttpHeader(client, bodyLen, "text/html");
  client.write((const uint8_t*)body, bodyLen);
  client.flush();
  Serial.println("Setup page sent!");
}

void sendSuccessPage(WiFiClient &client) {
  char body[512];
  int bodyLen = snprintf(body, sizeof(body),
    "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='2;url=/'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Saved</title>"
    "<style>*{box-sizing:border-box}body{font-family:-apple-system,BlinkMacSystemFont,Arial,sans-serif;margin:0;padding:20px;background:#f5f5f7;max-width:500px;margin:0 auto;text-align:center}"
    ".c{background:#fff;border-radius:12px;padding:30px;box-shadow:0 1px 3px rgba(0,0,0,0.1)}"
    "h2{color:#34c759;margin:0 0 12px}"
    "p{color:#666;margin:0}"
    "</style></head><body>"
    "<div class='c'><h2>Configuration Saved!</h2>"
    "<p>Redirecting to home...</p></div></body></html>");
  
  if (bodyLen < 0) {
    bodyLen = 0;
  } else if (bodyLen >= (int)sizeof(body)) {
    bodyLen = sizeof(body) - 1;
    Serial.print("WARNING: Success page HTML truncated! Buffer size: ");
    Serial.println(sizeof(body));
  }
  
  Serial.print("Success page HTML size: ");
  Serial.println(bodyLen);
  
  sendHttpHeader(client, bodyLen, "text/html");
  client.write((const uint8_t*)body, bodyLen);
  client.flush();
}

// URL decode helper function
void urlDecode(char* dst, const char* src, int maxLen) {
  int i = 0, j = 0;
  while (src[i] && j < maxLen - 1) {
    if (src[i] == '%' && src[i+1] && src[i+2]) {
      char hex[3] = {src[i+1], src[i+2], 0};
      dst[j++] = (char)strtol(hex, NULL, 16);
      i += 3;
    } else if (src[i] == '+') {
      dst[j++] = ' ';
      i++;
    } else {
      dst[j++] = src[i++];
    }
  }
  dst[j] = 0;
}

// Parse query parameter helper
bool getQueryParam(const String& request, const char* param, char* value, int maxLen) {
  String searchStr = String(param) + "=";
  int startIdx = request.indexOf(searchStr);
  if (startIdx == -1) return false;
  
  startIdx += searchStr.length();
  int endIdx = request.indexOf('&', startIdx);
  if (endIdx == -1) {
    endIdx = request.indexOf(' ', startIdx);
  }
  if (endIdx == -1) return false;
  
  String encoded = request.substring(startIdx, endIdx);
  urlDecode(value, encoded.c_str(), maxLen);
  return true;
}

// WiFi management function with retry logic
void manageWiFi() {
  unsigned long now = millis();
  
  // Check WiFi status every 5 seconds
  if (now - lastWifiCheck > WIFI_CHECK_INTERVAL) {
    lastWifiCheck = now;
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected!");
      if (displayEnabled) {
        Screen.print(3, "WiFi lost!");
      }
      webServerStarted = false;
      mqttConnected = false;
      
      // Try to reconnect every 30 seconds
      if (now - lastWifiRetry > WIFI_RETRY_INTERVAL) {
        lastWifiRetry = now;
        Serial.println("Attempting WiFi reconnection...");
        if (displayEnabled) {
          Screen.print(3, "WiFi retry...");
        }
        
        WiFi.disconnect();
        delay(1000);
        WiFi.begin(config.ssid, config.password);
        
        // Wait up to 10 seconds for connection
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
          delay(500);
          Serial.print(".");
          attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("\nWiFi reconnected!");
          Serial.print("IP: ");
          Serial.println(WiFi.localIP());
          if (displayEnabled) {
            char ipStr[16];
            sprintf(ipStr, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
            Screen.print(3, ipStr);
          }
        } else {
          Serial.println("\nWiFi retry failed!");
          if (displayEnabled) {
            Screen.print(3, "WiFi failed!");
          }
        }
      }
    } else {
      // WiFi is connected
      if (!webServerStarted) {
        // Start web server if not already started
        webServer.begin();
        webServerStarted = true;
        Serial.println("Web server restarted");
        Serial.print("Access control panel at: http://");
        Serial.println(WiFi.localIP());
      }
    }
  }
}

// Web server thread function - runs independently
void webServerThreadFunc() {
  Serial.println("Web server thread started");
  
  while (1) {
    if (WiFi.status() == WL_CONNECTED && webServerStarted) {
      WiFiClient client = webServer.available();
      
      if (client) {
        Serial.println(">>> Web client connected <<<");
        
        // Wait up to 2 seconds for incoming data
        unsigned long start = millis();
        while (!client.available() && client.connected() && (millis() - start < 2000)) {
          Thread::wait(1);
        }
        
        if (!client.available()) {
          Serial.println("No data received within timeout, closing");
          client.stop();
          continue;
        }
        
        // Read the first request line: "GET /path HTTP/1.1"
        String requestLine = "";
        unsigned long readStart = millis();
        while (client.available() && requestLine.length() < 512 && millis() - readStart < 100) {
          char c = client.read();
          if (c == '\r') continue;
          if (c == '\n') break;
          requestLine += c;
        }
        requestLine.trim();
        
        Serial.print("HTTP request line: ");
        Serial.println(requestLine);
        
        // Parse HTTP method and path
        bool isPost = requestLine.startsWith("POST ");
        bool isGet = requestLine.startsWith("GET ");
        
        if (!isGet && !isPost) {
          Serial.println("Unsupported HTTP method, closing");
          client.stop();
          continue;
        }
        
        // Extract the path between the first and second spaces
        int firstSpace = requestLine.indexOf(' ');
        int secondSpace = requestLine.indexOf(' ', firstSpace + 1);
        
        String path = "/";
        if (firstSpace > 0 && secondSpace > firstSpace) {
          path = requestLine.substring(firstSpace + 1, secondSpace);
        }
        
        Serial.print("Raw path: ");
        Serial.println(path);
        
        // Keep full path for save-config (needs query params)
        String fullPath = path;
        
        // Strip any query string so /control?x=1 becomes /control
        int qPos = path.indexOf('?');
        if (qPos > 0) {
          path = path.substring(0, qPos);
        }
        
        Serial.print("Normalized path: ");
        Serial.println(path);
        
        // Parse headers (need Content-Length for POST requests)
        size_t contentLength = 0;
        String headerLine = "";
        unsigned long headerStart = millis();
        while (client.connected() && millis() - headerStart < 100) {
          if (!client.available()) {
            Thread::wait(1);
            continue;
          }
          char c = client.read();
          if (c == '\r') continue;
          if (c == '\n') {
            if (headerLine.length() == 0) break; // End of headers
            
            // Parse Content-Length header
            if (isPost && headerLine.startsWith("Content-Length:")) {
              String lengthStr = headerLine.substring(15);
              lengthStr.trim();
              contentLength = (size_t)atoi(lengthStr.c_str());
              Serial.print("Content-Length: ");
              Serial.println(contentLength);
            }
            
            headerLine = "";
          } else {
            headerLine += c;
          }
        }
        
        // Process commands
        if (path.length() > 0) {
          unsigned long now = millis();
          
          // Route: Main page
          if (path == "/") {
            Serial.println("Serving main page");
            sendMainPage(client);
            client.flush();
            Thread::wait(10);
            client.stop();
            Serial.println("Client connection closed");
            continue;
          }
          
          // Route: Control page
          else if (path == "/control") {
            Serial.println("Serving control page");
            sendControlPage(client);
            client.flush();
            Thread::wait(10);
            client.stop();
            Serial.println("Client connection closed");
            continue;
          }
          
          // Route: Telemetry page
          else if (path == "/telemetry") {
            Serial.println("Serving telemetry page");
            sendTelemetryPage(client);
            client.flush();
            Thread::wait(10);
            client.stop();
            Serial.println("Client connection closed");
            continue;
          }
          
          // Route: Setup page
          else if (path == "/setup") {
            Serial.println("Serving setup page");
            sendSetupPage(client);
            client.flush();
            Thread::wait(10);
            client.stop();
            Serial.println("Client connection closed");
            continue;
          }
          
          // Route: Save configuration
          else if (path == "/save-config") {
            Serial.println("Saving configuration from web form...");
            
            // Parse all parameters from fullPath
            char tempBuffer[64];
            
            if (getQueryParam(fullPath, "deviceId", tempBuffer, sizeof(tempBuffer))) {
              strncpy(config.deviceId, tempBuffer, sizeof(config.deviceId) - 1);
              config.deviceId[sizeof(config.deviceId) - 1] = 0;
            }
            if (getQueryParam(fullPath, "model", tempBuffer, sizeof(tempBuffer))) {
              strncpy(config.model, tempBuffer, sizeof(config.model) - 1);
              config.model[sizeof(config.model) - 1] = 0;
            }
            if (getQueryParam(fullPath, "location", tempBuffer, sizeof(tempBuffer))) {
              strncpy(config.location, tempBuffer, sizeof(config.location) - 1);
              config.location[sizeof(config.location) - 1] = 0;
            }
            if (getQueryParam(fullPath, "ssid", tempBuffer, sizeof(tempBuffer))) {
              strncpy(config.ssid, tempBuffer, sizeof(config.ssid) - 1);
              config.ssid[sizeof(config.ssid) - 1] = 0;
            }
            if (getQueryParam(fullPath, "password", tempBuffer, sizeof(tempBuffer))) {
              strncpy(config.password, tempBuffer, sizeof(config.password) - 1);
              config.password[sizeof(config.password) - 1] = 0;
            }
            if (getQueryParam(fullPath, "mqttServer", tempBuffer, sizeof(tempBuffer))) {
              strncpy(config.mqttServer, tempBuffer, sizeof(config.mqttServer) - 1);
              config.mqttServer[sizeof(config.mqttServer) - 1] = 0;
            }
            if (getQueryParam(fullPath, "mqttPort", tempBuffer, sizeof(tempBuffer))) {
              int port = atoi(tempBuffer);
              if (port > 0 && port <= 65535) {
                config.mqttPort = port;
              }
            }
            if (getQueryParam(fullPath, "mqttTopic", tempBuffer, sizeof(tempBuffer))) {
              strncpy(config.mqttTopic, tempBuffer, sizeof(config.mqttTopic) - 1);
              config.mqttTopic[sizeof(config.mqttTopic) - 1] = 0;
            }
            
            // Save to Flash
            Serial.println("Writing configuration to Flash...");
            if (saveConfigToFlash()) {
              Serial.println("Configuration saved successfully!");
              sendSuccessPage(client);
              client.flush();
              Thread::wait(10);
              client.stop();
            } else {
              Serial.println("Failed to save configuration!");
              sendMainPage(client);
              client.flush();
              Thread::wait(10);
              client.stop();
            }
            continue;
          }
          
          // Control commands with debouncing
          else if (path == "/led" && now - lastLedChange > DEBOUNCE_DELAY) {
            // Check query parameter for state
            if (fullPath.indexOf("state=on") > 0) {
              ledEnabled = true;
              lastLedChange = now;
            } else if (fullPath.indexOf("state=off") > 0) {
              ledEnabled = false;
              rgbLED.turnOff();
              lastLedChange = now;
            }
            sendControlPage(client);
            client.flush();
            Thread::wait(10);
            client.stop();
            Serial.println("Client connection closed");
            continue;
          }
          else if (path == "/display" && now - lastDisplayChange > DEBOUNCE_DELAY) {
            // Check query parameter for state
            if (fullPath.indexOf("state=on") > 0) {
              displayEnabled = true;
              lastDisplayChange = now;
              Screen.init();
              Screen.print(0, config.deviceId);
              Screen.print(1, "Web Control");
              if (WiFi.status() == WL_CONNECTED) {
                char ipStr[16];
                sprintf(ipStr, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
                Screen.print(3, ipStr);
              }
            } else if (fullPath.indexOf("state=off") > 0) {
              displayEnabled = false;
              lastDisplayChange = now;
              Screen.clean();
            }
            sendControlPage(client);
            client.flush();
            Thread::wait(10);
            client.stop();
            Serial.println("Client connection closed");
            continue;
          }
          else if (path == "/wifiled") {
            if (fullPath.indexOf("state=on") > 0) {
              wifiLedEnabled = true;
              pinMode(LED_WIFI, OUTPUT);
              digitalWrite(LED_WIFI, HIGH);
              Serial.println("WiFi LED turned ON");
            } else if (fullPath.indexOf("state=off") > 0) {
              wifiLedEnabled = false;
              pinMode(LED_WIFI, OUTPUT);
              digitalWrite(LED_WIFI, LOW);
              Serial.println("WiFi LED turned OFF");
            }
            sendControlPage(client);
            client.flush();
            Thread::wait(10);
            client.stop();
            Serial.println("Client connection closed");
            continue;
          }
          else if (path == "/azureled") {
            if (fullPath.indexOf("state=on") > 0) {
              azureLedEnabled = true;
              pinMode(LED_AZURE, OUTPUT);
              digitalWrite(LED_AZURE, HIGH);
              Serial.println("Azure LED turned ON");
            } else if (fullPath.indexOf("state=off") > 0) {
              azureLedEnabled = false;
              pinMode(LED_AZURE, OUTPUT);
              digitalWrite(LED_AZURE, LOW);
              Serial.println("Azure LED turned OFF");
            }
            sendControlPage(client);
            client.flush();
            Thread::wait(10);
            client.stop();
            Serial.println("Client connection closed");
            continue;
          }
          else if (path == "/userled") {
            if (fullPath.indexOf("state=on") > 0) {
              userLedEnabled = true;
              pinMode(LED_USER, OUTPUT);
              digitalWrite(LED_USER, HIGH);
              Serial.println("User LED turned ON");
            } else if (fullPath.indexOf("state=off") > 0) {
              userLedEnabled = false;
              pinMode(LED_USER, OUTPUT);
              digitalWrite(LED_USER, LOW);
              Serial.println("User LED turned OFF");
            }
            sendControlPage(client);
            client.flush();
            Thread::wait(10);
            client.stop();
            Serial.println("Client connection closed");
            continue;
          }
          else if (path == "/reset") {
            Serial.println("RESET requested via web interface");
            sendControlPage(client);
            client.flush();
            Thread::wait(10);
            client.stop();
            Thread::wait(100);
            NVIC_SystemReset();
          }
          else if (path == "/watchdog") {
            if (fullPath.indexOf("state=enable") > 0) {
              watchdogEnabled = true;
              lastSuccessfulNetworkActivity = millis(); // Reset timer when enabling
              Serial.println("Watchdog ENABLED via web interface");
            } else if (fullPath.indexOf("state=disable") > 0) {
              watchdogEnabled = false;
              Serial.println("Watchdog DISABLED via web interface");
            }
            sendControlPage(client);
            client.flush();
            Thread::wait(10);
            client.stop();
            Serial.println("Client connection closed");
            continue;
          }
          
          // Unknown path - serve main page
          else {
            Serial.print("Unknown path: ");
            Serial.println(path);
            Serial.println("Serving main page");
            sendMainPage(client);
            client.flush();
            Thread::wait(10);
            client.stop();
            Serial.println("Client connection closed");
            continue;
          }
        }
        
        // If we get here with no path, serve main page
        Serial.println("No path specified, serving main page");
        sendMainPage(client);
        client.flush();
        Thread::wait(10);
        client.stop();
        Serial.println("Client connection closed");
      }
    }
    
    Thread::wait(50);  // Yield to other threads
  }
}

// Arduino setup function - called once at startup
void setup() {
  // Initialize hardware
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n\n");
  Serial.println("=================================");
  Serial.println("=== PURE STM32 CODE STARTING ===");
  Serial.println("=================================");
  Serial.println("\n");
  
  // Try to load configuration from Flash first
  if (!loadConfigFromFlash()) {
    // If no valid config found, load defaults
    loadDefaultConfig();
    Serial.println("Using default configuration (Flash storage empty/invalid)");
  } else {
    Serial.println("Configuration loaded from Flash storage");
  }
  
  // Check if user wants to configure the device
  if (checkForConfigurationMode()) {
    configureDevice();
  }
  
  // Show current configuration
  Serial.println("\n=== CURRENT CONFIGURATION ===");
  Serial.print("Device ID: "); Serial.println(config.deviceId);
  Serial.print("WiFi SSID: "); Serial.println(config.ssid);
  Serial.print("MQTT Server: "); Serial.println(config.mqttServer);
  Serial.print("MQTT Port: "); Serial.println(config.mqttPort);
  Serial.print("MQTT Topic: "); Serial.println(config.mqttTopic);
  Serial.println();
  
  // Aggressively disable any Azure background services
  Serial.println("Disabling Azure background services...");
  
  // Disable any HTTP/telemetry services that might auto-start
  #ifdef DISABLE_AZURE_HTTP
  Serial.println("Azure HTTP disabled via build flag");
  #endif
  
  // Initialize OLED display
  Serial.println("Initializing OLED display...");
  Screen.init();
  Screen.print(0, config.deviceId);
  Screen.print(2, "Sensors + MQTT");
  

  
  // Initialize I2C and sensors
  Serial.println("Initializing sensors...");
  i2c = new DevI2C(D14, D15);
  
  ht_sensor = new HTS221Sensor(*i2c);
  ht_sensor->init(NULL);
  ht_sensor->enable();
  
  pressure_sensor = new LPS22HBSensor(*i2c);
  pressure_sensor->init(NULL);
  
  acc_gyro = new LSM6DSLSensor(*i2c, D4, D5);
  acc_gyro->init(NULL);
  acc_gyro->enableAccelerator();
  acc_gyro->enableGyroscope();
  
  magnetometer = new LIS2MDLSensor(*i2c);
  magnetometer->init(NULL);
  
  Serial.println("All sensors initialized: temp, humidity, pressure, accelerometer, gyroscope, magnetometer!");
  
  // Simple WiFi connection without Azure framework
  Serial.println("Connecting to WiFi...");
  Screen.print(3, "WiFi connecting...");
  WiFi.begin(config.ssid, config.password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    // Display IP address on OLED
    char ipStr[16];
    sprintf(ipStr, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    Screen.print(3, ipStr);
    
    // Start web server
    webServer.begin();
    webServerStarted = true;
    
    // Create web server thread using mbed Thread with 8KB stack
    webServerThread_ptr = new rtos::Thread(osPriorityNormal, 8192);
    if (webServerThread_ptr == NULL) {
      Serial.println("Failed to create web server thread!");
    } else {
      webServerThread_ptr->start(callback(webServerThreadFunc));
      Serial.println("Web server thread created and started");
    }
    
    Serial.println("=== WEB SERVER STARTED ===");
    Serial.print("Listening on: ");
    Serial.print(WiFi.localIP());
    Serial.println(":80");
    Serial.print("Access control panel at: http://");
    Serial.println(WiFi.localIP());
    Serial.println("==========================");
  } else {
    Serial.println("\nWiFi failed!");
    Screen.print(3, "WiFi failed!");
    webServerStarted = false;
  }
  
  // Initialize network watchdog timer
  lastSuccessfulNetworkActivity = millis();
  Serial.println("Network watchdog initialized (15 minute timeout)");
  
  // After everything is initialized, turn off the status LEDs
  disableStatusLedsOnce();
}

// Arduino loop function - called repeatedly
void loop() {
  static int counter = 0;
  static unsigned long lastSensorRead = 0;
  static unsigned long lastMqttPublish = 0;  // Separate timer for MQTT
  
  unsigned long now = millis();
  
  // Keep front-panel LEDs in their current state (only force off if not manually enabled)
  if (!wifiLedEnabled) digitalWrite(LED_WIFI,  LOW);
  if (!azureLedEnabled) digitalWrite(LED_AZURE, LOW);
  if (!userLedEnabled) digitalWrite(LED_USER,  LOW);
  
  // Check network watchdog (monitors connectivity and reboots if needed)
  checkNetworkWatchdog();
  
  // Manage WiFi connection with retry logic
  manageWiFi();
  
  // Try to connect MQTT if not connected (non-blocking retry)
    static unsigned long lastMqttAttempt = 0;
    if (!mqttConnected && WiFi.status() == WL_CONNECTED && (now - lastMqttAttempt > 10000)) {
      lastMqttAttempt = now;
      Serial.println("Attempting MQTT connection...");
      Serial.print("Device IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("Gateway: ");
      Serial.println(WiFi.gatewayIP());
      if (displayEnabled) {
        Screen.print(2, "MQTT connecting...");
      }
      mqttConnected = connectMQTT();
      if (mqttConnected) {
        if (displayEnabled) {
          Screen.print(2, "MQTT connected!");
        }
        Serial.println("MQTT connected successfully!");
      } else {
        if (displayEnabled) {
          Screen.print(2, "MQTT failed!");
        }
        Serial.println("MQTT connection failed, will retry in 10 seconds");
      }
    }

    // Read sensors every 30 seconds
    if (now - lastSensorRead > 30000) {
      lastSensorRead = now;
      
      Serial.print("\n=== Sensor Reading #");
      Serial.print(counter);
      Serial.println(" ===");
      
      // Read Temperature and Humidity
      float temperature, humidity;
      ht_sensor->getTemperature(&temperature);
      ht_sensor->getHumidity(&humidity);
      
      // Apply temperature calibration offset
      temperature += TEMPERATURE_OFFSET;
      
      // Store in global variables for web display
      lastTemperature = temperature;
      lastHumidity = humidity;
      
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" °C");
      
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");
      
      // Read Pressure
      float pressure;
      pressure_sensor->getPressure(&pressure);
      // Apply calibration offset to match actual atmospheric pressure
      pressure += PRESSURE_OFFSET;
      lastPressure = pressure;
      
      Serial.print("Pressure: ");
      Serial.print(pressure);
      Serial.println(" mbar");
      
      // Read Accelerometer
      int axes[3];
      acc_gyro->getXAxes(axes);
      float accel_x = axes[0] / 1000.0f;  // Convert to g
      float accel_y = axes[1] / 1000.0f;
      float accel_z = axes[2] / 1000.0f;
      lastAccelX = accel_x;
      lastAccelY = accel_y;
      lastAccelZ = accel_z;
      
      Serial.print("Accelerometer: X=");
      Serial.print(accel_x, 3);
      Serial.print("g Y=");
      Serial.print(accel_y, 3);
      Serial.print("g Z=");
      Serial.print(accel_z, 3);
      Serial.println("g");
      
      // Read Gyroscope
      int gyro_axes[3];
      acc_gyro->getGAxes(gyro_axes);
      float gyro_x = gyro_axes[0] / 1000.0f;  // Convert to dps
      float gyro_y = gyro_axes[1] / 1000.0f;
      float gyro_z = gyro_axes[2] / 1000.0f;
      lastGyroX = gyro_x;
      lastGyroY = gyro_y;
      lastGyroZ = gyro_z;
      
      Serial.print("Gyroscope: X=");
      Serial.print(gyro_x, 2);
      Serial.print("dps Y=");
      Serial.print(gyro_y, 2);
      Serial.print("dps Z=");
      Serial.print(gyro_z, 2);
      Serial.println("dps");
      
      // Read Magnetometer
      int mag_axes[3];
      magnetometer->getMAxes(mag_axes);
      float mag_x = mag_axes[0] / 1000.0f;  // Convert to gauss
      float mag_y = mag_axes[1] / 1000.0f;
      float mag_z = mag_axes[2] / 1000.0f;
      lastMagX = mag_x;
      lastMagY = mag_y;
      lastMagZ = mag_z;
      
      Serial.print("Magnetometer: X=");
      Serial.print(mag_x, 3);
      Serial.print("G Y=");
      Serial.print(mag_y, 3);
      Serial.print("G Z=");
      Serial.print(mag_z, 3);
      Serial.println("G");
      
      // Update display (only if enabled)
      if (displayEnabled) {
        char tempStr[32];
        sprintf(tempStr, "T:%.1fC H:%.0f%%", temperature, humidity);
        Screen.print(1, tempStr);
        
        char pressStr[32];
        sprintf(pressStr, "P:%.0fmbar", pressure);
        Screen.print(2, pressStr);
        
        // Show IP address on line 3 if connected
        if (WiFi.status() == WL_CONNECTED) {
          char ipStr[32];
          sprintf(ipStr, "IP:%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
          Screen.print(3, ipStr);
        } else {
          Screen.print(3, "WiFi: Connecting...");
        }
      }
      
      counter++;
    }
    
    // Publish to MQTT every 30 seconds (separate from sensor reading)
    if (mqttConnected && now - lastMqttPublish > 30000) {
      lastMqttPublish = now;
      
      // Create JSON payload with latest sensor values
      char jsonPayload[512];
      sprintf(jsonPayload, 
        "{\"device\":\"%s\",\"model\":\"%s\",\"location\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f,"
        "\"accel\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},"
        "\"gyro\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f},"
        "\"mag\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f}}",
        config.deviceId, config.model, config.location, lastTemperature, lastHumidity, lastPressure,
        lastAccelX, lastAccelY, lastAccelZ,
        lastGyroX, lastGyroY, lastGyroZ,
        lastMagX, lastMagY, lastMagZ);
      
      Serial.print("MQTT JSON: ");
      Serial.println(jsonPayload);
      
      if (publishMQTT(config.mqttTopic, jsonPayload)) {
        Serial.println("MQTT published successfully");
        // Update watchdog - successful network activity
        lastSuccessfulNetworkActivity = millis();
      } else {
        Serial.println("MQTT publish failed, will retry");
        if (displayEnabled) {
          Screen.print(3, "MQTT failed!");
        }
        // Don't disconnect - just retry next time
      }
    }
    
    // Heartbeat LED - use RGB LED instead of built-in (only if enabled)
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    if (ledEnabled && now - lastBlink > 1000) {
      lastBlink = now;
      ledState = !ledState;
      if (ledState) {
        rgbLED.setColor(0, 255, 0);  // Green when on
      } else {
      rgbLED.turnOff();    // Off
    }
  }
  
  delay(50);   // Reasonable delay for stable operation
}
