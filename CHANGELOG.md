# Changelog

All notable changes to the AZ3166 Sensor Station project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2025-01-XX

### Added
- **OTA (Over-The-Air) Firmware Updates**
  - Bootloader-based dual-slot update system with staging area
  - CRC32 validation before and after firmware copy
  - Web-based firmware upload via POST /ota endpoint
  - Complete OTA memory layout: Bootloader (16KB) + App (240KB) + Staging (240KB) + Metadata (2KB)
  - Automatic bootloader validation and application jump
  - Safe update process with rollback protection
  - Two PlatformIO build environments: `az3166_boot` and `az3166_app`
  - Custom linker scripts for bootloader (boot_flash.ld) and application (app_flash.ld)
  - Comprehensive OTA documentation in `OTA_GUIDE.md`
- File upload UI in Control page for firmware updates with progress tracking
- Flash helper functions: CRC32 calculation, sector erase, word write operations
- OTA metadata structure with magic bytes, size, CRC32, version, and flags
- HTTP POST method support in web server

### Changed
- HTTP server now supports both GET and POST methods
- Control page buffer increased from 2560 to 3072 bytes for OTA UI
- Enhanced HTTP header parsing to extract Content-Length for POST requests
- Application now links at 0x08004000 instead of 0x08000000 (bootloader at base)
- Added conditional OTA compilation with `OTA_ENABLED` flag
- Updated README.md to highlight OTA capability

## [1.0.0] - 2025-09-10

### Added
- Initial release of AZ3166 Sensor Station
- Pure STM32 implementation bypassing Azure framework
- Multi-sensor support (temperature, humidity, pressure, accelerometer, gyroscope, magnetometer)
- Persistent configuration storage in flash memory
- Interactive serial configuration interface
- Direct MQTT publishing with JSON sensor data
- WiFi connectivity without Azure dependencies
- OLED display for real-time sensor readings
- Comprehensive build flags to disable Azure services
- Flash memory checksum validation for configuration integrity

### Features
- **Sensors**: HTS221 (temp/humidity), LPS22HB (pressure), LSM6DSL (accel/gyro), LIS2MDL (magnetometer)
- **Connectivity**: WiFi + MQTT publishing
- **Configuration**: Serial interface with flash storage
- **Display**: OLED real-time sensor display
- **Data Format**: JSON sensor data with timestamps
- **Build System**: PlatformIO with STM32 platform

### Technical Details
- Platform: STM32F4 (MXChip AZ3166)
- Framework: Arduino (with Azure services disabled)
- Flash Storage: STM32 internal flash sector 10
- Communication: MQTT over WiFi
- Serial Baud Rate: 115200
- Sensor Update Rate: 5 seconds
