# Az3166 Firmware Release v1.0.2

**Release Date:** January 24, 2026

## What's New in v1.0.2

### Core Improvements
- ✅ **Optimized Build Configuration** - Comprehensive Azure service disabling for reduced memory footprint
- ✅ **Firmware Version Tracking** - Added firmware version constant for better release management
- ✅ **Enhanced Stability** - Improved memory usage and build optimizations

### Build Optimizations
- Added extensive build flags to completely disable unused Azure services
- Disabled background tasks and Azure threading for cleaner execution
- Removed HTTP client and telemetry overhead
- Streamlined build process with both app and legacy environments

### Technical Improvements
- Complete Azure IoT service isolation
- Reduced code complexity
- Improved build consistency across environments

## Technical Details

### Memory Usage
- **RAM:** 17.5% (45,804 bytes / 262,144 bytes)
- **Flash:** 24.3% (254,396 bytes / 1,048,576 bytes)

### Build Flags Added
- `DDISABLE_AZURE_HTTP`
- `DDISABLE_HTTP_CLIENT`
- `DNO_HTTP_TELEMETRY`
- `DDISABLE_SYSTEM_TELEMETRY`
- `DDISABLE_ALL_AZURE_SERVICES`
- `DNO_BACKGROUND_TASKS`
- `DDISABLE_AZURE_THREAD`

### Platform Details
- **Platform:** ST STM32 (19.4.0)
- **Board:** MXChip AZ3166
- **MCU:** STM32F412ZGT6 @ 100MHz
- **Framework:** Arduino
- **Upload Protocol:** ST-Link

## Features from Previous Releases

### Network Watchdog (v1.0.1)
- Automatic device reboot if network connectivity is lost
- Configurable timeout (default: 1 minute)
- Web interface controls

### Core Features (v1.0.0)
- Multi-sensor support (temperature, humidity, pressure, motion)
- MQTT connectivity
- WiFi management
- OLED display
- Web configuration interface
- Flash-based persistent storage

## Installation

### Prerequisites
- PlatformIO installed with STM32 platform
- Az3166 board connected via USB (ST-Link)
- Virtual environment with PlatformIO

### Upload Command
```bash
cd ${HOME}/src/az3166-blink && source .venv/bin/activate && pio run --target upload
```

### Monitoring
```bash
pio device monitor --baud 115200
```

## Configuration

Device can be configured via:
1. Serial console at 115200 baud
2. Web interface (when connected to WiFi)
3. Flash memory persistent storage

## Known Issues
None reported for this release.

## Upgrade Notes
- This is a drop-in replacement for v1.0.1
- No configuration changes required
- All existing settings preserved in flash memory

## Support
For issues or questions, please refer to the project documentation.

---

**Release Artifacts:**
- Firmware binary built for both `az3166_app` and `mxchip_az3166` environments
- Source code available in project repository
- Build verified and uploaded successfully
