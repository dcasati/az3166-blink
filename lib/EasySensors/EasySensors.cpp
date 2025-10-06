#include "EasySensors.h"
#include <math.h>

// Static member initialization
SensorHub EasySensors::sensors;
bool EasySensors::initialized = false;

void EasySensors::init() {
    if (!initialized) {
        sensors.begin();
        initialized = true;
    }
}

float EasySensors::getTemperature() {
    init();
    float temp, humidity;
    sensors.readTempHumidity(temp, humidity);
    return temp;
}

float EasySensors::getHumidity() {
    init();
    float temp, humidity;
    sensors.readTempHumidity(temp, humidity);
    return humidity;
}

float EasySensors::getPressure() {
    init();
    float pressure;
    sensors.readPressure(pressure);
    return pressure;
}

bool EasySensors::isMoving() {
    init();
    float x, y, z;
    sensors.readAccelerometer(x, y, z);
    
    // Check if total acceleration is significantly different from 1g (gravity)
    float totalAccel = sqrt(x*x + y*y + z*z);
    return (totalAccel < 0.8 || totalAccel > 1.2);
}

bool EasySensors::isTilted() {
    init();
    float x, y, z;
    sensors.readAccelerometer(x, y, z);
    
    // Check if Z axis is significantly different from 1g (not flat)
    return (abs(z) < 0.8);
}

float EasySensors::getTiltAngle() {
    init();
    float x, y, z;
    sensors.readAccelerometer(x, y, z);
    
    // Calculate tilt angle from vertical
    float angle = acos(abs(z)) * 180.0 / PI;
    return angle;
}

void EasySensors::showOnDisplay() {
    init();
    SensorHub::SensorData data;
    sensors.readAll(data);
    DisplayHelper::showSensorData(data.temperature, data.humidity, data.pressure);
}

void EasySensors::printAll() {
    init();
    sensors.printAll();
}

void EasySensors::toJSON(char* buffer, size_t size, const char* deviceName) {
    init();
    sensors.toJSON(buffer, size, deviceName, "az3166", "Home");
}
