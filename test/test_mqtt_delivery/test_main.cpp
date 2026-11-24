#include "MqttDelivery.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <limits>
#include <vector>

using MqttDelivery::Result;

const int WOULD_BLOCK = -3001;

struct FakeClient {
  std::vector<uint8_t> reply;
  std::vector<uint8_t> written;
  size_t offset = 0;
  size_t fragmentSize = 4;
  int emptyResult = WOULD_BLOCK;
  bool shortWrite = false;
  int initialWouldBlocks = 0;

  size_t write(const uint8_t* data, size_t size) {
    written.assign(data, data + size);
    return shortWrite ? size - 1 : size;
  }

  int read(uint8_t* data, size_t size) {
    if (initialWouldBlocks > 0) {
      --initialWouldBlocks;
      return WOULD_BLOCK;
    }
    if (offset == reply.size()) {
      return emptyResult;
    }
    size_t count = std::min(size, std::min(fragmentSize, reply.size() - offset));
    memcpy(data, reply.data() + offset, count);
    offset += count;
    return int(count);
  }
};

void testConnect() {
  uint32_t now = 0;
  auto clock = [&]() { return now; };
  auto sleep = [&](uint32_t ms) { now += ms; };
  FakeClient client;
  client.reply = {0x20, 0x02, 0x00, 0x00};
  client.fragmentSize = 1;
  client.initialWouldBlocks = 3;
  assert(MqttDelivery::connect(client, "station", WOULD_BLOCK, clock, sleep) ==
         Result::Acknowledged);
  assert(client.written == std::vector<uint8_t>({
      0x10, 19, 0, 4, 'M', 'Q', 'T', 'T', 4, 2, 0, 60,
      0, 7, 's', 't', 'a', 't', 'i', 'o', 'n'}));

  client = FakeClient();
  client.reply = {0x20};
  assert(MqttDelivery::connect(client, "station", WOULD_BLOCK, clock, sleep) ==
         Result::Timeout);

  client = FakeClient();
  client.reply = {0x20, 0x02, 0x00, 0x05};
  assert(MqttDelivery::connect(client, "station", WOULD_BLOCK, clock, sleep) ==
         Result::InvalidAcknowledgement);

  client = FakeClient();
  client.shortWrite = true;
  assert(MqttDelivery::connect(client, "station", WOULD_BLOCK, clock, sleep) ==
         Result::WriteFailed);
}

void testPublish() {
  uint32_t now = 0;
  auto clock = [&]() { return now; };
  auto sleep = [&](uint32_t ms) { now += ms; };
  FakeClient client;
  client.reply = {0x40, 0x02, 0x12, 0x34};
  client.fragmentSize = 1;
  assert(MqttDelivery::publish(client, "sensors", "{}", 0x1234,
                              WOULD_BLOCK, clock, sleep) == Result::Acknowledged);
  assert(client.written == std::vector<uint8_t>({
      0x32, 13, 0, 7, 's', 'e', 'n', 's', 'o', 'r', 's',
      0x12, 0x34, '{', '}'}));

  client = FakeClient();
  client.reply = {0x40, 0x02, 0x12, 0x35};
  assert(MqttDelivery::publish(client, "sensors", "{}", 0x1234,
                              WOULD_BLOCK, clock, sleep) ==
         Result::InvalidAcknowledgement);

  client = FakeClient();
  assert(MqttDelivery::publish(client, "sensors", "{}", 1,
                              WOULD_BLOCK, clock, sleep) == Result::Timeout);
  client.emptyResult = 0;
  assert(MqttDelivery::publish(client, "sensors", "{}", 1,
                              WOULD_BLOCK, clock, sleep) == Result::ConnectionClosed);
  client.emptyResult = -3004;
  assert(MqttDelivery::publish(client, "sensors", "{}", 1,
                              WOULD_BLOCK, clock, sleep) == Result::ReadFailed);
  client.shortWrite = true;
  assert(MqttDelivery::publish(client, "sensors", "{}", 1,
                              WOULD_BLOCK, clock, sleep) == Result::WriteFailed);

  client = FakeClient();
  client.reply = {0x40, 2, 0, 1};
  std::vector<char> payload(1014, 'x');
  payload.back() = '\0';
  assert(MqttDelivery::publish(client, "test", payload.data(), 1,
                              WOULD_BLOCK, clock, sleep) == Result::Acknowledged);
  assert(client.written.size() == 1024);
  assert(client.written[1] == 0xFD && client.written[2] == 0x07);
  payload.insert(payload.begin(), 'x');
  client.written.clear();
  assert(MqttDelivery::publish(client, "test", payload.data(), 1,
                              WOULD_BLOCK, clock, sleep) == Result::InvalidPacket);
  assert(client.written.empty());

  now = std::numeric_limits<uint32_t>::max() - 100;
  client = FakeClient();
  assert(MqttDelivery::publish(client, "sensors", "{}", 1,
                              WOULD_BLOCK, clock, sleep) == Result::Timeout);
  assert(now < MqttDelivery::ACK_TIMEOUT_MS);
}

void testRecovery() {
  MqttDelivery::Watchdog watchdog;
  watchdog.reset(1000);
  for (uint32_t slot = 1; slot < 5; ++slot) {
    watchdog.missed();
    assert(!watchdog.expired(1000 + slot * 30000));
  }
  watchdog.missed();
  assert(watchdog.expired(151000));
  watchdog.reset(151000);
  assert(!watchdog.expired(151001));

  // TCP/CONNACK successes do not reset this timer or failure counter.
  watchdog.missed();
  watchdog.missed();
  assert(watchdog.expired(451000));
  watchdog.reset(451000); // A PUBACK breaks the consecutive-failure streak.
  for (int i = 0; i < 4; ++i) {
    watchdog.missed();
  }
  assert(!watchdog.expired(571000));
  watchdog.missed();
  assert(watchdog.expired(601000));

  watchdog.reset(std::numeric_limits<uint32_t>::max() - 1000);
  assert(!watchdog.expired(1000));
  assert(watchdog.expired(299000));
}

int main() {
  testConnect();
  testPublish();
  testRecovery();
  std::cout << "MQTT protocol and recovery tests passed\n";
}
