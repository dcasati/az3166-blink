# Network Watchdog Feature

## Overview
The Az3166 firmware now includes a network watchdog that monitors network connectivity and automatically reboots the device if it cannot communicate with the network for an extended period.

## Features

### Automatic Reboot
- **Timeout**: 15 minutes (900 seconds)
- **Trigger**: No WiFi connectivity for 15 minutes
- **Action**: System reboot using STM32 NVIC_SystemReset()

### Monitoring
The watchdog monitors:
- WiFi connection status (`WiFi.status() == WL_CONNECTED`)
- Successful MQTT message publishing

### Visual Indicators
When watchdog timeout is triggered:
1. Serial console displays warning messages
2. OLED display shows "WATCHDOG TIMEOUT" and "Rebooting..."
3. RGB LED flashes red before reboot
4. System performs clean reset after 2 seconds

### Warning Messages
While disconnected from network:
- Serial console logs warning every 60 seconds
- Shows time disconnected and time until reboot
- Example: `WARNING: No network activity for 300 seconds (timeout in 600 seconds)`

## Configuration

### Variables
```cpp
unsigned long lastSuccessfulNetworkActivity = 0;
const unsigned long NETWORK_WATCHDOG_TIMEOUT = 15 * 60 * 1000; // 15 minutes
bool watchdogEnabled = true;
```

### Default State
- **Enabled by default** on device startup
- Can be disabled/enabled via web interface

## Web Interface Control

### Endpoints

#### `/watchdog?state=enable`
- Enables the network watchdog
- Resets the activity timer

#### `/watchdog?state=disable`
- Disables the network watchdog
- Device will not auto-reboot

### Control Page
The web control panel (`http://<device-ip>/control`) now includes:
- Watchdog status indicator (ENABLED/DISABLED)
- Buttons to enable/disable watchdog
- Status color coding (green=enabled, red=disabled)

## Serial Console Output

### Initialization
```
Network watchdog initialized (15 minute timeout)
```

### Periodic Warnings (when disconnected)
```
WARNING: No network activity for 300 seconds (timeout in 600 seconds)
```

### Timeout Triggered
```
========================================
NETWORK WATCHDOG: Initiating system reboot...
========================================

!!! NETWORK WATCHDOG TIMEOUT !!!
No network activity for 900 seconds
Threshold: 900 seconds
```

## Implementation Details

### Timer Reset Points
The watchdog timer (`lastSuccessfulNetworkActivity`) is reset when:
1. Device has WiFi connection (checked every loop iteration)
2. MQTT message published successfully
3. Watchdog is enabled via web interface

### Rollover Protection
- Handles `millis()` rollover (occurs every ~49 days)
- Automatically resets timer on rollover detection

### Safety Features
- 2-second delay before reboot allows message display
- Serial output flushed before reset
- Clean STM32 system reset (not power cycle)

## Use Cases

### Automatic Recovery
- Network router restart/failure
- DHCP lease issues
- WiFi signal loss
- Network cable unplugged

### Debugging
- Disable watchdog when testing network issues
- Prevents unexpected reboots during development
- Can be re-enabled without firmware update

## Memory Usage
The watchdog adds minimal overhead:
- RAM: ~16 bytes (3 unsigned long variables + 1 bool)
- Flash: ~1.5KB (functions and string literals)

Total firmware after watchdog:
- RAM: 17.5% (45,804 bytes / 262,144 bytes)
- Flash: 24.2% (254,244 bytes / 1,048,576 bytes)

## Testing

### To Test Watchdog
1. Connect device to WiFi
2. Verify watchdog is enabled (web interface or serial console)
3. Disconnect network (unplug router or block device MAC address)
4. Monitor serial console for warnings
5. After 15 minutes, device will automatically reboot
6. Device will attempt to reconnect after reboot

### To Disable for Testing
1. Access web interface: `http://<device-ip>/control`
2. Click "Watchdog OFF" button
3. Device will stay running indefinitely without network

## Future Enhancements

Possible improvements:
- Configurable timeout value via web interface
- MQTT ping/keepalive as additional health check
- Email/SMS notification before reboot (if network recovers briefly)
- Watchdog event logging to flash memory
- Exponential backoff on repeated reboots
