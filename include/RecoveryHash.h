#pragma once
#include "Recovery.h"
#include "mbedtls/sha256.h"

namespace Recovery {
template <typename Ops>
bool hash(Ops& ops, Region region, uint32_t length, uint8_t output[32]) {
  if (!bounds(region, 0, length)) return false;
  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  bool ok = mbedtls_sha256_starts_ret(&sha, 0) == 0;
  uint8_t buffer[512];
  for (uint32_t offset = 0; ok && offset < length;) {
    uint32_t size = length - offset;
    if (size > sizeof(buffer)) size = sizeof(buffer);
    ok = ops.read(region, offset, buffer, size) &&
        mbedtls_sha256_update_ret(&sha, buffer, size) == 0;
    if (ok) { offset += size; ops.progress(); }
  }
  if (ok) ok = mbedtls_sha256_finish_ret(&sha, output) == 0;
  mbedtls_sha256_free(&sha);
  return ok;
}
} // namespace Recovery
