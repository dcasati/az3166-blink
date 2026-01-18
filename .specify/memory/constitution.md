<!--
SYNC IMPACT REPORT:
- Version: Initial → 1.0.0
- Constitution created for Az3166 Sensor Station
- New principles: 6 core principles established
- Added sections: Resource Constraints, Development Workflow
- Templates: ✅ All templates consistent with constitution
- Follow-up: None - initial version complete
-->

# AZ3166 Sensor Station Constitution

## Core Principles

### I. Reliability First (NON-NEGOTIABLE)
The firmware MUST maintain reliable operation in production environments. This includes:
- Network watchdog monitoring with automatic recovery (5-minute timeout)
- Automatic reconnection for WiFi and MQTT failures
- Configuration persistence with checksum validation
- Protection against edge cases (millis() rollover, socket failures)
- Watchdog timer MUST only reset on successful MQTT publish, not WiFi alone

**Rationale**: IoT devices operate unattended. Silent failures are unacceptable. Every network failure must trigger automatic recovery or system reboot.

### II. Resource Constraints (NON-NEGOTIABLE)
The firmware MUST operate within strict hardware limits:
- Maximum RAM usage: 25% (65,536 / 262,144 bytes)
- Maximum Flash usage: 50% (524,288 / 1,048,576 bytes)
- All features must be justified against resource impact
- Optimization required before adding new capabilities

**Rationale**: STM32F412ZGT6 has limited resources. Exceeding limits causes instability, crashes, or prevents future enhancements.

### III. Azure-Free Architecture (NON-NEGOTIABLE)
The firmware MUST NOT depend on Azure IoT framework or services:
- Pure STM32 + Arduino implementation only
- Direct WiFi and MQTT connections (no Azure IoT Hub)
- All Azure services disabled via build flags
- No background Azure threads or telemetry

**Rationale**: Azure framework adds unnecessary complexity, resource overhead, and vendor lock-in. Direct protocols provide transparency and control.

### IV. Observable Operations
All critical operations MUST be observable via serial output:
- Connection status (WiFi, MQTT)
- Sensor readings with timestamps
- Configuration changes
- Error conditions and recovery actions
- Watchdog warnings before timeout

**Rationale**: Serial debugging is the primary diagnostic tool for embedded devices. Operations must be traceable without external tools.

### V. Configuration as Data
Device configuration MUST be stored separately from code:
- STM32 flash memory persistence (Sector 10)
- Checksum validation on read
- Serial interface for field configuration
- No hardcoded credentials or broker addresses in source

**Rationale**: Same firmware image must work across multiple devices with different configurations. Field updates must not require recompilation.

### VI. Semantic Versioning
Firmware versions MUST follow semantic versioning (MAJOR.MINOR.PATCH):
- MAJOR: Breaking changes to MQTT payload, configuration format, or API
- MINOR: New features (sensors, endpoints, capabilities)
- PATCH: Bug fixes, reliability improvements, optimizations

Version MUST be displayed in web interface and serial output.

**Rationale**: Version tracking enables debugging, rollback decisions, and clear communication about compatibility.

## Resource Constraints

### Memory Allocation
- RAM usage tracked in CHANGELOG.md for each release
- Flash usage tracked in CHANGELOG.md for each release
- Pre-commit validation: Build must succeed with current usage reported
- Any feature exceeding 20% resource usage requires architectural review

### Performance Targets
- Sensor reading cycle: 30 seconds (MUST NOT exceed)
- MQTT publishing cycle: 30 seconds (aligned with sensor readings)
- Web server response time: < 2 seconds for all endpoints
- Serial monitor baud rate: 115200 (standard for debugging)

## Development Workflow

### Code Changes
All code changes MUST include:
1. Clear problem statement or feature description
2. Serial debug output for new operations
3. Memory impact assessment (RAM/Flash delta)
4. Testing on physical hardware (no simulation-only changes)
5. CHANGELOG.md update with version bump

### Firmware Versioning
- Version number in `src/main.cpp` line 14: `#define FIRMWARE_VERSION`
- CHANGELOG.md must document version and changes
- Version must increment before release
- Git tag format: `vX.Y.Z` matching firmware version

### Hardware Testing Requirements
- WiFi connection and reconnection scenarios
- MQTT publish success and failure paths
- Watchdog timeout verification (allow 5+ minutes of network failure)
- Configuration persistence across reboots
- Web interface functionality on mobile and desktop
- Long-term stability test (24+ hours continuous operation)

### Documentation Standards
- README.md for user-facing setup and usage
- WATCHDOG.md for network recovery behavior
- HOMEASSISTANT.md for integration patterns
- AGENTS.md for AI-assisted build commands
- CHANGELOG.md for version history (Keep a Changelog format)

## Governance

This constitution supersedes all other development practices and guides all firmware decisions.

### Amendment Process
1. Proposed changes must document rationale and impact
2. Version bump required (MINOR for new principles, PATCH for clarifications)
3. Sync Impact Report generated listing affected templates/docs
4. Updated constitution committed with descriptive message

### Compliance Verification
- All changes must align with Core Principles (especially NON-NEGOTIABLE items)
- Resource constraints checked on every build
- Watchdog behavior validated after any network code changes
- Configuration persistence verified after flash storage changes

### Conflict Resolution
When principles conflict, priority order:
1. Reliability First (system must recover from failures)
2. Resource Constraints (must fit in hardware limits)
3. Azure-Free Architecture (no Azure dependencies)
4. Observable Operations (debugging capability)
5. Configuration as Data (flexibility)
6. Semantic Versioning (tracking)

**Version**: 1.0.0 | **Ratified**: 2026-01-17 | **Last Amended**: 2026-01-17
