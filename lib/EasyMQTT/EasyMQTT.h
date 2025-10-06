#ifndef EASY_MQTT_H
#define EASY_MQTT_H

#include <Arduino.h>
#include "SimpleMQTT.h"
#include "EasySensors.h"

/**
 * EasyMQTT - Super simple MQTT messaging for kids!
 * 
 * Send messages to the internet with easy functions.
 */
class EasyMQTT {
public:
    /**
     * Connect to an MQTT server
     * @param server - Server address (like "mqtt.example.com")
     * @param deviceName - Your device name
     * @return true if connected
     */
    static bool connect(const char* server, const char* deviceName);

    /**
     * Check if connected to server
     */
    static bool isConnected();

    /**
     * Send a simple text message
     * @param topic - Channel name (like "temperature" or "messages")
     * @param message - Your message
     * @return true if sent successfully
     */
    static bool sendMessage(const char* topic, const char* message);

    /**
     * Send a number
     * @param topic - Channel name
     * @param number - Number to send
     * @return true if sent successfully
     */
    static bool sendNumber(const char* topic, float number);

    /**
     * Send all sensor data automatically
     * @param topic - Channel name (like "sensors")
     * @param deviceName - Your device name
     * @return true if sent successfully
     */
    static bool sendSensorData(const char* topic, const char* deviceName = "MyDevice");

    /**
     * Disconnect from server
     */
    static void disconnect();

private:
    static SimpleMQTT mqtt;
    static bool connected;
};

#endif // EASY_MQTT_H
