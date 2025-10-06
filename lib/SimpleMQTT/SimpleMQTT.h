#ifndef SIMPLE_MQTT_H
#define SIMPLE_MQTT_H

#include <Arduino.h>
#include "AZ3166WiFi.h"

/**
 * SimpleMQTT - Lightweight MQTT client for Arduino/AZ3166
 * 
 * This library provides basic MQTT functionality without external dependencies.
 * Implements MQTT 3.1 protocol with QoS 0 (fire and forget).
 * 
 * Features:
 * - Simple hostname to IP resolution (with caching)
 * - MQTT CONNECT with client ID
 * - MQTT PUBLISH with QoS 0
 * - Connection state management
 */
class SimpleMQTT {
public:
    /**
     * Constructor
     */
    SimpleMQTT();

    /**
     * Destructor
     */
    ~SimpleMQTT();

    /**
     * Set MQTT server by hostname
     * @param server - Server hostname or IP address
     * @param port - MQTT port (default: 1883)
     */
    void setServer(const char* server, int port = 1883);

    /**
     * Set MQTT client ID
     * @param clientId - Client identifier string
     */
    void setClientId(const char* clientId);

    /**
     * Add a hostname to IP mapping for DNS resolution
     * @param hostname - Hostname to map
     * @param ip - IP address
     */
    void addHostnameMapping(const char* hostname, IPAddress ip);

    /**
     * Connect to MQTT broker
     * @return true if connected successfully
     */
    bool connect();

    /**
     * Check if connected to broker
     * @return true if connected
     */
    bool connected();

    /**
     * Disconnect from broker
     */
    void disconnect();

    /**
     * Publish a message to a topic (QoS 0)
     * @param topic - Topic name
     * @param payload - Message payload
     * @return true if published successfully
     */
    bool publish(const char* topic, const char* payload);

    /**
     * Publish a message with binary payload (QoS 0)
     * @param topic - Topic name
     * @param payload - Binary payload
     * @param length - Payload length
     * @return true if published successfully
     */
    bool publish(const char* topic, const uint8_t* payload, size_t length);

    /**
     * Get last error message
     * @return Error message string
     */
    const char* getLastError();

private:
    WiFiClient client;
    IPAddress serverIP;
    int serverPort;
    char clientId[32];
    char serverHostname[64];
    bool ipResolved;
    char lastError[128];
    
    // DNS cache (simple key-value store)
    struct DnsEntry {
        char hostname[64];
        IPAddress ip;
        bool valid;
    };
    static const int MAX_DNS_CACHE = 5;
    DnsEntry dnsCache[MAX_DNS_CACHE];
    int dnsCacheSize;

    bool resolveHostname(const char* hostname, IPAddress& ip);
    bool sendConnectPacket();
    bool waitForConnack();
    void setError(const char* error);
};

#endif // SIMPLE_MQTT_H
