# Network Watchdog Feature

## Overview
The AZ3166 MQTT watchdog monitors broker-acknowledged telemetry, not just WiFi
association. Firmware 1.1.1 retains MQTT reconnection/reboot behavior and adds
bounded OTA maintenance.

## Features

### Automatic Reboot
- **Delivery threshold**: 5 consecutive missed telemetry deliveries
- **Timeout fallback**: 5 minutes (300 seconds) without a successful delivery
- **Trigger**: Either threshold, even if WiFi remains connected
- **Action**: System reboot using STM32 NVIC_SystemReset()

### Monitoring
The watchdog monitors:
- MQTT QoS 1 publishing every 30 seconds
- A complete, valid PUBACK matching the outgoing packet ID
- Missed publish intervals while MQTT or WiFi is disconnected

Each failed or skipped delivery increments the counter once. Connection retries
(every 10 seconds) do not increment it or clear it. Only a successful PUBACK
clears the streak. Five missed intervals normally cause a reboot in about
2.5 minutes; blocking network calls may extend this.

These are missed MQTT deliveries, not raw IP/TCP packet drops. TCP packet loss
that recovers through retransmission and produces a PUBACK is not a failure.
Unacknowledged telemetry is not queued for replay after reconnect or reboot.

### Visual Indicators
When watchdog timeout is triggered:
1. Serial console displays warning messages
2. OLED display shows "WATCHDOG TIMEOUT" and "Rebooting..."
3. RGB LED lights red before reboot
4. System performs a reset after the display and LED delays (about 3 seconds)

### Warning Messages
While disconnected from network:
- Serial console logs warning every 60 seconds
- Shows time disconnected and time until reboot
- Example: `WARNING: No MQTT delivery acknowledged for 60 seconds (timeout in 240 seconds)`

## Configuration

### Variables
```cpp
// include/MqttDelivery.h
const uint32_t WATCHDOG_TIMEOUT_MS = 5 * 60 * 1000;
const uint8_t MAX_MISSED_DELIVERIES = 5;
// src/main.cpp
MqttDelivery::Watchdog mqttWatchdog;
bool watchdogEnabled = true;
```

### Default State
- **Enabled by default** on device startup
- Can be disabled/enabled via web interface

## Web Interface Control

### Endpoints

#### `POST /watchdog`, body `state=enable`
- Enables the network watchdog
- Resets the acknowledgement timer and missed-delivery counter

#### `POST /watchdog`, body `state=disable`
- Disables the network watchdog
- Device will not auto-reboot

Both require the operator's HTTP Basic credentials and the current
`X-CSRF-Token`. GET mutations and URL query parameters are rejected. The
authenticated control page supplies these automatically.

### OTA maintenance

An authenticated upload starts a three-minute maintenance window. Watchdog
reboots are deferred during reception, readback verification, staging and
activation, without changing the operator's enabled/disabled setting. Uploads
also have a ten-second data-inactivity deadline. Discard, failure or staged
expiry restores normal monitoring with a fresh acknowledgement grace period.
Unauthenticated requests cannot start maintenance. Sensors and MQTT continue
on the main thread; administrative mutations and serial password changes are
blocked during OTA. See [OTA.md](OTA.md).

### Control Page
The web control panel (`http://<device-ip>/control`) now includes:
- Watchdog status indicator (ENABLED/DISABLED)
- Buttons to enable/disable watchdog
- Status color coding (green=enabled, red=disabled)

## Serial Console Output

### Initialization
```
MQTT watchdog initialized (5 missed deliveries or 5 minutes without PUBACK)
```

### Periodic Warnings (when disconnected)
```
MQTT delivery missed; consecutive failures: 1/5
WARNING: No MQTT delivery acknowledged for 60 seconds (timeout in 240 seconds)
```

### Timeout Triggered
```
!!! NETWORK WATCHDOG TIMEOUT !!!
No MQTT delivery acknowledged for 150 seconds
Consecutive missed MQTT deliveries: 5
Threshold: 300 seconds

========================================
NETWORK WATCHDOG: Initiating system reboot...
========================================
```

## Implementation Details

### Timer Reset Points
The timer and counter are reset when:
1. Monitoring starts after setup
2. A matching PUBACK confirms a telemetry delivery
3. The watchdog is enabled via the web interface
4. OTA maintenance ends without installation

WiFi association, TCP connection and successful CONNACK do not reset them.

### Connection Recovery
- Failed writes, acknowledgement timeouts, malformed replies and closed sockets
  close the MQTT socket and clear its connected flag.
- Connection retries start with a fresh socket.
- Fragmented acknowledgements are accumulated with a 5-second deadline.
- The AZ3166 library's `available()` reports socket existence, not buffered byte
  count, so MQTT reads use the nonblocking socket directly.
- Broker names are resolved through the native IPv4 resolver on each connection
  attempt (subject to the resolver's DNS cache). There is no hardcoded broker IP.
- After initial or recovered WiFi connectivity, the LAN gateway becomes the
  primary DNS server; the previously configured primary is retained as backup.
  This allows private LAN records to resolve instead of querying public DNS.

### Rollover Protection
- Handles `millis()` rollover (occurs every ~49 days)
- Uses unsigned 32-bit subtraction across rollover; no artificial timer reset

### Safety Features
- Display and LED delays before reboot
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
OTA-capable firmware uses approximately 17.7% static RAM and 26.6% flash
(`az3166_app`, GCC 6.3.1). RTOS stacks and web-page allocations also use heap.

## Testing

### To Test Watchdog
1. Connect device to WiFi
2. Verify watchdog is enabled (web interface or serial console)
3. Make the broker unreachable for this device, or suppress PUBACK replies.
   WiFi can remain connected; avoid interrupting other broker clients.
4. Monitor serial console for warnings
5. At the fifth missed publish interval, the device will automatically reboot
6. Device will attempt to reconnect after reboot

### To Disable for Testing
1. Access web interface: `http://<device-ip>/control`
2. Click "Watchdog OFF" button
3. Device will stay running indefinitely without network

### Verified Behavior
Host regression tests cover fragmented/rejected replies, missing PUBACK,
connection closure, partial writes, packet bounds, failure-streak reset,
the five-miss threshold and timer rollover. See `test/README`.

Hardware validation observed an actual reboot at `consecutive failures: 5/5`,
then three consecutive broker-confirmed deliveries after restoring connectivity.
A targeted broker disconnect was also detected and followed by reconnection and
acknowledged telemetry, without rebooting. The saved flash configuration
survived the update.

Firmware 1.0.3 was verified on the board resolving the configured broker through
the LAN gateway, then receiving three consecutive PUBACKs. The previous primary
resolver was `1.1.1.1`, which could not resolve the private LAN record.
