#include "SimpleMQTT.h"

SimpleMQTT::SimpleMQTT() : serverPort(1883), ipResolved(false), dnsCacheSize(0) {
    memset(clientId, 0, sizeof(clientId));
    memset(serverHostname, 0, sizeof(serverHostname));
    memset(lastError, 0, sizeof(lastError));
    strcpy(clientId, "arduino_client");
    
    // Initialize DNS cache
    for (int i = 0; i < MAX_DNS_CACHE; i++) {
        dnsCache[i].valid = false;
    }
}

SimpleMQTT::~SimpleMQTT() {
    disconnect();
}

void SimpleMQTT::setServer(const char* server, int port) {
    strncpy(serverHostname, server, sizeof(serverHostname) - 1);
    serverPort = port;
    ipResolved = false;
}

void SimpleMQTT::setClientId(const char* id) {
    strncpy(clientId, id, sizeof(clientId) - 1);
}

void SimpleMQTT::addHostnameMapping(const char* hostname, IPAddress ip) {
    if (dnsCacheSize >= MAX_DNS_CACHE) {
        Serial.println("[SimpleMQTT] DNS cache full");
        return;
    }
    
    strncpy(dnsCache[dnsCacheSize].hostname, hostname, 
            sizeof(dnsCache[dnsCacheSize].hostname) - 1);
    dnsCache[dnsCacheSize].ip = ip;
    dnsCache[dnsCacheSize].valid = true;
    dnsCacheSize++;
    
    Serial.print("[SimpleMQTT] Added DNS mapping: ");
    Serial.print(hostname);
    Serial.print(" -> ");
    Serial.println(ip);
}

bool SimpleMQTT::resolveHostname(const char* hostname, IPAddress& ip) {
    // Check cache first
    for (int i = 0; i < dnsCacheSize; i++) {
        if (dnsCache[i].valid && strcmp(dnsCache[i].hostname, hostname) == 0) {
            ip = dnsCache[i].ip;
            Serial.print("[SimpleMQTT] Using cached IP for ");
            Serial.print(hostname);
            Serial.print(": ");
            Serial.println(ip);
            return true;
        }
    }
    
    Serial.print("[SimpleMQTT] No cached IP for ");
    Serial.println(hostname);
    return false;
}

bool SimpleMQTT::connect() {
    // Resolve hostname if not already done
    if (!ipResolved) {
        Serial.print("[SimpleMQTT] Resolving hostname: ");
        Serial.println(serverHostname);
        
        if (!resolveHostname(serverHostname, serverIP)) {
            setError("Failed to resolve hostname");
            return false;
        }
        ipResolved = true;
    }
    
    // Test basic connectivity
    Serial.print("[SimpleMQTT] Connecting to ");
    Serial.print(serverIP);
    Serial.print(":");
    Serial.println(serverPort);
    
    if (!client.connect(serverIP, serverPort)) {
        setError("TCP connection failed");
        return false;
    }
    
    Serial.println("[SimpleMQTT] TCP connected, sending MQTT CONNECT...");
    
    if (!sendConnectPacket()) {
        client.stop();
        setError("Failed to send CONNECT packet");
        return false;
    }
    
    if (!waitForConnack()) {
        client.stop();
        setError("Failed to receive CONNACK");
        return false;
    }
    
    Serial.println("[SimpleMQTT] MQTT connected successfully!");
    return true;
}

bool SimpleMQTT::sendConnectPacket() {
    uint8_t packet[128];
    int pos = 0;
    
    // Fixed header
    packet[pos++] = 0x10;  // CONNECT packet type
    int lenPos = pos++;    // Length placeholder
    
    // Variable header - Protocol name
    packet[pos++] = 0x00; packet[pos++] = 0x06; // Protocol name length
    packet[pos++] = 'M'; packet[pos++] = 'Q'; 
    packet[pos++] = 'I'; packet[pos++] = 's'; 
    packet[pos++] = 'd'; packet[pos++] = 'p';
    
    packet[pos++] = 0x03;  // Protocol version (MQTT 3.1)
    packet[pos++] = 0x02;  // Connect flags (clean session)
    packet[pos++] = 0x00; packet[pos++] = 0x3C;  // Keep alive (60 seconds)
    
    // Payload - Client ID
    int idLen = strlen(clientId);
    packet[pos++] = 0x00; packet[pos++] = idLen;
    memcpy(&packet[pos], clientId, idLen);
    pos += idLen;
    
    // Set remaining length
    packet[lenPos] = pos - 2;
    
    // Send packet
    client.write(packet, pos);
    return true;
}

bool SimpleMQTT::waitForConnack() {
    unsigned long timeout = millis() + 5000;
    
    // Wait for response
    while (millis() < timeout && client.available() == 0) {
        delay(10);
    }
    
    if (client.available() >= 4) {
        uint8_t response[4];
        client.read(response, 4);
        
        if (response[0] == 0x20 && response[3] == 0x00) {
            return true;
        } else {
            Serial.print("[SimpleMQTT] CONNACK failed, return code: ");
            Serial.println(response[3]);
            return false;
        }
    } else if (client.available() >= 1) {
        uint8_t partialResponse[4] = {0};
        int bytesRead = client.read(partialResponse, client.available());
        
        // If we got 0x20, that's CONNACK - treat as success
        if (partialResponse[0] == 0x20) {
            return true;
        }
    }
    
    return false;
}

bool SimpleMQTT::connected() {
    return client.connected();
}

void SimpleMQTT::disconnect() {
    if (client.connected()) {
        client.stop();
    }
}

bool SimpleMQTT::publish(const char* topic, const char* payload) {
    return publish(topic, (const uint8_t*)payload, strlen(payload));
}

bool SimpleMQTT::publish(const char* topic, const uint8_t* payload, size_t length) {
    if (!client.connected()) {
        setError("Not connected to broker");
        return false;
    }
    
    int topicLen = strlen(topic);
    int remainingLength = 2 + topicLen + length;
    
    // Build PUBLISH packet
    uint8_t packet[1024];
    int pos = 0;
    
    // Fixed header
    packet[pos++] = 0x30;  // PUBLISH (QoS 0, no retain, no dup)
    
    // Remaining length (variable length encoding)
    if (remainingLength < 128) {
        packet[pos++] = remainingLength;
    } else if (remainingLength < 16384) {
        packet[pos++] = (remainingLength % 128) | 0x80;
        packet[pos++] = remainingLength / 128;
    } else {
        setError("Payload too large");
        return false;
    }
    
    // Topic length
    packet[pos++] = (topicLen >> 8) & 0xFF;
    packet[pos++] = topicLen & 0xFF;
    
    // Topic
    memcpy(&packet[pos], topic, topicLen);
    pos += topicLen;
    
    // Payload
    memcpy(&packet[pos], payload, length);
    pos += length;
    
    // Send packet
    client.write(packet, pos);
    
    Serial.print("[SimpleMQTT] Published to ");
    Serial.print(topic);
    Serial.print(" (");
    Serial.print(length);
    Serial.println(" bytes)");
    
    return true;
}

const char* SimpleMQTT::getLastError() {
    return lastError;
}

void SimpleMQTT::setError(const char* error) {
    strncpy(lastError, error, sizeof(lastError) - 1);
    Serial.print("[SimpleMQTT] Error: ");
    Serial.println(lastError);
}
