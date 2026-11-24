# AZ3166 Sensor Station

A pure STM32-based sensor monitoring solution for the Azure IoT DevKit (MXChip AZ3166) that provides direct MQTT connectivity with sensor data collection.

## Features

- **Pure STM32 Implementation**: Bypasses Azure framework for lightweight operation
- **Multi-Sensor Support**: Temperature, humidity, pressure, accelerometer, gyroscope, magnetometer
- **Persistent Configuration**: Flash memory storage for device settings
- **MQTT Publishing**: QoS 1 JSON telemetry with matching broker acknowledgements
- **Automatic Recovery**: Reconnect after failed deliveries; reboot after 5 consecutive missed deliveries or 5 minutes without a PUBACK (see [WATCHDOG.md](WATCHDOG.md))
- **Serial Configuration**: Interactive serial interface for device configuration
- **WiFi Connectivity**: Direct WiFi connection without Azure dependencies
- **OLED Display**: Real-time sensor display on the device
- **Web Control Panel**: Mobile-responsive web UI for device management

## Hardware Requirements

- Azure IoT DevKit (MXChip AZ3166)
- WiFi network access
- MQTT broker (e.g., Mosquitto)

## Serial Communication

The device uses USB serial communication for configuration and monitoring:

**Connection Parameters:**
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None

**Connecting:**
1. Connect the AZ3166 to your computer via USB
2. The device appears as a virtual COM port
3. Use any serial terminal application:
   - **macOS/Linux**: `screen /dev/tty.usbmodem* 115200` or `minicom`
   - **Windows**: PuTTY, Tera Term, or built-in Windows Terminal
   - **PlatformIO**: `platformio device monitor` (automatically detects settings)

**Serial Commands:**
- Press **'C'** within 5 seconds of device startup to enter configuration mode
- Configuration mode allows setting WiFi credentials and MQTT parameters
- Normal operation displays sensor readings and connection status

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

### DNS

The MQTT server can be a hostname or an IPv4 address; no broker IP is hardcoded.
After each WiFi connection, firmware uses the LAN gateway as the primary DNS
server and retains the previously configured resolver as backup. The gateway
must provide DNS for private LAN hostnames; public resolvers cannot answer
private records. Resolution failures are logged and retried under the MQTT
recovery policy.

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
3. Flash the application `firmware.bin` to address `0x0800C000`, preserving the bootloader at `0x08000000`

**st-flash (Command Line):**
```bash
# Install via Homebrew
brew install stlink

# Flash the binary
st-flash write firmware.bin 0x0800C000
```

⚠️ **Note**: Drag-and-drop to the AZ3166 volume is NOT reliable and can brick the device.

See `FLASHING_GUIDE.md` in releases for complete instructions.

The framework links the application at `0x0800C000`. Do not write a standalone
application binary at `0x08000000`: that overwrites the bootloader. Configuration
is stored separately in sector 10 at `0x080C0000`.

The board uses the legacy GCC 6.3.1 toolchain. Its macOS executable is x86_64;
on Apple Silicon without translation, build in a local Linux x86_64 container
using the same PlatformIO configuration, then upload the binary with a native
ST-Link tool at the application address above.

## Build Configuration

The project includes extensive build flags to disable Azure services:
- Disables Azure IoT Hub telemetry
- Disables OTA updates
- Disables time synchronization
- Removes Azure HTTP services
- Excludes Azure framework system files

## License

MIT License - see LICENSE file for details.
