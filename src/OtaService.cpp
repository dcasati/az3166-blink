#include <Arduino.h>
#include "OtaService.h"
#include "ConfigStorage.h"
#include "OtaPage.h"
#include "mico.h"
#include "CheckSumUtils.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha256.h"
#include "rtos.h"

extern DeviceConfig config;
void resetMqttWatchdog();

namespace {
const uint32_t MAX_IMAGE = 0xF4000;
const uint32_t SESSION_TIMEOUT = 180000;
rtos::Mutex stateMutex;
enum class State { Idle, Receiving, Staged, Applying };
State state = State::Idle;
uint32_t sessionStarted = 0;
bool activationReady = false;
char csrf[65] = {};
uint32_t stagedLength = 0;
uint8_t stagedHash[32] = {};
uint16_t stagedCRC = 0;
uint32_t failedLoginAt = 0;
bool loginBlocked = false;
char serialPassword[65] = {};
unsigned serialUsed = 0;
bool passwordEntry = false;
bool passwordRejected = false;
uint32_t passwordStarted = 0;

bool writeAll(WiFiClient& client, const char* data, size_t size) {
  uint32_t started = millis();
  while (size && uint32_t(millis() - started) < 5000) {
    int count = int(client.write(reinterpret_cast<const uint8_t*>(data), size));
    if (count == NSAPI_ERROR_WOULD_BLOCK) { Thread::wait(1); continue; }
    if (count <= 0 || size_t(count) > size) return false;
    data += count;
    size -= size_t(count);
  }
  return size == 0;
}

void hex(const uint8_t* data, unsigned size, char* output) {
  const char* digits = "0123456789abcdef";
  for (unsigned i = 0; i < size; ++i) {
    output[2 * i] = digits[data[i] >> 4];
    output[2 * i + 1] = digits[data[i] & 15];
  }
  output[2 * size] = 0;
}

void clearSession() {
  state = State::Idle;
  stagedLength = 0;
  memset(stagedHash, 0, sizeof(stagedHash));
  stagedCRC = 0;
  activationReady = false;
}

bool verifyStaging(uint32_t length, const uint8_t expected[32], uint16_t& crc) {
  uint8_t buffer[512], digest[32];
  uint32_t offset = 0;
  CRC16_Context checksum;
  CRC16_Init(&checksum);
  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts(&sha, 0);
  bool ok = true;
  while (offset < length) {
    uint32_t count = length - offset;
    if (count > sizeof(buffer)) count = sizeof(buffer);
    uint32_t before = offset;
    if (uint32_t(millis() - sessionStarted) >= SESSION_TIMEOUT ||
        MicoFlashRead(MICO_PARTITION_OTA_TEMP, &offset, buffer, count) != kNoErr ||
        offset != before + count ||
        (before == 0 && !Http::validImageVectors(buffer, length))) { ok = false; break; }
    mbedtls_sha256_update(&sha, buffer, count);
    CRC16_Update(&checksum, buffer, count);
    Thread::wait(1);
  }
  mbedtls_sha256_finish(&sha, digest);
  mbedtls_sha256_free(&sha);
  CRC16_Final(&checksum, &crc);
  return ok && memcmp(digest, expected, 32) == 0;
}

bool commitBootMetadata(uint32_t length, uint16_t crc) {
  mico_Context_t* context = mico_system_context_get();
  if (!context || !otaPartitionsValid()) return false;
  boot_table_t previous = context->bootTable;
  boot_table_t next = {};
  next.start_address = MicoFlashGetInfo(MICO_PARTITION_OTA_TEMP)->partition_start_addr;
  next.length = length;
  next.type = 'A';
  next.upgrade_type = 'U';
  next.crc = crc;
  context->bootTable = next;
  // The SDK OTA wrapper discards this return code. Use its underlying API.
  bool ok = mico_system_context_update(context) == kNoErr;
  const mico_partition_t parameters[] = {MICO_PARTITION_PARAMETER_1, MICO_PARTITION_PARAMETER_2};
  for (unsigned i = 0; i < 2; ++i) {
    uint32_t offset = 0;
    boot_table_t readback;
    ok = MicoFlashRead(parameters[i], &offset, reinterpret_cast<uint8_t*>(&readback),
                       sizeof(readback)) == kNoErr &&
         memcmp(&next, &readback, sizeof(next)) == 0 && ok;
  }
  if (!ok) {
    context->bootTable = previous;
    if (mico_system_context_update(context) != kNoErr) {
      Serial.println("OTA: boot metadata recovery failed; keep ST-Link recovery available");
    }
  }
  return ok;
}

} // namespace

bool httpReply(WiFiClient& client, int code, const char* body, const char* type) {
  char header[384];
  const char* status = code == 200 ? "OK" : code == 401 ? "Unauthorized" :
                       code == 403 ? "Forbidden" : code == 409 ? "Conflict" :
                       code == 503 ? "Service Unavailable" : "Bad Request";
  int length = snprintf(header, sizeof(header),
      "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %u\r\n"
      "Connection: close\r\nCache-Control: no-store\r\n"
      "X-Content-Type-Options: nosniff\r\n%s\r\n",
      code, status, type, unsigned(strlen(body)),
      code == 401 ? "WWW-Authenticate: Basic realm=\"AZ3166 admin\"\r\n" : "");
  return length > 0 && size_t(length) < sizeof(header) &&
      writeAll(client, header, size_t(length)) && writeAll(client, body, strlen(body));
}

const char* adminCsrfToken() {
  if (!csrf[0]) {
    uint8_t random[32];
    if (!secureRandom(random, sizeof(random))) {
      Serial.println("AUTH: entropy failed; administrative mutations locked");
      return "";
    }
    hex(random, sizeof(random), csrf);
  }
  return csrf;
}

bool authorizeAdmin(WiFiClient& client, const Http::Request& request) {
  if (!operatorConfigured()) {
    httpReply(client, 503, "Configure an operator password locally: send P over USB serial.");
    return false;
  }
  if (loginBlocked && uint32_t(millis() - failedLoginAt) < 2000) {
    httpReply(client, 401, "Authentication rate limited");
    return false;
  }
  uint8_t decoded[80] = {};
  size_t length = 0;
  bool ok = strncmp(request.authorization, "Basic ", 6) == 0 &&
      mbedtls_base64_decode(decoded, sizeof(decoded) - 1, &length,
          reinterpret_cast<const uint8_t*>(request.authorization + 6),
          strlen(request.authorization + 6)) == 0 &&
      length >= 22 && length <= 70 && memchr(decoded, 0, length) == NULL &&
      memcmp(decoded, "admin:", 6) == 0 &&
      checkOperatorPassword(reinterpret_cast<const char*>(decoded + 6));
  memset(decoded, 0, sizeof(decoded));
  if (!ok) {
    if (request.authorization[0]) { failedLoginAt = millis(); loginBlocked = true; }
    httpReply(client, 401, "Operator authentication required");
    return false;
  }
  loginBlocked = false;
  return true;
}

bool authorizeMutation(WiFiClient& client, const Http::Request& request) {
  const char* token = adminCsrfToken();
  if (strcmp(request.method, "POST") || !token[0] || strcmp(token, request.csrf)) {
    httpReply(client, 403, "POST and a valid X-CSRF-Token are required");
    return false;
  }
  return true;
}

bool handleOtaRequest(WiFiClient& client, const Http::Request& request) {
  if (strcmp(request.path, "/ota") == 0 && strcmp(request.method, "GET") == 0) {
    String body(OTA_PAGE);
    body.replace("__CSRF__", adminCsrfToken());
    httpReply(client, 200, body.c_str(), "text/html");
    return true;
  }
  if (strcmp(request.path, "/ota/status") == 0 && strcmp(request.method, "GET") == 0) {
    stateMutex.lock();
    const char* text = state == State::Staged ? "staged" :
                       state == State::Idle ? "idle" : "busy";
    char body[160];
    snprintf(body, sizeof(body), "{\"state\":\"%s\",\"csrf\":\"%s\"}", text, adminCsrfToken());
    stateMutex.unlock();
    httpReply(client, 200, body, "application/json");
    return true;
  }
  if (strncmp(request.path, "/ota/", 5) != 0 && strcmp(request.path, "/ota") != 0) return false;
  if (!authorizeMutation(client, request)) return true;

  if (strcmp(request.path, "/ota/abort") == 0) {
    stateMutex.lock();
    bool ok = state != State::Applying;
    if (ok) clearSession();
    stateMutex.unlock();
    httpReply(client, ok ? 200 : 409, ok ? "Staged image discarded" : "Installation already requested");
    return true;
  }
  if (strcmp(request.path, "/ota/apply") == 0) {
    uint8_t expected[32];
    uint32_t length = 0;
    stateMutex.lock();
    bool ok = state == State::Staged &&
        uint32_t(millis() - sessionStarted) < SESSION_TIMEOUT &&
        Http::fromHex(request.sha256, expected, sizeof(expected)) &&
        memcmp(expected, stagedHash, sizeof(expected)) == 0;
    if (ok) {
      state = State::Applying;
      length = stagedLength;
    }
    stateMutex.unlock();
    if (!ok) { httpReply(client, 409, "No matching verified staged image"); return true; }
    uint16_t crc = 0;
    if (!storedConfigDurable() || !verifyStaging(length, expected, crc)) {
      stateMutex.lock(); clearSession(); stateMutex.unlock();
      httpReply(client, 409, "Image or configuration readback failed; update not activated");
      return true;
    }
    stateMutex.lock();
    bool timely = state == State::Applying &&
                  uint32_t(millis() - sessionStarted) < SESSION_TIMEOUT - 10000;
    if (!timely) clearSession();
    stateMutex.unlock();
    if (!timely) {
      httpReply(client, 409, "Maintenance window expired; upload again");
      return true;
    }
    // Respond before the main loop records boot metadata and reboots.
    bool replied = httpReply(client, 200, "Verified. Rebooting to install firmware.");
    client.stop();
    stateMutex.lock();
    if (!replied) clearSession();
    else if (state == State::Applying) {
      stagedCRC = crc;
      activationReady = true;
    }
    stateMutex.unlock();
    return true;
  }
  if (strcmp(request.path, "/ota/upload") != 0) {
    httpReply(client, 400, "Unknown OTA route");
    return true;
  }
  uint8_t expected[32];
  if (!request.hasLength || request.length < 8 || request.length > MAX_IMAGE ||
      !Http::fromHex(request.sha256, expected, sizeof(expected)) ||
      !otaPartitionsValid() || !storedConfigDurable()) {
    httpReply(client, 400, "Invalid length/hash or configuration storage not durable");
    return true;
  }
  stateMutex.lock();
  if (state != State::Idle) {
    stateMutex.unlock();
    httpReply(client, 409, "Discard the previous update first");
    return true;
  }
  sessionStarted = millis();
  state = State::Receiving;
  stateMutex.unlock();
  uint32_t eraseSize = (request.length + 4095) & ~4095u;
  bool ok = MicoFlashErase(MICO_PARTITION_OTA_TEMP, 0, eraseSize) == kNoErr;
  uint8_t buffer[512];
  uint32_t received = 0, lastData = millis();
  while (ok && received < request.length) {
    if (uint32_t(millis() - sessionStarted) >= SESSION_TIMEOUT ||
        uint32_t(millis() - lastData) >= 10000) { ok = false; break; }
    unsigned want = request.length - received;
    if (want > sizeof(buffer)) want = sizeof(buffer);
    int count = client.read(buffer, want);
    if (count == NSAPI_ERROR_WOULD_BLOCK) { Thread::wait(1); continue; }
    if (count <= 0 || unsigned(count) > want) { ok = false; break; }
    lastData = millis();
    uint32_t offset = received;
    ok = MicoFlashWrite(MICO_PARTITION_OTA_TEMP, &offset, buffer, unsigned(count)) == kNoErr;
    received += unsigned(count);
  }
  uint16_t crc = 0;
  ok = ok && received == request.length && verifyStaging(received, expected, crc) &&
       storedConfigDurable();
  stateMutex.lock();
  if (ok) {
    stagedLength = received;
    memcpy(stagedHash, expected, sizeof(expected));
    stagedCRC = crc;
    state = State::Staged;
  } else clearSession();
  stateMutex.unlock();
  Serial.println(ok ? "OTA: image staged and readback verified" : "OTA: upload rejected; no activation");
  httpReply(client, ok ? 200 : 400,
      ok ? "Image verified. Apply within this maintenance window." :
           "Upload interrupted, invalid, timed out, or hash/readback failed");
  return true;
}

bool otaMaintenance() {
  stateMutex.lock();
  bool active = state != State::Idle;
  if (active && uint32_t(millis() - sessionStarted) >= SESSION_TIMEOUT) {
    // Receiving is owned by the web thread, which enforces the same deadline.
    if (state == State::Staged || state == State::Applying) {
      Serial.println("OTA: maintenance window expired");
      clearSession();
    }
  }
  active = state != State::Idle &&
           uint32_t(millis() - sessionStarted) < SESSION_TIMEOUT;
  stateMutex.unlock();
  return active;
}

void pollOta() {
  static bool maintenance = false;
  bool active = otaMaintenance();
  if (active && !maintenance) {
    maintenance = true;
    Serial.println("OTA: bounded maintenance started");
  }
  if (!active && maintenance) {
    resetMqttWatchdog();
    maintenance = false;
    Serial.println("OTA: maintenance ended");
  }
  stateMutex.lock();
  bool apply = state == State::Applying && activationReady;
  stateMutex.unlock();
  if (apply) {
    if (!storedConfigDurable() || !commitBootMetadata(stagedLength, stagedCRC)) {
      Serial.println("OTA: activation failed; not rebooting");
      stateMutex.lock(); clearSession(); stateMutex.unlock();
      return;
    }
    Serial.println("OTA: boot metadata committed; rebooting");
    Serial.flush();
    NVIC_SystemReset();
  }
}

void pollOperatorSerial() {
  if (otaMaintenance()) return;
  if (passwordEntry && uint32_t(millis() - passwordStarted) > 30000) {
    memset(serialPassword, 0, sizeof(serialPassword));
    passwordEntry = false;
    Serial.println("AUTH: password entry timed out");
  }
  while (Serial.available()) {
    char c = Serial.read();
    if (!passwordEntry) {
      if (c == 'P') {
        passwordEntry = true; serialUsed = 0; passwordStarted = millis();
        passwordRejected = false;
        Serial.println("AUTH: enter a new 16-64 character operator password (not echoed):");
      }
    } else if (c == '\n' || c == '\r') {
      if (serialUsed == 0) continue;
      serialPassword[serialUsed] = 0;
      if (passwordRejected) Serial.println("AUTH: invalid or overlong password; unchanged");
      else setOperatorPassword(serialPassword);
      memset(serialPassword, 0, sizeof(serialPassword));
      passwordEntry = false;
    } else if (c >= 33 && c <= 126 && serialUsed < sizeof(serialPassword) - 1) {
      serialPassword[serialUsed++] = c;
    } else {
      passwordRejected = true;
    }
  }
}
