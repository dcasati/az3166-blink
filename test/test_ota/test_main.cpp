#include "DurableRecord.h"
#include "HttpRequest.h"
#include "ConfigStorage.h"
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct FakeFlash {
  uint8_t bytes[8192];
  int writesUntilFailure = -1;
  bool readFailed = false;
  FakeFlash() { memset(bytes, 0xFF, sizeof(bytes)); }
  bool read(uint32_t offset, void* output, uint32_t size) {
    if (readFailed || offset + size > sizeof(bytes)) return false;
    memcpy(output, bytes + offset, size); return true;
  }
  bool erase(uint32_t offset, uint32_t size) {
    assert(offset + size <= sizeof(bytes));
    memset(bytes + offset, 0xFF, size); return true;
  }
  bool write(uint32_t offset, const void* input, uint32_t size) {
    if (writesUntilFailure == 0) return false;
    if (writesUntilFailure > 0) --writesUntilFailure;
    assert(offset + size <= sizeof(bytes));
    const uint8_t* source = static_cast<const uint8_t*>(input);
    for (unsigned i = 0; i < size; ++i) bytes[offset + i] &= source[i];
    return true;
  }
};

struct Data { uint32_t value; };

void testRecords() {
  FakeFlash flash;
  DurableRecord::Store<Data, FakeFlash> store(flash);
  Data data = {7}, readback = {};
  assert(store.load(readback) == DurableRecord::Load::Empty);
  assert(store.save(data));
  assert(store.load(readback) == DurableRecord::Load::Loaded && readback.value == 7);
  data.value = 9;
  flash.writesUntilFailure = 1; // Data written, commit marker interrupted.
  assert(!store.save(data));
  DurableRecord::Store<Data, FakeFlash> rebooted(flash);
  assert(rebooted.load(readback) == DurableRecord::Load::Loaded && readback.value == 7);
  flash.writesUntilFailure = -1;
  assert(rebooted.save(data));
  assert(rebooted.load(readback) == DurableRecord::Load::Loaded && readback.value == 9);
  flash.bytes[4096 + offsetof(DurableRecord::Record<Data>, payload)] ^= 1;
  assert(rebooted.load(readback) == DurableRecord::Load::Loaded && readback.value == 7);
  flash.bytes[0] = 0;
  assert(rebooted.load(readback) == DurableRecord::Load::Failed);
  flash.readFailed = true;
  assert(rebooted.load(readback) == DurableRecord::Load::Failed);

  FakeFlash rollover;
  DurableRecord::Record<Data> r = {};
  r.magic = DurableRecord::MAGIC; r.version = 1; r.length = sizeof(Data);
  r.generation = 0xFFFFFFFF; r.payload.value = 11;
  r.crc = DurableRecord::crc32(&r, offsetof(DurableRecord::Record<Data>, crc));
  r.commit = DurableRecord::COMMITTED;
  memcpy(rollover.bytes, &r, sizeof(r));
  DurableRecord::Store<Data, FakeFlash> wrap(rollover);
  assert(wrap.load(readback) == DurableRecord::Load::Loaded);
  assert(wrap.save(Data{12}));
  assert(wrap.load(readback) == DurableRecord::Load::Loaded && readback.value == 12);
  assert(wrap.load(readback, [](const Data& d) { return d.value == 11; }) ==
         DurableRecord::Load::Loaded && readback.value == 11);
}

struct FakeSocket {
  std::string input;
  size_t offset = 0;
  bool stall = false;
  bool alternating = false;
  int read(uint8_t* output, size_t count) {
    if (stall || (alternating = !alternating)) return -3001;
    if (offset >= input.size()) return 0;
    count = std::min(count, input.size() - offset);
    memcpy(output, input.data() + offset, count);
    offset += count; return int(count);
  }
};

bool parse(const std::string& input, Http::Request& request) {
  FakeSocket socket; socket.input = input;
  uint32_t now = 0;
  return Http::readRequest(socket, request, -3001, [&]() { return now; },
                           [&](unsigned ms) { now += ms; });
}

void testHttp() {
  Http::Request r;
  assert(parse("POST /ota/upload HTTP/1.1\r\ncontent-length: 256\r\n"
               "x-csrf-token: abc\r\n\r\nBODY", r));
  assert(r.length == 256 && !strcmp(r.csrf, "abc"));
  const char* bad[] = {
    "POST /ota/upload HTTP/1.1\r\nContent-Length: -1\r\n\r\n",
    "POST /ota/upload HTTP/1.1\r\nContent-Length: 4294967296\r\n\r\n",
    "POST /ota/upload HTTP/1.1\r\nContent-Length: 1\r\ncontent-length: 1\r\n\r\n",
    "POST /ota/upload HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n",
    "POST /ota/upload HTTP/1.1\r\nExpect: 100-continue\r\n\r\n",
    "POST /ota/upload HTTP/1.1\r\nAuthorization: a\r\nAuthorization: b\r\n\r\n",
    "GET / HTTP/1.1\n\n",
    "GET / HTTP/1.1\r\n header: value\r\n\r\n",
    "GET / HTTP/1.1\r\nContent-Length: \r\n\r\n"
    ,"GET / HTTP/1.1\r\n: empty-name\r\n\r\n"
    ,"GET / HTTP/1.1\r\nAuthorization: \r\nAuthorization: Basic other\r\n\r\n"
    ,"GET / HTTP/1.1\r\nX-CSRF-Token: \r\nX-CSRF-Token: abc\r\n\r\n"
  };
  for (const char* value : bad) {
    Http::Request request;
    assert(!parse(value, request));
  }
  Http::Request oversized;
  assert(!parse("GET / HTTP/1.1\r\nLong: " + std::string(700, 'a') + "\r\n\r\n", oversized));
  FakeSocket stalled; stalled.stall = true;
  uint32_t now = 0xFFFFFF00;
  Http::Request timeout;
  assert(!Http::readRequest(stalled, timeout, -3001, [&]() { return now; },
                            [&](unsigned ms) { now += ms; }));
}

void testImages() {
  uint8_t image[16] = {};
  uint32_t stack = 0x20040000, reset = 0x0800C009;
  memcpy(image, &stack, 4); memcpy(image + 4, &reset, 4);
  assert(Http::validImageVectors(image, sizeof(image)));
  assert(!Http::validImageVectors(image, 0));
  assert(!Http::validImageVectors(image, 0xF4001));
  reset = 0x08000035; memcpy(image + 4, &reset, 4);
  assert(!Http::validImageVectors(image, sizeof(image)));
  reset = 0x0800C008; memcpy(image + 4, &reset, 4);
  assert(!Http::validImageVectors(image, sizeof(image)));
  memcpy(image, "\x7f""ELF", 4);
  assert(!Http::validImageVectors(image, sizeof(image)));
  uint8_t hash[32];
  assert(Http::fromHex(std::string(64, 'a').c_str(), hash, sizeof(hash)));
  assert(!Http::fromHex(std::string(64, 'g').c_str(), hash, sizeof(hash)));
  assert(!Http::fromHex("abcd", hash, sizeof(hash)));
}

void testConfiguration() {
  DeviceConfig config = {};
  memcpy(config.magic, "AZ31", 4);
  strcpy(config.deviceId, "station");
  strcpy(config.mqttServer, "broker.local");
  strcpy(config.mqttTopic, "sensors");
  config.mqttPort = 1883;
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&config);
  for (size_t i = 0; i < offsetof(DeviceConfig, checksum); ++i) config.checksum ^= bytes[i];
  assert(validLegacyConfig(config));
  FakeFlash flash;
  DurableRecord::Store<DeviceConfig, FakeFlash> migrated(flash);
  assert(migrated.save(config));
  DeviceConfig loaded;
  assert(migrated.load(loaded, validDeviceConfig) == DurableRecord::Load::Loaded);
  assert(memcmp(&config, &loaded, sizeof(config)) == 0);
  config.checksum ^= 1;
  assert(!validLegacyConfig(config));
  config.mqttPort = 65536;
  assert(!validDeviceConfig(config));
  config.mqttPort = 0;
  assert(!validDeviceConfig(config));
  config.mqttPort = 1883;
  memset(config.password, 'p', sizeof(config.password));
  assert(!validDeviceConfig(config));
}

int main() {
  testRecords(); testHttp(); testImages(); testConfiguration();
  std::cout << "OTA storage, HTTP and image validation tests passed\n";
}
