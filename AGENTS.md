# Agent Instructions for Az3166 Project

## Uploading Code to the Board

To compile and upload the code to the Az3166 board, use the following command:

```bash
cd /home/dcasati/src/az3166-blink && source .venv/bin/activate && pio run --target upload
```

### Prerequisites
- Ensure the Az3166 board is connected via USB (ST-Link should be detected with `lsusb`)
- The virtual environment (.venv) contains the working PlatformIO installation
- The board should appear as: `Bus 001 Device 003: ID 0483:374b STMicroelectronics ST-LINK/V2.1`

### Build Information
- Platform: ST STM32 (ststm32)
- Board: mxchip_az3166 
- Framework: Arduino
- Upload Protocol: stlink
- Monitor Speed: 115200 baud

### Memory Usage (as of last build)
- RAM: 17.0% used (44,488 bytes of 262,144 bytes available)
- Flash: 32.1% used (336,076 bytes of 1,048,576 bytes available)

### Project Features
This Az3166 sensor station includes:
- WiFi connectivity management
- MQTT communication
- Environmental sensor readings (temperature, humidity, pressure)
- Motion detection
- OLED display output
- RGB LED status indication
- Flash-based configuration storage
- Web configuration interface

### Monitoring
After upload, monitor serial output at 115200 baud to view device status and debug information.