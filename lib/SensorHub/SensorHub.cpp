#include "SensorHub.h"

SensorHub::SensorHub(PinName i2c_sda, PinName i2c_scl) 
    : sda_pin(i2c_sda), scl_pin(i2c_scl), initialized(false) {
    i2c = nullptr;
    ht_sensor = nullptr;
    pressure_sensor = nullptr;
    acc_gyro = nullptr;
    magnetometer = nullptr;
}

SensorHub::~SensorHub() {
    if (magnetometer) delete magnetometer;
    if (acc_gyro) delete acc_gyro;
    if (pressure_sensor) delete pressure_sensor;
    if (ht_sensor) delete ht_sensor;
    if (i2c) delete i2c;
}

bool SensorHub::begin() {
    Serial.println("[SensorHub] Initializing sensors...");
    
    // Initialize I2C
    i2c = new DevI2C(sda_pin, scl_pin);
    
    // Initialize Temperature & Humidity sensor
    ht_sensor = new HTS221Sensor(*i2c);
    if (ht_sensor->init(NULL) != 0) {
        Serial.println("[SensorHub] Failed to init HTS221");
        return false;
    }
    ht_sensor->enable();
    
    // Initialize Pressure sensor
    pressure_sensor = new LPS22HBSensor(*i2c);
    if (pressure_sensor->init(NULL) != 0) {
        Serial.println("[SensorHub] Failed to init LPS22HB");
        return false;
    }
    
    // Initialize Accelerometer & Gyroscope
    acc_gyro = new LSM6DSLSensor(*i2c, D4, D5);
    if (acc_gyro->init(NULL) != 0) {
        Serial.println("[SensorHub] Failed to init LSM6DSL");
        return false;
    }
    acc_gyro->enableAccelerator();
    acc_gyro->enableGyroscope();
    
    // Initialize Magnetometer
    magnetometer = new LIS2MDLSensor(*i2c);
    if (magnetometer->init(NULL) != 0) {
        Serial.println("[SensorHub] Failed to init LIS2MDL");
        return false;
    }
    
    initialized = true;
    Serial.println("[SensorHub] All sensors initialized successfully!");
    return true;
}

bool SensorHub::readAll(SensorData& data) {
    if (!initialized) return false;
    
    // Read Temperature & Humidity
    ht_sensor->getTemperature(&data.temperature);
    ht_sensor->getHumidity(&data.humidity);
    
    // Read Pressure
    pressure_sensor->getPressure(&data.pressure);
    
    // Read Accelerometer
    int axes[3];
    acc_gyro->getXAxes(axes);
    data.accel_x = axes[0] / 1000.0f;
    data.accel_y = axes[1] / 1000.0f;
    data.accel_z = axes[2] / 1000.0f;
    
    // Read Gyroscope
    int gyro_axes[3];
    acc_gyro->getGAxes(gyro_axes);
    data.gyro_x = gyro_axes[0] / 1000.0f;
    data.gyro_y = gyro_axes[1] / 1000.0f;
    data.gyro_z = gyro_axes[2] / 1000.0f;
    
    // Read Magnetometer
    int mag_axes[3];
    magnetometer->getMAxes(mag_axes);
    data.mag_x = mag_axes[0] / 1000.0f;
    data.mag_y = mag_axes[1] / 1000.0f;
    data.mag_z = mag_axes[2] / 1000.0f;
    
    return true;
}

bool SensorHub::readTempHumidity(float& temp, float& humidity) {
    if (!initialized) return false;
    ht_sensor->getTemperature(&temp);
    ht_sensor->getHumidity(&humidity);
    return true;
}

bool SensorHub::readPressure(float& pressure) {
    if (!initialized) return false;
    pressure_sensor->getPressure(&pressure);
    return true;
}

bool SensorHub::readAccelerometer(float& x, float& y, float& z) {
    if (!initialized) return false;
    int axes[3];
    acc_gyro->getXAxes(axes);
    x = axes[0] / 1000.0f;
    y = axes[1] / 1000.0f;
    z = axes[2] / 1000.0f;
    return true;
}

bool SensorHub::readGyroscope(float& x, float& y, float& z) {
    if (!initialized) return false;
    int gyro_axes[3];
    acc_gyro->getGAxes(gyro_axes);
    x = gyro_axes[0] / 1000.0f;
    y = gyro_axes[1] / 1000.0f;
    z = gyro_axes[2] / 1000.0f;
    return true;
}

bool SensorHub::readMagnetometer(float& x, float& y, float& z) {
    if (!initialized) return false;
    int mag_axes[3];
    magnetometer->getMAxes(mag_axes);
    x = mag_axes[0] / 1000.0f;
    y = mag_axes[1] / 1000.0f;
    z = mag_axes[2] / 1000.0f;
    return true;
}

void SensorHub::printAll() {
    SensorData data;
    if (!readAll(data)) {
        Serial.println("[SensorHub] Failed to read sensors");
        return;
    }
    
    Serial.println("[SensorHub] === Sensor Readings ===");
    Serial.print("Temperature: "); Serial.print(data.temperature); Serial.println(" °C");
    Serial.print("Humidity: "); Serial.print(data.humidity); Serial.println(" %");
    Serial.print("Pressure: "); Serial.print(data.pressure); Serial.println(" mbar");
    
    Serial.print("Accel: X="); Serial.print(data.accel_x, 3);
    Serial.print("g Y="); Serial.print(data.accel_y, 3);
    Serial.print("g Z="); Serial.print(data.accel_z, 3); Serial.println("g");
    
    Serial.print("Gyro: X="); Serial.print(data.gyro_x, 2);
    Serial.print("dps Y="); Serial.print(data.gyro_y, 2);
    Serial.print("dps Z="); Serial.print(data.gyro_z, 2); Serial.println("dps");
    
    Serial.print("Mag: X="); Serial.print(data.mag_x, 3);
    Serial.print("G Y="); Serial.print(data.mag_y, 3);
    Serial.print("G Z="); Serial.print(data.mag_z, 3); Serial.println("G");
}

bool SensorHub::toJSON(char* buffer, size_t bufferSize, 
                       const char* deviceId, const char* model, const char* location) {
    SensorData data;
    if (!readAll(data)) return false;
    
    int written = snprintf(buffer, bufferSize,
        "{\"device\":\"%s\",\"model\":\"%s\",\"location\":\"%s\","
        "\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f,"
        "\"accel\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},"
        "\"gyro\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f},"
        "\"mag\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f}}",
        deviceId, model, location,
        data.temperature, data.humidity, data.pressure,
        data.accel_x, data.accel_y, data.accel_z,
        data.gyro_x, data.gyro_y, data.gyro_z,
        data.mag_x, data.mag_y, data.mag_z
    );
    
    return written > 0 && written < (int)bufferSize;
}
