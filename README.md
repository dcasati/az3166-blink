# AZ3166 Sensor Station

A pure STM32-based sensor monitoring solution for the Azure IoT DevKit (MXChip AZ3166) that completely bypasses Azure services and provides direct MQTT connectivity with sensor data collection.

## Features

- **Pure STM32 Implementation**: Bypasses Azure framework for lightweight operation
- **Multi-Sensor Support**: Temperature, humidity, pressure, accelerometer, gyroscope, magnetometer
- **Persistent Configuration**: Flash memory storage for device settings
- **MQTT Publishing**: Direct MQTT publishing of sensor data in JSON format
- **Serial Configuration**: Interactive serial interface for device configuration
- **WiFi Connectivity**: Direct WiFi connection without Azure dependencies
- **OLED Display**: Real-time sensor display on the device

## Hardware Requirements

- Azure IoT DevKit (MXChip AZ3166)
- WiFi network access
- MQTT broker (e.g., Mosquitto)

## Configuration

The device supports both default configuration and interactive serial configuration:

### Default Configuration
- Device ID: `SensorStation_01`
- MQTT Topic: `sensors/az3166`
- MQTT Port: `1883`

### Interactive Configuration
1. Connect to the device via serial monitor (115200 baud)
2. Press 'C' within 5 seconds of startup to enter configuration mode
3. Configure: Device ID, WiFi credentials, MQTT server settings
4. Configuration is automatically saved to flash memory

## Data Format

The device publishes JSON sensor data to the configured MQTT topic:

```json
{
  "device": "SensorStation_01",
  "timestamp": 12345678,
  "temperature": 25.5,
  "humidity": 45.2,
  "pressure": 1013.25,
  "accel": {"x": 0.023, "y": -0.981, "z": 0.156},
  "gyro": {"x": 1.2, "y": -0.8, "z": 0.3},
  "mag": {"x": 0.234, "y": -0.123, "z": 0.678}
}
```

## Building and Uploading

### Method 1: PlatformIO (Recommended)

```bash
# Build the project
platformio run

# Upload to device
platformio run --target upload

# Monitor serial output
platformio device monitor
```

### Method 2: Alternative Flashing Methods

If you don't have PlatformIO installed, you can use pre-built binaries:

**STM32CubeProgrammer (Recommended):**
1. Download from ST Microelectronics website (free registration required)
2. Put AZ3166 in DFU mode (hold A+B, release B, release A)
3. Flash `firmware.bin` to address `0x08000000`

**st-flash (Command Line):**
```bash
# Install via Homebrew
brew install stlink

# Flash the binary
st-flash write firmware.bin 0x8000000
```

⚠️ **Note**: Drag-and-drop to the AZ3166 volume is NOT reliable and can brick the device.

See `FLASHING_GUIDE.md` in releases for complete instructions.

## Build Configuration

The project includes extensive build flags to disable Azure services:
- Disables Azure IoT Hub telemetry
- Disables OTA updates
- Disables time synchronization
- Removes Azure HTTP services
- Excludes Azure framework system files

## License

MIT License - see LICENSE file for details.
