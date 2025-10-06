#include "EasyMQTT.h"

// Static member initialization
SimpleMQTT EasyMQTT::mqtt;
bool EasyMQTT::connected = false;

bool EasyMQTT::connect(const char* server, const char* deviceName) {
    Serial.print("[EasyMQTT] Connecting to: ");
    Serial.println(server);
    
    mqtt.setServer(server, 1883);
    mqtt.setClientId(deviceName);
    connected = mqtt.connect();
    
    if (connected) {
        Serial.println("[EasyMQTT] Connected!");
    } else {
        Serial.println("[EasyMQTT] Connection failed!");
    }
    
    return connected;
}

bool EasyMQTT::isConnected() {
    connected = mqtt.connected();
    return connected;
}

bool EasyMQTT::sendMessage(const char* topic, const char* message) {
    if (!connected) {
        Serial.println("[EasyMQTT] Not connected! Call connect() first.");
        return false;
    }
    
    Serial.print("[EasyMQTT] Sending to '");
    Serial.print(topic);
    Serial.print("': ");
    Serial.println(message);
    
    return mqtt.publish(topic, message);
}

bool EasyMQTT::sendNumber(const char* topic, float number) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f", number);
    return sendMessage(topic, buffer);
}

bool EasyMQTT::sendSensorData(const char* topic, const char* deviceName) {
    if (!connected) {
        Serial.println("[EasyMQTT] Not connected! Call connect() first.");
        return false;
    }
    
    char jsonBuffer[512];
    EasySensors::toJSON(jsonBuffer, sizeof(jsonBuffer), deviceName);
    
    Serial.print("[EasyMQTT] Sending sensor data to '");
    Serial.print(topic);
    Serial.println("'");
    
    return mqtt.publish(topic, jsonBuffer);
}

void EasyMQTT::disconnect() {
    mqtt.disconnect();
    connected = false;
    Serial.println("[EasyMQTT] Disconnected");
}
