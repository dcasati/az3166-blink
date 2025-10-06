#ifndef AZ3166_BOARD_H
#define AZ3166_BOARD_H

#include <Arduino.h>
#include "FlashConfig.h"
#include "SensorHub.h"
#include "SimpleMQTT.h"
#include "DisplayHelper.h"
#include "AZ3166WiFi.h"

/**
 * AZ3166Board - Super simple all-in-one library for kids!
 * 
 * This library makes the AZ3166 board as easy to use as an Arduino.
 * Just call simple functions and everything works!
 */
class AZ3166Board {
public:
    /**
     * Start the board - call this first in setup()
     */
    static void begin();

    /**
     * Connect to WiFi
     * @param ssid - Your WiFi name
     * @param password - Your WiFi password
     * @return true if connected
     */
    static bool connectWiFi(const char* ssid, const char* password);

    /**
     * Connect to MQTT server
     * @param server - Server address
     * @param deviceName - Name for your device
     * @return true if connected
     */
    static bool connectMQTT(const char* server, const char* deviceName);

    /**
     * Check if WiFi is connected
     */
    static bool isWiFiConnected();

    /**
     * Check if MQTT is connected
     */
    static bool isMQTTConnected();

    /**
     * Get WiFi IP address as a string
     */
    static String getIPAddress();

    /**
     * Save settings to memory (survives power off!)
     */
    static void saveSettings();

    /**
     * Load settings from memory
     */
    static void loadSettings();

private:
    static FlashConfig config;
    static SensorHub sensors;
    static SimpleMQTT mqtt;
    static bool initialized;
    static bool wifiConnected;
    static bool mqttConnected;
};

#endif // AZ3166_BOARD_H
