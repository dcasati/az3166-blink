# Az3166 Firmware Release v1.0.1

**Release Date:** January 4, 2026

## What's New in v1.0.1

### Network Watchdog
- ✅ **Network Watchdog Feature** - Automatic device reboot if network connectivity is lost
  - Monitors WiFi connectivity continuously
  - Configurable timeout (default: 1 minute)
  - Can be enabled/disabled via web interface
  - Visual feedback on OLED display before reboot
  - Serial console warnings when network is down

### Web Interface Improvements
- ✅ Added watchdog control buttons (ON/OFF) to control panel
- ✅ Watchdog status indicator shows ENABLED/DISABLED state
- ✅ Increased HTML buffer size for control page (3200 → 4096 bytes)

### Reliability Improvements
- ✅ Automatic recovery from network failures
- ✅ Handles millis() rollover (49-day uptime protection)
- ✅ Reset timer when watchdog is manually enabled
- ✅ Periodic warnings logged to serial console when disconnected

## Technical Details

### Memory Usage
- **RAM:** 17.5% (45,804 bytes / 262,144 bytes)
- **Flash:** 24.2% (254,244 bytes / 1,048,576 bytes)

### New Features
1. **Network Watchdog System**
   - Timeout: 1 minute (configurable in code)
   - Monitors: WiFi connection status
   - Action: System reboot via NVIC_SystemReset()
   - Control: Web interface endpoints (`/watchdog?state=enable|disable`)

2. **Web Interface Endpoints**
   - `/watchdog?state=enable` - Enable watchdog
   - `/watchdog?state=disable` - Disable watchdog

### Default Settings
- Network watchdog: **ENABLED** by default
- Watchdog timeout: **60 seconds**
- WiFi check interval: 5 seconds
- MQTT publish interval: 30 seconds

## Installation

### Upload via PlatformIO
```bash
cd /home/dcasati/src/az3166-blink
source .venv/bin/activate
pio run --target upload
```

### Manual Flash (using binary)
```bash
# Using OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program releases/v1.0.1/az3166-firmware-v1.0.1.bin 0x08000000 verify reset exit"
```

## Configuration

### Enable/Disable Watchdog
- **Via Web Interface:** Navigate to `http://<device-ip>/control` and use the "Watchdog ON/OFF" buttons
- **Via Code:** Modify `watchdogEnabled` variable or `NETWORK_WATCHDOG_TIMEOUT` constant

### Change Watchdog Timeout
Edit `src/main.cpp` line ~288:
```cpp
const unsigned long NETWORK_WATCHDOG_TIMEOUT = 1 * 60 * 1000; // 1 minute
```

Change to desired timeout (in milliseconds), then rebuild and upload.

## Testing the Watchdog

1. **Verify it's running:**
   - Check serial console for: `Network watchdog initialized (1 minute timeout)`
   - Check web interface status shows: "Watchdog: ENABLED"

2. **Test automatic reboot:**
   - Disconnect network (unplug ethernet or disable WiFi)
   - After 60 seconds, device will display "WATCHDOG TIMEOUT" and reboot
   - Device will attempt to reconnect after reboot

3. **Disable for testing:**
   - Use web interface to click "Watchdog OFF"
   - Device will not auto-reboot without network

## Serial Console Output

### Normal Operation
```
Network watchdog initialized (1 minute timeout)
```

### When Disconnected (warnings every 60 seconds)
```
WARNING: No network activity for 60 seconds (timeout in 840 seconds)
```

### When Timeout Triggered
```
!!! NETWORK WATCHDOG TIMEOUT !!!
No network activity for 60 seconds
Threshold: 60 seconds
WATCHDOG TIMEOUT
Rebooting...
```

## Files in This Release

- `az3166-firmware-v1.0.1.bin` - Binary firmware (249 KB)
- `az3166-firmware-v1.0.1.elf` - ELF file with debug symbols (1.3 MB)
- `RELEASE_NOTES.md` - This file

## Known Issues

None

## Changelog

### v1.0.1 (2026-01-04)
- Added network watchdog with 1-minute timeout
- Added web interface controls for watchdog
- Improved reliability and automatic recovery
- Increased HTML buffer for control page

### v1.0.0 (Initial Release)
- Basic sensor station functionality
- WiFi connectivity
- MQTT publishing
- Web interface
- LED controls
- OLED display management
