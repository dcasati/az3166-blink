#ifndef SENSOR_HUB_H
#define SENSOR_HUB_H

#include <Arduino.h>
#include "AZ3166WiFi.h"
#include "Sensor.h"

/**
 * SensorHub - Unified sensor management for AZ3166
 * 
 * This library provides a simple interface to initialize and read
 * all sensors on the AZ3166 board:
 * - HTS221: Temperature and Humidity
 * - LPS22HB: Pressure
 * - LSM6DSL: Accelerometer and Gyroscope
 * - LIS2MDL: Magnetometer
 */
class SensorHub {
public:
    struct SensorData {
        // Temperature and Humidity
        float temperature;  // °C
        float humidity;     // %
        
        // Pressure
        float pressure;     // mbar
        
        // Accelerometer (g)
        float accel_x;
        float accel_y;
        float accel_z;
        
        // Gyroscope (dps - degrees per second)
        float gyro_x;
        float gyro_y;
        float gyro_z;
        
        // Magnetometer (gauss)
        float mag_x;
        float mag_y;
        float mag_z;
    };

    /**
     * Constructor
     * @param i2c_sda - I2C SDA pin (default: D14)
     * @param i2c_scl - I2C SCL pin (default: D15)
     */
    SensorHub(PinName i2c_sda = D14, PinName i2c_scl = D15);

    /**
     * Destructor - cleanup sensor objects
     */
    ~SensorHub();

    /**
     * Initialize all sensors
     * @return true if all sensors initialized successfully
     */
    bool begin();

    /**
     * Read all sensor values
     * @param data - Reference to SensorData struct to fill
     * @return true if read successful
     */
    bool readAll(SensorData& data);

    /**
     * Read only temperature and humidity
     */
    bool readTempHumidity(float& temp, float& humidity);

    /**
     * Read only pressure
     */
    bool readPressure(float& pressure);

    /**
     * Read only accelerometer
     */
    bool readAccelerometer(float& x, float& y, float& z);

    /**
     * Read only gyroscope
     */
    bool readGyroscope(float& x, float& y, float& z);

    /**
     * Read only magnetometer
     */
    bool readMagnetometer(float& x, float& y, float& z);

    /**
     * Print all sensor values to Serial
     */
    void printAll();

    /**
     * Get JSON string with all sensor data
     * @param buffer - Buffer to store JSON string
     * @param bufferSize - Size of buffer
     * @param deviceId - Device identifier
     * @param model - Device model
     * @param location - Device location
     * @return true if JSON created successfully
     */
    bool toJSON(char* buffer, size_t bufferSize, 
                const char* deviceId = "unknown",
                const char* model = "az3166",
                const char* location = "unknown");

private:
    DevI2C* i2c;
    HTS221Sensor* ht_sensor;
    LPS22HBSensor* pressure_sensor;
    LSM6DSLSensor* acc_gyro;
    LIS2MDLSensor* magnetometer;
    
    PinName sda_pin;
    PinName scl_pin;
    bool initialized;
};

#endif // SENSOR_HUB_H
