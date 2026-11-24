#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace Http {
struct Request {
  char method[5] = {};
  char path[512] = {};
  char authorization[128] = {};
  char csrf[65] = {};
  char sha256[65] = {};
  uint32_t length = 0;
  bool hasLength = false;
  bool hasAuthorization = false;
  bool hasCsrf = false;
  bool hasHash = false;
};

inline bool equalIgnoreCase(const char* a, const char* b) {
  while (*a && *b) {
    char x = *a++, y = *b++;
    if (x >= 'A' && x <= 'Z') x += 'a' - 'A';
    if (y >= 'A' && y <= 'Z') y += 'a' - 'A';
    if (x != y) return false;
  }
  return *a == *b;
}

inline bool decimal(const char* value, uint32_t& number) {
  if (!*value) return false;
  number = 0;
  for (; *value; ++value) {
    if (*value < '0' || *value > '9' ||
        number > (0xFFFFFFFFu - unsigned(*value - '0')) / 10) return false;
    number = number * 10 + unsigned(*value - '0');
  }
  return true;
}

inline bool copy(char* target, size_t capacity, const char* value) {
  size_t length = strlen(value);
  if (length >= capacity) return false;
  memcpy(target, value, length + 1);
  return true;
}

template <typename Client, typename Clock, typename Sleep>
bool readRequest(Client& client, Request& request, int wouldBlock,
                 Clock clock, Sleep sleep) {
  char line[640];
  unsigned used = 0, total = 0, lines = 0;
  bool carriageReturn = false, first = true;
  uint32_t start = clock();
  while (uint32_t(clock() - start) < 5000 && total < 4096) {
    uint8_t byte;
    int count = client.read(&byte, 1);
    if (count < 0 && count == wouldBlock) { sleep(1); continue; }
    if (count != 1) return false;
    ++total;
    if (byte == '\r') {
      if (carriageReturn) return false;
      carriageReturn = true;
      continue;
    }
    if (byte != '\n') {
      if (carriageReturn || byte < 32 || byte > 126 || used >= sizeof(line) - 1) return false;
      line[used++] = char(byte);
      continue;
    }
    if (!carriageReturn) return false;
    carriageReturn = false;
    line[used] = 0;
    if (first) {
      char* path = strchr(line, ' ');
      if (!path) return false;
      *path++ = 0;
      char* version = strchr(path, ' ');
      if (!version) return false;
      *version++ = 0;
      if ((strcmp(line, "GET") && strcmp(line, "POST")) ||
          strcmp(version, "HTTP/1.1") || path[0] != '/' ||
          !copy(request.method, sizeof(request.method), line) ||
          !copy(request.path, sizeof(request.path), path)) return false;
      first = false;
    } else {
      if (used == 0) return true;
      if (++lines > 32) return false;
      char* value = strchr(line, ':');
      if (!value) return false;
      *value++ = 0;
      if (!line[0]) return false;
      for (const char* p = line; *p; ++p) {
        if (!( (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
               (*p >= '0' && *p <= '9') || *p == '-')) return false;
      }
      while (*value == ' ') ++value;
      char* end = value + strlen(value);
      while (end > value && end[-1] == ' ') *--end = 0;
      if (equalIgnoreCase(line, "Content-Length")) {
        if (request.hasLength || !decimal(value, request.length)) return false;
        request.hasLength = true;
      } else if (equalIgnoreCase(line, "Transfer-Encoding") ||
                 equalIgnoreCase(line, "Expect")) {
        return false;
      } else if (equalIgnoreCase(line, "Authorization")) {
        if (request.hasAuthorization ||
            !copy(request.authorization, sizeof(request.authorization), value)) return false;
        request.hasAuthorization = true;
      } else if (equalIgnoreCase(line, "X-CSRF-Token")) {
        if (request.hasCsrf || !copy(request.csrf, sizeof(request.csrf), value)) return false;
        request.hasCsrf = true;
      } else if (equalIgnoreCase(line, "X-Firmware-SHA256")) {
        if (request.hasHash || !copy(request.sha256, sizeof(request.sha256), value)) return false;
        request.hasHash = true;
      }
    }
    used = 0;
  }
  return false;
}

inline bool fromHex(const char* text, uint8_t* output, size_t length) {
  if (strlen(text) != length * 2) return false;
  for (size_t i = 0; i < length; ++i) {
    unsigned value = 0;
    for (unsigned j = 0; j < 2; ++j) {
      char c = text[i * 2 + j];
      unsigned digit;
      if (c >= '0' && c <= '9') digit = unsigned(c - '0');
      else if (c >= 'a' && c <= 'f') digit = unsigned(c - 'a') + 10;
      else if (c >= 'A' && c <= 'F') digit = unsigned(c - 'A') + 10;
      else return false;
      value = value * 16 + digit;
    }
    output[i] = uint8_t(value);
  }
  return true;
}

inline bool validImageVectors(const uint8_t* image, uint32_t length) {
  if (length < 8 || length > 0xF4000) return false;
  uint32_t stack, reset;
  memcpy(&stack, image, 4);
  memcpy(&reset, image + 4, 4);
  return stack > 0x200001C4 && stack <= 0x20040000 && (stack & 7) == 0 &&
      (reset & 1) && (reset & ~1u) >= 0x0800C000 &&
      (reset & ~1u) < 0x0800C000 + length;
}
} // namespace Http
