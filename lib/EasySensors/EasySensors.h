#ifndef EASY_SENSORS_H
#define EASY_SENSORS_H

#include <Arduino.h>
#include "SensorHub.h"
#include "DisplayHelper.h"

/**
 * EasySensors - Super simple sensor reading for kids!
 * 
 * Read temperature, humidity, and other sensors with easy functions.
 */
class EasySensors {
public:
    /**
     * Get the temperature in Celsius
     */
    static float getTemperature();

    /**
     * Get the humidity percentage
     */
    static float getHumidity();

    /**
     * Get the air pressure
     */
    static float getPressure();

    /**
     * Is the board moving? (accelerometer)
     */
    static bool isMoving();

    /**
     * Is the board tilted? (accelerometer)
     */
    static bool isTilted();

    /**
     * Get tilt angle in degrees (0-90)
     */
    static float getTiltAngle();

    /**
     * Show all sensor values on the display
     */
    static void showOnDisplay();

    /**
     * Print all sensor values to Serial monitor
     */
    static void printAll();

    /**
     * Get JSON string with all sensor data
     * @param buffer - Where to store the JSON
     * @param size - Size of buffer
     * @param deviceName - Your device name
     */
    static void toJSON(char* buffer, size_t size, const char* deviceName = "MyDevice");

private:
    static SensorHub sensors;
    static bool initialized;
    static void init();
};

#endif // EASY_SENSORS_H
