# Feature Specification: MQTT Watchdog Connectivity Fix

**Feature Branch**: `001-mqtt-watchdog-fix`  
**Created**: 2026-01-17  
**Status**: Completed  
**Input**: User description: "Fix MQTT watchdog connectivity monitoring and automatic recovery after devices losing connection"

## User Scenarios & Testing

### User Story 1 - Automatic MQTT Recovery (Priority: P1)

A device deployed in a basement loses MQTT connectivity due to network instability. The device should automatically detect the failure, attempt reconnection, and if unsuccessful for 5 minutes, reboot to fully recover.

**Why this priority**: Critical reliability issue. Devices in the field must recover without manual intervention. Current implementation shows "MQTT failed!" on display but never recovers.

**Independent Test**: Deploy device, disconnect MQTT broker, verify device attempts reconnection every 10 seconds and reboots after 5 minutes if unsuccessful. After reboot, device should successfully reconnect.

**Acceptance Scenarios**:

1. **Given** device is publishing to MQTT successfully, **When** MQTT broker becomes unavailable, **Then** device detects connection loss within 30 seconds and displays "MQTT failed!"
2. **Given** MQTT connection is lost, **When** 10 seconds pass, **Then** device attempts to reconnect to MQTT broker
3. **Given** MQTT reconnection attempts fail for 5 minutes, **When** watchdog timeout is reached, **Then** device displays "WATCHDOG TIMEOUT" and reboots within 2 seconds
4. **Given** device has rebooted, **When** MQTT broker is available, **Then** device successfully reconnects and resumes publishing

---

### User Story 2 - Socket-Level Connection Verification (Priority: P1)

Device maintains WiFi connection but MQTT socket dies silently (broker restart, network routing issue, TCP timeout). The device should detect the dead socket before attempting to publish and force reconnection.

**Why this priority**: Silent socket failures cause data loss. Watchdog only checked WiFi status, not MQTT socket health, leading to devices appearing online but not publishing data.

**Independent Test**: Establish MQTT connection, kill broker process or reset network switch, verify device detects socket failure within 30 seconds (before next publish attempt) and reconnects.

**Acceptance Scenarios**:

1. **Given** MQTT connection is established, **When** TCP socket dies (broker restart, network failure), **Then** device detects connection loss before next publish attempt
2. **Given** dead MQTT socket detected, **When** device attempts to publish, **Then** device closes old connection and initiates new connection attempt
3. **Given** socket verification fails, **When** connection is closed, **Then** mqttConnected flag is set to false to trigger reconnection logic

---

### User Story 3 - Watchdog Timer Only Resets on Publish Success (Priority: P1)

Device has WiFi connectivity but MQTT publishing fails repeatedly. Watchdog should only reset its timer when data is successfully published to MQTT, not just when WiFi is connected.

**Why this priority**: Core architectural flaw. Previous implementation reset watchdog on WiFi connection alone, masking MQTT failures and preventing automatic recovery.

**Independent Test**: Connect device with WiFi but blocked MQTT port (firewall rule). Verify watchdog countdown continues despite WiFi connectivity and triggers reboot after 5 minutes.

**Acceptance Scenarios**:

1. **Given** device has WiFi connection but MQTT port is blocked, **When** publish attempt fails, **Then** watchdog timer continues counting (does not reset)
2. **Given** watchdog timer has been counting for 4 minutes, **When** MQTT publish succeeds, **Then** watchdog timer resets to 0
3. **Given** WiFi is connected but MQTT unavailable for 5 minutes, **When** watchdog timeout is reached, **Then** device reboots to recover

---

### User Story 4 - Proper Timeout Configuration (Priority: P2)

Watchdog timeout should match documented behavior of 5 minutes, not the 1 minute implemented in v1.0.1, to reduce false positives from temporary network instability.

**Why this priority**: Configuration mismatch causes premature reboots. 5-minute timeout allows time for temporary network issues to resolve while still ensuring recovery from persistent failures.

**Independent Test**: Disconnect MQTT for exactly 4 minutes, reconnect, verify device does not reboot. Disconnect for 5 minutes and 10 seconds, verify device reboots.

**Acceptance Scenarios**:

1. **Given** MQTT connection is lost, **When** 4 minutes and 30 seconds pass, **Then** device displays warning but does not reboot
2. **Given** MQTT connection is lost for 5 minutes, **When** timeout is reached, **Then** device triggers reboot sequence
3. **Given** MQTT is disconnected, **When** 60 seconds pass, **Then** serial console displays warning with time remaining

### Edge Cases

- **WiFi connected but DNS fails**: Device should retry hostname resolution on each connection attempt
- **Millis() rollover (49+ days uptime)**: Watchdog timer should handle rollover without false timeout
- **Rapid connect/disconnect cycles**: Device should not spam reconnection attempts (10-second minimum interval)
- **MQTT broker accepts connection but rejects publish**: Device should detect publish failure and force reconnection
- **Web interface accessed during watchdog countdown**: User should see time remaining until reboot in control panel
- **Watchdog disabled via web interface**: Timer should stop counting and not trigger reboot

## Requirements

### Functional Requirements

- **FR-001**: System MUST verify MQTT socket connection status using mqttWifiClient.connected() before each publish attempt
- **FR-002**: System MUST close dead MQTT connections and set mqttConnected = false when socket verification fails
- **FR-003**: System MUST close MQTT connection and set mqttConnected = false when publishMQTT() returns false
- **FR-004**: Watchdog timer MUST only reset when publishMQTT() returns true
- **FR-005**: Watchdog timer MUST NOT reset when WiFi connection is established alone
- **FR-006**: System MUST set NETWORK_WATCHDOG_TIMEOUT constant to 5 minutes (300,000 milliseconds)
- **FR-007**: System MUST display "MQTT failed!" on OLED when publish fails or connection is lost
- **FR-008**: System MUST display "MQTT disconnected!" on OLED when socket verification fails
- **FR-009**: System MUST attempt MQTT reconnection every 10 seconds when mqttConnected == false and WiFi is connected
- **FR-010**: System MUST reboot via NVIC_SystemReset() when watchdog timeout is exceeded
- **FR-011**: System MUST log all MQTT connection failures, socket verification failures, and reconnection attempts to serial console
- **FR-012**: System MUST handle millis() rollover by resetting watchdog timer when millis() < lastSuccessfulNetworkActivity

### Key Entities

- **Watchdog Timer**: Monotonic millisecond counter tracking time since last successful MQTT publish, triggers reboot at 5-minute threshold
- **MQTT Connection State**: Boolean flag (mqttConnected) indicating whether MQTT client has active, verified connection to broker
- **TCP Socket**: Underlying WiFiClient connection to MQTT broker, verified via connected() method before use

## Success Criteria

### Measurable Outcomes

- **SC-001**: Device automatically recovers from MQTT connection failures without manual intervention in 100% of test scenarios
- **SC-002**: Device detects dead MQTT sockets within one publish cycle (maximum 30 seconds)
- **SC-003**: Device reboots within 5 minutes and 10 seconds of last successful MQTT publish when connection cannot be restored
- **SC-004**: Watchdog timer does not trigger false positives when WiFi is stable and MQTT is functional for 24+ hour continuous operation
- **SC-005**: Device successfully reconnects to MQTT broker within 10 seconds when broker becomes available after failure
- **SC-006**: Serial console logs provide complete diagnostic trail of connection state changes for troubleshooting
- **SC-007**: OLED display accurately reflects current MQTT connection status with appropriate error messages

## Assumptions

- MQTT broker address and credentials are correctly configured in device flash memory
- WiFi network is available and device WiFi credentials are valid
- MQTT broker supports MQTT 3.1 protocol as currently implemented
- Device has valid network route to MQTT broker (no firewall blocking)
- Serial console is monitored for diagnostic output during field deployment or testing

## Dependencies

- STM32 HAL functions for system reset (NVIC_SystemReset())
- WiFiClient library for socket connection status verification
- Existing MQTT client implementation (raw protocol, no library)
- Existing watchdog infrastructure (checkNetworkWatchdog() function)
- OLED display library for status messages

## Out of Scope

- MQTT QoS level changes (remains at QoS 0)
- TLS/SSL support for MQTT connections
- MQTT broker authentication beyond basic connection
- Alternative recovery strategies (sleep mode, reduced polling frequency)
- MQTT keep-alive packet implementation (relies on publish frequency)
- Historical connection statistics or failure logging beyond current session
- Web interface for viewing watchdog countdown in real-time

## Implementation Notes

This specification documents the completed implementation in firmware v1.0.2. Changes were made to src/main.cpp implementing all functional requirements. Feature has been tested and deployed successfully.
