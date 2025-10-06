# SensorHub Library

A unified sensor management library for the AZ3166 IoT DevKit, providing easy access to all onboard sensors.

## Supported Sensors

- **HTS221** - Temperature and Humidity sensor
- **LPS22HB** - Barometric Pressure sensor
- **LSM6DSL** - 6-axis Accelerometer and Gyroscope
- **LIS2MDL** - 3-axis Magnetometer

## Features

- Single initialization for all sensors
- Unified data structure for all readings
- Individual or combined sensor reading
- Built-in JSON serialization
- Serial debug printing
- Configurable I2C pins

## Usage

### Basic Example

```cpp
#include "SensorHub.h"

SensorHub sensors;

void setup() {
    Serial.begin(115200);
    
    // Initialize all sensors
    if (!sensors.begin()) {
        Serial.println("Sensor initialization failed!");
        while(1);
    }
    
    Serial.println("Sensors ready!");
}

void loop() {
    // Read all sensors
    SensorHub::SensorData data;
    if (sensors.readAll(data)) {
        Serial.print("Temperature: ");
        Serial.print(data.temperature);
        Serial.println(" °C");
        
        Serial.print("Humidity: ");
        Serial.print(data.humidity);
        Serial.println(" %");
        
        Serial.print("Pressure: ");
        Serial.print(data.pressure);
        Serial.println(" mbar");
        
        Serial.print("Accelerometer: X=");
        Serial.print(data.accel_x);
        Serial.print("g Y=");
        Serial.print(data.accel_y);
        Serial.print("g Z=");
        Serial.print(data.accel_z);
        Serial.println("g");
    }
    
    delay(1000);
}
```

### Quick Debug Print

```cpp
// Print all sensor values to Serial
sensors.printAll();
```

### Individual Sensor Reading

```cpp
float temp, humidity;
if (sensors.readTempHumidity(temp, humidity)) {
    Serial.print("Temp: ");
    Serial.print(temp);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
}

float pressure;
sensors.readPressure(pressure);

float ax, ay, az;
sensors.readAccelerometer(ax, ay, az);

float gx, gy, gz;
sensors.readGyroscope(gx, gy, gz);

float mx, my, mz;
sensors.readMagnetometer(mx, my, mz);
```

### JSON Output

```cpp
char jsonBuffer[512];
if (sensors.toJSON(jsonBuffer, sizeof(jsonBuffer), 
                   "MyDevice", "az3166", "Office")) {
    Serial.println(jsonBuffer);
    // Output: {"device":"MyDevice","model":"az3166","location":"Office",...}
}
```

### Custom I2C Pins

```cpp
// Use custom I2C pins
SensorHub sensors(D14, D15);  // SDA, SCL
```

## API Reference

### Constructor

```cpp
SensorHub(PinName i2c_sda = D14, PinName i2c_scl = D15);
```

### Methods

- `bool begin()` - Initialize all sensors (returns true if successful)
- `bool readAll(SensorData& data)` - Read all sensor values
- `bool readTempHumidity(float& temp, float& humidity)` - Read temperature and humidity
- `bool readPressure(float& pressure)` - Read pressure
- `bool readAccelerometer(float& x, float& y, float& z)` - Read accelerometer
- `bool readGyroscope(float& x, float& y, float& z)` - Read gyroscope
- `bool readMagnetometer(float& x, float& y, float& z)` - Read magnetometer
- `void printAll()` - Print all sensor values to Serial
- `bool toJSON(char* buffer, size_t bufferSize, const char* deviceId, const char* model, const char* location)` - Generate JSON string

### SensorData Structure

```cpp
struct SensorData {
    float temperature;  // °C
    float humidity;     // %
    float pressure;     // mbar
    float accel_x;      // g
    float accel_y;      // g
    float accel_z;      // g
    float gyro_x;       // dps (degrees per second)
    float gyro_y;       // dps
    float gyro_z;       // dps
    float mag_x;        // gauss
    float mag_y;        // gauss
    float mag_z;        // gauss
};
```

## Sensor Specifications

### HTS221 (Temperature & Humidity)
- Temperature range: -40 to +120°C
- Humidity range: 0 to 100%
- Accuracy: ±0.5°C, ±3.5% RH

### LPS22HB (Pressure)
- Pressure range: 260 to 1260 hPa
- Accuracy: ±0.1 hPa

### LSM6DSL (Accelerometer & Gyroscope)
- Accelerometer: ±2/±4/±8/±16 g
- Gyroscope: ±125/±245/±500/±1000/±2000 dps

### LIS2MDL (Magnetometer)
- Range: ±50 gauss
- Resolution: 1.5 mGauss

## License

MIT License
