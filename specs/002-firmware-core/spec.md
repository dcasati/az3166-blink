# Feature Specification: Az3166 Sensor Station Core Firmware

**Feature Branch**: `002-firmware-core`  
**Created**: 2026-01-17  
**Status**: Production  
**Version**: 1.0.2  
**Input**: Complete embedded firmware for IoT sensor monitoring with direct MQTT connectivity

## User Scenarios & Testing

### User Story 1 - Autonomous Sensor Monitoring (Priority: P1)

A user deploys the Az3166 device in a remote location (basement, garage, outdoors). The device must continuously collect environmental and motion sensor data, maintain WiFi connectivity, publish data to MQTT broker every 30 seconds, and operate unattended for weeks without intervention.

**Why this priority**: Core value proposition. Device must be "set and forget" for IoT monitoring applications.

**Independent Test**: Deploy device with valid WiFi and MQTT credentials, observe continuous data publishing for 48+ hours without manual intervention. Verify all sensor readings appear in MQTT messages.

**Acceptance Scenarios**:

1. **Given** device powered on with valid configuration, **When** 30 seconds pass, **Then** device reads all sensors (temperature, humidity, pressure, accelerometer, gyroscope, magnetometer) and publishes JSON to MQTT
2. **Given** sensors are operational, **When** environmental conditions change, **Then** device reports updated values within 30-second cycle
3. **Given** device is operating normally, **When** 24+ hours pass, **Then** device continues publishing without crashes or memory leaks

---

### User Story 2 - Field Configuration via Serial (Priority: P1)

A technician needs to configure a device in the field without recompiling firmware. Using a USB serial connection, they can set device ID, location, WiFi credentials, and MQTT broker settings. Configuration persists across reboots and power cycles.

**Why this priority**: Critical for deployment at scale. Same firmware image must work across multiple devices with different configurations.

**Independent Test**: Connect new device via serial, press 'C' within 5 seconds of boot, configure all parameters, reboot, verify device connects with new settings.

**Acceptance Scenarios**:

1. **Given** device boots up, **When** user presses 'C' within 5 seconds, **Then** device enters configuration mode and prompts for parameters
2. **Given** configuration mode is active, **When** user provides WiFi SSID, password, MQTT broker, port, topic, device ID, and location, **Then** device saves all values to flash memory with checksum
3. **Given** configuration is saved, **When** device reboots, **Then** device loads configuration from flash and connects using stored credentials
4. **Given** invalid configuration is detected (checksum fail), **When** device boots, **Then** device uses default configuration and logs error to serial console

---

### User Story 3 - Web-Based Device Control (Priority: P2)

A user on the same WiFi network accesses the device's web interface via browser. They can view real-time sensor telemetry, control LEDs and display, view MQTT connection status, enable/disable watchdog, and trigger system reboot without physical access.

**Why this priority**: Convenient remote management and diagnostics without serial cable. Essential for deployed devices.

**Independent Test**: Connect to http://[device-ip]/ in browser, navigate all pages, toggle controls, verify device responds and displays reflect changes.

**Acceptance Scenarios**:

1. **Given** device has WiFi connection and IP address, **When** user navigates to device IP in browser, **Then** main page displays with device name, MQTT status, and firmware version
2. **Given** user is on control page, **When** user clicks LED ON/OFF buttons, **Then** RGB LED changes state immediately
3. **Given** user is on control page, **When** user clicks Display ON/OFF, **Then** OLED screen enables/disables accordingly
4. **Given** user is on telemetry page, **When** page loads, **Then** all current sensor readings display with units and device metadata
5. **Given** user clicks RESET button, **When** confirmation occurs, **Then** device reboots within 2 seconds

---

### User Story 4 - Automatic Network Recovery (Priority: P1)

Device loses WiFi or MQTT connectivity due to network issues, router reboot, or broker downtime. Device must automatically detect failures, attempt reconnection, and if unsuccessful after 5 minutes, reboot to fully recover. No manual intervention required.

**Why this priority**: Reliability requirement for unattended operation. Documented in [001-mqtt-watchdog-fix specification](../001-mqtt-watchdog-fix/spec.md).

**Independent Test**: See [001-mqtt-watchdog-fix/spec.md](../001-mqtt-watchdog-fix/spec.md) for complete test scenarios.

**Acceptance Scenarios**:

1. **Given** WiFi connection is lost, **When** 30 seconds pass, **Then** device attempts WiFi reconnection
2. **Given** MQTT connection fails, **When** 10 seconds pass, **Then** device attempts MQTT reconnection  
3. **Given** no successful MQTT publish for 5 minutes, **When** watchdog timeout reached, **Then** device reboots
4. **Given** device reboots from watchdog, **When** network is available, **Then** device reconnects and resumes normal operation

---

### User Story 5 - Calibrated Sensor Readings (Priority: P2)

Environmental sensor readings (temperature, pressure) require calibration offsets to match reference instruments or local conditions. Device applies fixed calibration constants defined in firmware.

**Why this priority**: Sensor accuracy directly impacts data quality. Calibration ensures readings match user expectations.

**Independent Test**: Compare device temperature reading against calibrated thermometer, verify readings match within ±1°C after applying -2.0°C offset. Similarly test pressure with barometer (±5 mbar after +141.0 mbar offset).

**Acceptance Scenarios**:

1. **Given** temperature sensor reads raw value, **When** value is published, **Then** published value = raw value - 2.0°C
2. **Given** pressure sensor reads raw value, **When** value is published, **Then** published value = raw value + 141.0 mbar
3. **Given** calibration constants are defined in firmware, **When** firmware is recompiled, **Then** all devices use same calibration offsets

---

### User Story 6 - Visual Status Indication (Priority: P3)

Device provides visual feedback via RGB LED (heartbeat when enabled), front-panel LEDs (WiFi, Azure, User - controllable), and OLED display showing sensor readings, IP address, and connection status.

**Why this priority**: Useful for debugging and at-a-glance status checking but not critical for core functionality.

**Independent Test**: Power on device, observe green heartbeat LED, check OLED displays device name and sensor values, verify front-panel LEDs can be toggled via web interface.

**Acceptance Scenarios**:

1. **Given** LED is enabled, **When** device is operating normally, **Then** RGB LED flashes green every 1 second
2. **Given** display is enabled, **When** sensors are read, **Then** OLED displays temperature, humidity, pressure, and IP address
3. **Given** user enables front-panel LED via web, **When** command executes, **Then** corresponding LED (WiFi/Azure/User) turns on
4. **Given** watchdog timeout triggers, **When** device is about to reboot, **Then** RGB LED flashes red as warning

### Edge Cases

- **Flash sector corruption**: Device should detect invalid configuration checksum and use defaults
- **Sensor initialization failure**: Device should log error but continue with remaining functional sensors  
- **MQTT broker hostname vs IP**: Device should support both DNS resolution and direct IP addresses
- **Web server during high load**: Concurrent web requests should not block sensor reading or MQTT publishing
- **Serial configuration timeout**: If user doesn't complete configuration within reasonable time, device should continue with existing/default config
- **Millis() rollover (49+ days)**: All time-based logic should handle rollover gracefully
- **Out of memory**: Device should not crash; monitor RAM usage and log warnings if approaching limits
- **Invalid MQTT JSON**: Malformed JSON should be detected during creation and logged as error
- **Network flapping**: Rapid WiFi connect/disconnect cycles should not cause reboot loops (minimum retry intervals)

## Requirements

### Functional Requirements - Core System

- **FR-001**: System MUST initialize STM32 hardware (GPIO, UART, I2C, WiFi) on boot
- **FR-002**: System MUST disable Azure IoT framework and related services via build flags
- **FR-003**: System MUST operate within resource constraints (max 25% RAM, 50% Flash)
- **FR-004**: System MUST provide serial console at 115200 baud for configuration and diagnostics
- **FR-005**: System MUST display firmware version in serial output and web interface
- **FR-006**: System MUST maintain main loop with 50ms delay for stable operation

### Functional Requirements - Configuration Management

- **FR-010**: System MUST store configuration in STM32 flash sector 10 (address 0x080C0000)
- **FR-011**: Configuration structure MUST include: magic bytes "AZ31", device ID (32 bytes), model (16 bytes), location (32 bytes), WiFi SSID (32 bytes), WiFi password (64 bytes), MQTT server (64 bytes), MQTT port (int), MQTT topic (64 bytes), checksum (uint8_t), padding (3 bytes)
- **FR-012**: System MUST calculate XOR checksum over all configuration fields except checksum field itself
- **FR-013**: System MUST validate configuration checksum on load and use defaults if invalid
- **FR-014**: System MUST provide 5-second window after boot for user to press 'C' to enter configuration mode
- **FR-015**: Configuration mode MUST prompt for all parameters and save to flash with updated checksum
- **FR-016**: System MUST erase flash sector before writing new configuration

### Functional Requirements - WiFi Connectivity

- **FR-020**: System MUST attempt WiFi connection using configured SSID and password
- **FR-021**: System MUST retry WiFi connection every 30 seconds if initial connection fails
- **FR-022**: System MUST monitor WiFi connection status every 5 seconds
- **FR-023**: System MUST cache WiFi status to avoid excessive status checks
- **FR-024**: System MUST support DNS hostname resolution for MQTT broker with caching
- **FR-025**: System MUST display WiFi connection status on serial console and OLED

### Functional Requirements - MQTT Publishing

- **FR-030**: System MUST implement MQTT 3.1 protocol without external library
- **FR-031**: System MUST connect to MQTT broker using configured hostname/IP and port
- **FR-032**: System MUST send MQTT CONNECT packet with client ID from device configuration
- **FR-033**: System MUST wait for CONNACK (0x20) response to verify connection
- **FR-034**: System MUST publish sensor data every 30 seconds to configured topic
- **FR-035**: MQTT payload MUST be valid JSON with all sensor readings and device metadata
- **FR-036**: System MUST use QoS 0 (fire and forget) for publishing
- **FR-037**: System MUST verify TCP socket connection before each publish attempt
- **FR-038**: System MUST close connection and retry when publish fails
- **FR-039**: System MUST attempt MQTT reconnection every 10 seconds when disconnected
- **FR-040**: System MUST cache MQTT broker IP address after successful resolution

### Functional Requirements - Sensor Data Collection

- **FR-050**: System MUST read all sensors every 30 seconds
- **FR-051**: System MUST read HTS221 sensor for temperature and humidity
- **FR-052**: System MUST read LPS22HB sensor for barometric pressure
- **FR-053**: System MUST read LSM6DSL sensor for 3-axis accelerometer and gyroscope
- **FR-054**: System MUST read LIS2MDL sensor for 3-axis magnetometer
- **FR-055**: System MUST apply calibration offset of -2.0°C to temperature readings
- **FR-056**: System MUST apply calibration offset of +141.0 mbar to pressure readings
- **FR-057**: System MUST convert accelerometer values to g-force (divide by 1000)
- **FR-058**: System MUST convert gyroscope values to degrees/second (divide by 1000)
- **FR-059**: System MUST convert magnetometer values to gauss (divide by 1000)
- **FR-060**: System MUST store latest sensor readings in global variables for web display

### Functional Requirements - Web Server

- **FR-070**: System MUST run HTTP web server on port 80
- **FR-071**: System MUST serve main page at / with device status and navigation links
- **FR-072**: System MUST serve control page at /control with device control buttons
- **FR-073**: System MUST serve telemetry page at /telemetry with real-time sensor data JSON
- **FR-074**: System MUST serve setup page at /setup (if implemented)
- **FR-075**: System MUST handle GET requests for /led, /display, /wifiled, /azureled, /userled with state=on/off parameters
- **FR-076**: System MUST handle GET request for /watchdog with state=enable/disable parameter
- **FR-077**: System MUST handle GET request for /reset to trigger device reboot
- **FR-078**: Web pages MUST be mobile-responsive with viewport meta tag
- **FR-079**: System MUST send HTTP 200 OK with no-cache headers for all responses
- **FR-080**: Web server MUST run in separate RTOS thread to avoid blocking main loop
- **FR-081**: System MUST limit web server buffer sizes to prevent memory overflow

### Functional Requirements - Display & LEDs

- **FR-090**: System MUST initialize OLED display (if enabled)
- **FR-091**: OLED MUST display device name on line 0
- **FR-092**: OLED MUST display temperature and humidity on line 1
- **FR-093**: OLED MUST display pressure on line 2
- **FR-094**: OLED MUST display IP address on line 3 when WiFi connected
- **FR-095**: Display state (on/off) MUST be controllable via web interface
- **FR-096**: RGB LED MUST flash green every 1 second when heartbeat enabled
- **FR-097**: RGB LED MUST flash red before watchdog reboot
- **FR-098**: Front-panel LEDs (WiFi, Azure, User) MUST be controllable via web interface
- **FR-099**: System MUST force front-panel LEDs off on initialization unless manually enabled

### Functional Requirements - Network Watchdog

(Detailed in [001-mqtt-watchdog-fix specification](../001-mqtt-watchdog-fix/spec.md))

- **FR-100**: System MUST monitor MQTT connectivity with 5-minute timeout
- **FR-101**: Watchdog timer MUST only reset on successful MQTT publish
- **FR-102**: System MUST reboot via NVIC_SystemReset() when watchdog timeout exceeded
- **FR-103**: System MUST display watchdog warnings every 60 seconds when disconnected
- **FR-104**: Watchdog MUST be enable/disable via web interface
- **FR-105**: System MUST handle millis() rollover in watchdog timer logic

### Key Entities

- **DeviceConfig**: Persistent configuration structure stored in flash memory containing all user-configurable parameters
- **Sensor Readings**: Temperature (°C), Humidity (%), Pressure (mbar), Accelerometer (g), Gyroscope (dps), Magnetometer (gauss)
- **MQTT Connection**: TCP socket connection to broker with connection state flag and client instance
- **WiFiClient**: Network client for MQTT communication with connection status tracking
- **Watchdog Timer**: Millisecond counter tracking time since last successful network activity
- **Web Server**: HTTP server instance running in separate RTOS thread
- **OLED Display**: 128x64 pixel display for local status indication
- **RGB LED**: Programmable LED for visual status feedback

## Success Criteria

### Measurable Outcomes

- **SC-001**: Device successfully reads all 6 sensor types and publishes complete data every 30 seconds
- **SC-002**: Device operates continuously for 7+ days without crashes, reboots (except watchdog), or memory leaks
- **SC-003**: RAM usage remains below 25% (65,536 bytes) during normal operation
- **SC-004**: Flash usage remains below 50% (524,288 bytes) for current firmware
- **SC-005**: Configuration persists across 100+ power cycles without corruption
- **SC-006**: Device reconnects to WiFi within 30 seconds after router reboot
- **SC-007**: Device reconnects to MQTT within 10 seconds after broker restart
- **SC-008**: Web interface responds to requests in under 2 seconds on local network
- **SC-009**: Serial console provides complete diagnostic trail of all operations
- **SC-010**: Device detects and recovers from network failures within 5 minutes maximum

## Assumptions

- STM32F412ZGT6 hardware is functioning correctly (no hardware defects)
- WiFi network uses WPA2 security (not open or WEP)
- MQTT broker supports MQTT 3.1 protocol
- MQTT broker does not require TLS/SSL encryption
- MQTT broker does not require username/password authentication
- Device has sufficient power supply (USB or battery) for continuous operation
- All sensors (HTS221, LPS22HB, LSM6DSL, LIS2MDL) are properly connected via I2C
- OLED display is connected and functioning
- Device is deployed in temperature range supported by sensors (-40°C to +85°C)
- Network allows outbound TCP connections on configured MQTT port

## Dependencies

### Hardware Dependencies
- Azure IoT DevKit (MXChip AZ3166) board
- STM32F412ZGT6 microcontroller (100MHz, 256KB RAM, 1MB Flash)
- ST-Link programmer for firmware upload
- USB cable for serial communication and power

### Software Dependencies
- PlatformIO build system
- ST STM32 platform (ststm32)
- Arduino framework for STM32
- STM32 HAL library
- AZ3166 WiFi library (custom)
- AZ3166 Sensor library (HTS221, LPS22HB, LSM6DSL, LIS2MDL drivers)
- OLED Display library
- RGB LED library
- RTOS (FreeRTOS) for threading

### External Dependencies
- WiFi network (802.11 b/g/n)
- MQTT broker (Mosquitto or compatible)
- DNS server (if using hostname for MQTT broker)

## Out of Scope

### Not Implemented
- Azure IoT Hub integration (deliberately disabled)
- MQTT TLS/SSL encryption
- MQTT broker authentication (username/password)
- MQTT QoS levels 1 or 2
- MQTT subscriptions (device only publishes)
- NTP time synchronization
- Real-time clock (RTC) support
- SD card logging
- Battery level monitoring
- Sleep modes or power management
- OTA (Over-The-Air) firmware updates
- Multiple WiFi network profiles
- WiFi access point mode
- Bluetooth connectivity
- Cloud storage integration
- Email/SMS alerting
- Historical data storage on device

### Future Enhancements
- TLS/SSL for MQTT
- MQTT authentication
- OTA updates (non-Azure)
- SD card data logging
- Battery monitoring
- Sleep mode for power saving
- Multiple sensor calibration profiles
- Web-based configuration (vs serial)
- RESTful API for sensor data
- Time series graphing in web interface

## Technical Architecture

### System Components

1. **Boot & Initialization**: Hardware setup, configuration loading, sensor initialization
2. **WiFi Manager**: Connection establishment, monitoring, reconnection logic
3. **MQTT Client**: Raw protocol implementation, publishing, connection management
4. **Sensor Manager**: Multi-sensor polling, calibration, data collection
5. **Web Server**: HTTP server in separate thread, request handling, response generation
6. **Display Manager**: OLED output, status updates
7. **LED Controller**: RGB and front-panel LED control
8. **Network Watchdog**: Connectivity monitoring, automatic recovery
9. **Configuration Manager**: Flash read/write, checksum validation
10. **Serial Console**: Debug output, configuration interface

### Data Flow

```
Boot → Load Config from Flash
     → Initialize Hardware (GPIO, I2C, UART, WiFi)
     → Connect WiFi
     → Start Web Server Thread
     → Enter Main Loop
          ↓
     Main Loop (50ms cycle):
          → Check Watchdog Timer
          → Manage WiFi Connection
          → Attempt MQTT Connection (if needed)
          → Read Sensors (every 30s)
          → Publish to MQTT (every 30s)
          → Update Display (if enabled)
          → Update Heartbeat LED (every 1s)
          → Handle Web Requests (in separate thread)
```

### Memory Map

- Flash Memory:
  - 0x08000000 - 0x080BFFFF: Firmware code and constants
  - 0x080C0000 - 0x080DFFFF: Configuration storage (Sector 10)
- RAM:
  - Stack: RTOS-managed per thread
  - Heap: ~256KB total (45KB used = 17.5%)
  - Global variables: Sensor readings, connection state, buffers

### Build Flags (Azure Disablement)

```
-DDISABLE_AZURE_IOT_HUB_TELEMETRY
-DDISABLE_OTA_UPDATE
-DDISABLE_TIME_SYNC
-DDISABLE_TELEMETRY
-DDISABLE_AZURE_IOT
-DNO_AZURE_SERVICES
-DNO_TELEMETRY
-DNO_MICROSOFT_TELEMETRY
-DAZURE_IOT_DISABLE
-DTELEMETRY_DISABLE
-DDISABLE_AZURE_HTTP
-DDISABLE_HTTP_CLIENT
-DNO_HTTP_TELEMETRY
-DDISABLE_SYSTEM_TELEMETRY
-DDISABLE_ALL_AZURE_SERVICES
-DNO_BACKGROUND_TASKS
-DDISABLE_AZURE_THREAD
```

## Implementation Details

### File Structure
- **src/main.cpp**: Single-file implementation (1870 lines)
- **platformio.ini**: Build configuration
- **include/README**: Header includes
- **lib/README**: Library dependencies

### Key Functions
- `setup()`: Arduino-style initialization
- `loop()`: Main execution loop
- `loadConfigFromFlash()`: Read persistent configuration
- `saveConfigToFlash()`: Write configuration with checksum
- `configModePrompt()`: Interactive serial configuration
- `connectWiFi()`: WiFi connection establishment
- `connectMQTT()`: MQTT connection with handshake
- `publishMQTT()`: MQTT publish implementation
- `checkNetworkWatchdog()`: Connectivity monitoring
- `webServerLoop()`: HTTP request handling (threaded)
- `sendMainPage()`, `sendControlPage()`, etc.: Web response generation

### Global State Variables
- `DeviceConfig config`: Current configuration
- `bool mqttConnected`: MQTT connection state
- `WiFiClient mqttWifiClient`: MQTT TCP client
- `unsigned long lastSuccessfulNetworkActivity`: Watchdog timer
- `float lastTemperature, lastHumidity, lastPressure`: Latest sensor readings
- `float lastAccelX/Y/Z, lastGyroX/Y/Z, lastMagX/Y/Z`: Motion sensor readings
- `bool displayEnabled, ledEnabled, watchdogEnabled`: Feature flags

## Validation & Testing

### Unit Test Scenarios
- ✅ Configuration read/write with checksum validation
- ✅ Sensor reading with calibration offsets applied
- ✅ MQTT packet construction and parsing
- ✅ WiFi connection state management
- ✅ Watchdog timer and rollover handling
- ✅ Flash sector erase and write operations
- ✅ JSON payload generation for MQTT

### Integration Test Scenarios
- ✅ End-to-end: Boot → Configure → Connect → Publish loop
- ✅ Web interface: All pages render and controls function
- ✅ Network recovery: WiFi and MQTT reconnection
- ✅ Long-term stability: 7+ day continuous operation
- ✅ Configuration persistence: 100+ power cycles
- ✅ Memory stability: No leaks over 48+ hours
- ✅ Multi-sensor: All 6 sensor types read correctly
- ✅ Watchdog: Reboot on 5-minute MQTT failure

### Performance Validation
- Current RAM usage: 17.5% (45,804 / 262,144 bytes) ✅
- Current Flash usage: 24.3% (254,396 / 1,048,576 bytes) ✅
- Sensor read cycle: 30 seconds ✅
- MQTT publish cycle: 30 seconds ✅
- Web response time: < 2 seconds ✅
- Serial baud rate: 115200 ✅

## Version History

### v1.0.2 (2026-01-17)
- Fixed MQTT watchdog connectivity monitoring
- Watchdog timeout increased to 5 minutes
- Watchdog only resets on successful MQTT publish
- Added MQTT socket verification before publish
- Force reconnection on publish failures
- See [001-mqtt-watchdog-fix/spec.md](../001-mqtt-watchdog-fix/spec.md)

### v1.0.1 (2026-01-04)
- Added network watchdog (1-minute timeout)
- Web interface watchdog controls
- Millis() rollover protection
- Watchdog status in control panel

### v1.0.0 (2025-09-10)
- Initial production release
- All core features implemented
- Multi-sensor support
- MQTT publishing
- Web interface
- Persistent configuration
- Serial configuration
- OLED display
- LED control

## Documentation References

- [README.md](../../README.md): User guide and setup
- [WATCHDOG.md](../../WATCHDOG.md): Network watchdog details
- [HOMEASSISTANT.md](../../HOMEASSISTANT.md): Home Assistant integration
- [CHANGELOG.md](../../CHANGELOG.md): Version history
- [AGENTS.md](../../AGENTS.md): AI agent build instructions
- [.specify/memory/constitution.md](../../.specify/memory/constitution.md): Project constitution
- [001-mqtt-watchdog-fix/spec.md](../001-mqtt-watchdog-fix/spec.md): Watchdog fix specification
