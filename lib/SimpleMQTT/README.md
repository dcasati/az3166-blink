# SimpleMQTT Library

A lightweight MQTT client library for Arduino/AZ3166 with no external dependencies.

## Features

- Pure Arduino implementation (no PubSubClient dependency)
- MQTT 3.1 protocol support
- QoS 0 (fire and forget) publishing
- Built-in hostname to IP caching
- Simple connection management
- Binary and text payload support
- Error reporting

## Usage

### Basic Example

```cpp
#include "SimpleMQTT.h"
#include "AZ3166WiFi.h"

SimpleMQTT mqtt;

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi first
    WiFi.begin("YourWiFi", "YourPassword");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    // Configure MQTT
    mqtt.setServer("mqtt.example.com", 1883);
    mqtt.setClientId("MyDevice");
    
    // Optional: Add hostname mapping for faster connection
    mqtt.addHostnameMapping("mqtt.example.com", IPAddress(192, 168, 1, 100));
    
    // Connect to broker
    if (mqtt.connect()) {
        Serial.println("MQTT Connected!");
    } else {
        Serial.print("MQTT Connection failed: ");
        Serial.println(mqtt.getLastError());
    }
}

void loop() {
    if (mqtt.connected()) {
        // Publish a message
        mqtt.publish("sensors/temperature", "25.5");
        
        // Publish JSON
        const char* json = "{\"temp\":25.5,\"humidity\":60}";
        mqtt.publish("sensors/data", json);
    } else {
        // Reconnect
        mqtt.connect();
    }
    
    delay(5000);
}
```

### Binary Payload

```cpp
uint8_t binaryData[] = {0x01, 0x02, 0x03, 0x04};
mqtt.publish("sensors/binary", binaryData, sizeof(binaryData));
```

### DNS Hostname Caching

```cpp
// Pre-cache DNS resolution for faster connection
mqtt.addHostnameMapping("mqtt.server1.com", IPAddress(192, 168, 1, 10));
mqtt.addHostnameMapping("mqtt.server2.com", IPAddress(10, 0, 0, 5));
```

### Error Handling

```cpp
if (!mqtt.connect()) {
    Serial.print("Connection failed: ");
    Serial.println(mqtt.getLastError());
}

if (!mqtt.publish("test/topic", "message")) {
    Serial.print("Publish failed: ");
    Serial.println(mqtt.getLastError());
}
```

### Connection Management

```cpp
// Check connection status
if (mqtt.connected()) {
    mqtt.publish("sensors/status", "online");
} else {
    // Attempt reconnection
    if (mqtt.connect()) {
        Serial.println("Reconnected!");
    }
}

// Explicit disconnect
mqtt.disconnect();
```

## API Reference

### Constructor

```cpp
SimpleMQTT();
```

### Configuration Methods

- `void setServer(const char* server, int port = 1883)` - Set MQTT broker address
- `void setClientId(const char* clientId)` - Set MQTT client identifier
- `void addHostnameMapping(const char* hostname, IPAddress ip)` - Add DNS cache entry

### Connection Methods

- `bool connect()` - Connect to MQTT broker (returns true if successful)
- `bool connected()` - Check if connected to broker
- `void disconnect()` - Disconnect from broker

### Publishing Methods

- `bool publish(const char* topic, const char* payload)` - Publish text message
- `bool publish(const char* topic, const uint8_t* payload, size_t length)` - Publish binary data

### Utility Methods

- `const char* getLastError()` - Get last error message

## Protocol Details

### MQTT Version
- Implements MQTT 3.1 protocol
- Clean session enabled by default
- 60-second keepalive interval

### Quality of Service
- QoS 0 (at most once delivery)
- No message acknowledgment
- Suitable for sensor data and telemetry

### Packet Types Supported
- CONNECT - Client connection
- CONNACK - Connection acknowledgment
- PUBLISH - Message publishing (QoS 0 only)

## DNS Caching

The library includes a simple DNS cache to avoid repeated hostname resolution:

- Supports up to 5 cached entries
- Manual entry via `addHostnameMapping()`
- Automatic lookup on connection
- Reduces connection latency

## Limitations

- No SUBSCRIBE support (publish-only)
- No QoS 1/2 support
- No message persistence
- No automatic reconnection
- No TLS/SSL support
- No authentication (username/password)

## Perfect For

- Sensor data publishing
- Telemetry applications
- Status updates
- Fire-and-forget messaging
- Resource-constrained devices

## Future Enhancements

Potential additions for future versions:
- SUBSCRIBE support
- QoS 1/2 implementation
- Username/password authentication
- Last Will and Testament (LWT)
- Automatic reconnection with backoff
- TLS/SSL support

## License

MIT License
