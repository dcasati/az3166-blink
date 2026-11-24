#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace MqttDelivery {

const uint32_t ACK_TIMEOUT_MS = 5000;
const uint32_t WATCHDOG_TIMEOUT_MS = 5 * 60 * 1000;
const uint8_t MAX_MISSED_DELIVERIES = 5;

enum class Result {
  Acknowledged,
  InvalidPacket,
  WriteFailed,
  ReadFailed,
  ConnectionClosed,
  Timeout,
  InvalidAcknowledgement
};

struct Watchdog {
  uint32_t lastAcknowledged = 0;
  uint8_t missedDeliveries = 0;

  void reset(uint32_t now) {
    lastAcknowledged = now;
    missedDeliveries = 0;
  }

  void missed() {
    if (missedDeliveries < MAX_MISSED_DELIVERIES) {
      ++missedDeliveries;
    }
  }

  bool expired(uint32_t now) const {
    return missedDeliveries >= MAX_MISSED_DELIVERIES ||
           uint32_t(now - lastAcknowledged) >= WATCHDOG_TIMEOUT_MS;
  }
};

// AZ3166 available() reports socket existence, not buffered bytes. Read directly
// and accumulate fragmented replies, retrying only the nonblocking error.
template <typename Client, typename Clock, typename Sleep>
Result readAcknowledgement(Client& client, const uint8_t expected[4],
                           int wouldBlock, Clock clock, Sleep sleep) {
  uint8_t response[4];
  size_t received = 0;
  const uint32_t started = clock();
  while (uint32_t(clock() - started) < ACK_TIMEOUT_MS) {
    int count = client.read(response + received, sizeof(response) - received);
    if (count > 0) {
      received += count;
      if (received == sizeof(response)) {
        return memcmp(response, expected, sizeof(response)) == 0
                   ? Result::Acknowledged : Result::InvalidAcknowledgement;
      }
    } else if (count == 0) {
      return Result::ConnectionClosed;
    } else if (count != wouldBlock) {
      return Result::ReadFailed;
    }
    sleep(10);
  }
  return Result::Timeout;
}

template <typename Client, typename Clock, typename Sleep>
Result connect(Client& client, const char* deviceId, int wouldBlock,
               Clock clock, Sleep sleep) {
  const size_t idLength = strlen(deviceId);
  if (idLength == 0 || idLength > 31) {
    return Result::InvalidPacket;
  }
  uint8_t packet[45] = {
    0x10, uint8_t(12 + idLength),
    0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x02, 0x00, 0x3C,
    0x00, uint8_t(idLength)
  };
  memcpy(packet + 14, deviceId, idLength);
  const size_t length = 14 + idLength;
  if (client.write(packet, length) != length) {
    return Result::WriteFailed;
  }
  const uint8_t connack[4] = {0x20, 0x02, 0x00, 0x00};
  return readAcknowledgement(client, connack, wouldBlock, clock, sleep);
}

template <typename Client, typename Clock, typename Sleep>
Result publish(Client& client, const char* topic, const char* payload,
               uint16_t packetId, int wouldBlock, Clock clock, Sleep sleep) {
  const size_t topicLength = strlen(topic);
  const size_t payloadLength = strlen(payload);
  uint8_t packet[1024];
  if (topicLength == 0 || topicLength > 65535 || packetId == 0 ||
      topicLength > sizeof(packet) - 7 ||
      payloadLength > sizeof(packet) - 7 - topicLength) {
    return Result::InvalidPacket;
  }
  size_t remaining = 4 + topicLength + payloadLength;
  size_t pos = 0;
  packet[pos++] = 0x32; // QoS 1: success requires a matching PUBACK.
  do {
    uint8_t byte = remaining % 128;
    remaining /= 128;
    packet[pos++] = byte | (remaining ? 0x80 : 0x00);
  } while (remaining);
  packet[pos++] = uint8_t(topicLength >> 8);
  packet[pos++] = uint8_t(topicLength);
  memcpy(packet + pos, topic, topicLength);
  pos += topicLength;
  packet[pos++] = uint8_t(packetId >> 8);
  packet[pos++] = uint8_t(packetId);
  memcpy(packet + pos, payload, payloadLength);
  pos += payloadLength;
  if (client.write(packet, pos) != pos) {
    return Result::WriteFailed;
  }
  const uint8_t puback[4] = {0x40, 0x02, uint8_t(packetId >> 8),
                           uint8_t(packetId)};
  return readAcknowledgement(client, puback, wouldBlock, clock, sleep);
}

} // namespace MqttDelivery
