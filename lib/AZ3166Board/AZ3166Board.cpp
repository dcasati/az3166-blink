#include "AZ3166Board.h"

// Static member initialization
FlashConfig AZ3166Board::config;
SensorHub AZ3166Board::sensors;
SimpleMQTT AZ3166Board::mqtt;
bool AZ3166Board::initialized = false;
bool AZ3166Board::wifiConnected = false;
bool AZ3166Board::mqttConnected = false;

void AZ3166Board::begin() {
    if (!initialized) {
        Serial.begin(115200);
        delay(1000);
        
        DisplayHelper::begin();
        DisplayHelper::print(0, "AZ3166 Ready!");
        
        sensors.begin();
        
        initialized = true;
        Serial.println("[AZ3166Board] Board initialized!");
    }
}

bool AZ3166Board::connectWiFi(const char* ssid, const char* password) {
    Serial.print("[AZ3166Board] Connecting to WiFi: ");
    Serial.println(ssid);
    
    DisplayHelper::showWiFiConnecting();
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    
    if (wifiConnected) {
        Serial.println("\n[AZ3166Board] WiFi connected!");
        Serial.print("[AZ3166Board] IP: ");
        Serial.println(WiFi.localIP());
        DisplayHelper::showWiFiConnected(WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[AZ3166Board] WiFi failed!");
        DisplayHelper::showError("WiFi failed!");
    }
    
    return wifiConnected;
}

bool AZ3166Board::connectMQTT(const char* server, const char* deviceName) {
    Serial.print("[AZ3166Board] Connecting to MQTT: ");
    Serial.println(server);
    
    DisplayHelper::showMQTTConnecting();
    
    mqtt.setServer(server, 1883);
    mqtt.setClientId(deviceName);
    
    mqttConnected = mqtt.connect();
    
    if (mqttConnected) {
        Serial.println("[AZ3166Board] MQTT connected!");
        DisplayHelper::showMQTTConnected();
    } else {
        Serial.println("[AZ3166Board] MQTT failed!");
        DisplayHelper::showError("MQTT failed!");
    }
    
    return mqttConnected;
}

bool AZ3166Board::isWiFiConnected() {
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    return wifiConnected;
}

bool AZ3166Board::isMQTTConnected() {
    mqttConnected = mqtt.connected();
    return mqttConnected;
}

String AZ3166Board::getIPAddress() {
    return WiFi.localIP().toString();
}

void AZ3166Board::saveSettings() {
    if (config.save()) {
        Serial.println("[AZ3166Board] Settings saved!");
    } else {
        Serial.println("[AZ3166Board] Failed to save settings!");
    }
}

void AZ3166Board::loadSettings() {
    if (config.load()) {
        Serial.println("[AZ3166Board] Settings loaded!");
    } else {
        Serial.println("[AZ3166Board] No settings found!");
    }
}
