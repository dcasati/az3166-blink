#include <Arduino.h>
#include "ConfigStorage.h"
#include "DurableRecord.h"
#include "mico.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/md.h"
#include "rtos.h"

namespace {
const uint32_t LEGACY_ADDRESS = 0x080C0000;
const uint32_t PASSWORD_ITERATIONS = 10000;
const mico_partition_t CONFIG_PARTITION = static_cast<mico_partition_t>(MICO_PARTITION_FILESYS);
rtos::Mutex storageMutex;
struct Payload {
  DeviceConfig config;
  OperatorAuth auth;
};
Payload current = {};
bool durable = false;
bool writable = false;

struct Flash {
  bool read(uint32_t offset, void* output, uint32_t size) {
    return offset <= 8192 && size <= 8192 - offset &&
        MicoFlashRead(CONFIG_PARTITION, &offset,
                      static_cast<uint8_t*>(output), size) == kNoErr;
  }
  bool write(uint32_t offset, const void* input, uint32_t size) {
    return offset <= 8192 && size <= 8192 - offset &&
        MicoFlashWrite(CONFIG_PARTITION, &offset,
                      const_cast<uint8_t*>(static_cast<const uint8_t*>(input)), size) == kNoErr;
  }
  bool erase(uint32_t offset, uint32_t size) {
    return offset % 4096 == 0 && size == 4096 && offset <= 4096 &&
           MicoFlashErase(CONFIG_PARTITION, offset, size) == kNoErr;
  }
} flash;
DurableRecord::Store<Payload, Flash> store(flash);

bool hashPassword(const char* password, const OperatorAuth& auth, uint8_t output[32]) {
  if (auth.iterations != PASSWORD_ITERATIONS) return false;
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  int result = mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  if (result == 0) {
    result = mbedtls_pkcs5_pbkdf2_hmac(&ctx,
        reinterpret_cast<const uint8_t*>(password), strlen(password),
        auth.salt, sizeof(auth.salt), auth.iterations, 32, output);
  }
  mbedtls_md_free(&ctx);
  return result == 0;
}
bool validPayload(const Payload& payload) {
  return validDeviceConfig(payload.config) &&
         (payload.auth.iterations == 0 || payload.auth.iterations == PASSWORD_ITERATIONS);
}
bool reservationErased() {
  uint8_t bytes[128];
  for (uint32_t offset = 0; offset < 8192; offset += sizeof(bytes)) {
    if (!flash.read(offset, bytes, sizeof(bytes))) return false;
    for (unsigned i = 0; i < sizeof(bytes); ++i) if (bytes[i] != 0xFF) return false;
  }
  Serial.println("CONFIG: partition layout and both unused 4 KiB sectors verified");
  return true;
}

bool readLegacy(DeviceConfig& legacy) {
  memcpy(&legacy, reinterpret_cast<const void*>(LEGACY_ADDRESS), sizeof(legacy));
  return validLegacyConfig(legacy);
}
} // namespace

bool otaPartitionsValid() {
  const mico_logic_partition_t* fs = MicoFlashGetInfo(CONFIG_PARTITION);
  const mico_logic_partition_t* ota = MicoFlashGetInfo(MICO_PARTITION_OTA_TEMP);
  const mico_logic_partition_t* app = MicoFlashGetInfo(MICO_PARTITION_APPLICATION);
  const mico_logic_partition_t* primary = MicoFlashGetInfo(MICO_PARTITION_PARAMETER_1);
  const mico_logic_partition_t* backup = MicoFlashGetInfo(MICO_PARTITION_PARAMETER_2);
  return fs && ota && app && primary && backup &&
      primary->partition_owner == MICO_FLASH_QSPI &&
      primary->partition_start_addr == 0 && primary->partition_length == 4096 &&
      backup->partition_owner == MICO_FLASH_QSPI &&
      backup->partition_start_addr == 4096 && backup->partition_length == 4096 &&
      fs->partition_owner == MICO_FLASH_QSPI &&
      fs->partition_start_addr == 0x164000 && fs->partition_length == 0x9C000 &&
      ota->partition_owner == MICO_FLASH_QSPI &&
      ota->partition_start_addr == 0x70000 && ota->partition_length == 0xF4000 &&
      app->partition_owner == MICO_FLASH_EMBEDDED &&
      app->partition_start_addr == 0x0800C000 && app->partition_length == 0xF4000;
}

ConfigLoad loadStoredConfig(DeviceConfig& config) {
  storageMutex.lock();
  writable = otaPartitionsValid();
  ConfigLoad result = ConfigLoad::Failed;
  if (writable) {
    DurableRecord::Load loaded = store.load(current, validPayload);
    if (loaded == DurableRecord::Load::Loaded && validPayload(current)) {
      config = current.config;
      durable = true;
      result = ConfigLoad::Loaded;
    } else if (loaded == DurableRecord::Load::Empty && reservationErased()) {
      DeviceConfig legacy;
      if (readLegacy(legacy)) {
        current.config = legacy;
        if (store.save(current)) {
          config = legacy;
          durable = true;
          result = ConfigLoad::Migrated;
        }
      } else {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&legacy);
        bool erased = true;
        for (size_t i = 0; i < sizeof(legacy); ++i) if (bytes[i] != 0xFF) erased = false;
        if (erased) result = ConfigLoad::Empty;
      }
    }
  }
  if (result == ConfigLoad::Failed) {
    writable = false;
    if (readLegacy(config)) {
      result = ConfigLoad::LegacyOnly;
      Serial.println("CONFIG: recovery mode uses legacy settings; external writes and OTA locked");
    }
  }
  storageMutex.unlock();
  Serial.println(result == ConfigLoad::Failed ? "CONFIG: external storage failed; OTA locked" :
                 result == ConfigLoad::LegacyOnly ? "CONFIG: legacy-only recovery; OTA locked" :
                 result == ConfigLoad::Migrated ? "CONFIG: migrated to redundant external flash" :
                 result == ConfigLoad::Loaded ? "CONFIG: external record loaded" :
                 "CONFIG: no saved settings");
  return result;
}

bool saveStoredConfig(const DeviceConfig& config) {
  storageMutex.lock();
  Payload next = current;
  next.config = config;
  bool ok = writable && validPayload(next) && store.save(next);
  if (ok) { current = next; durable = true; }
  storageMutex.unlock();
  Serial.println(ok ? "CONFIG: external record committed" : "CONFIG: external save failed");
  return ok;
}

bool storedConfigDurable() {
  storageMutex.lock();
  Payload readback;
  bool ok = durable && store.load(readback, validPayload) == DurableRecord::Load::Loaded &&
            memcmp(&readback, &current, sizeof(current)) == 0;
  storageMutex.unlock();
  return ok;
}

bool operatorConfigured() {
  storageMutex.lock();
  bool configured = durable && current.auth.iterations == PASSWORD_ITERATIONS;
  storageMutex.unlock();
  return configured;
}

bool secureRandom(uint8_t* output, unsigned length) {
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context drbg;
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&drbg);
  const unsigned char personalization[] = "az3166-ota";
  int result = mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy,
                                    personalization, sizeof(personalization));
  if (result == 0) result = mbedtls_ctr_drbg_random(&drbg, output, length);
  mbedtls_ctr_drbg_free(&drbg);
  mbedtls_entropy_free(&entropy);
  return result == 0;
}

bool setOperatorPassword(const char* password) {
  const size_t length = strlen(password);
  if (length < 16 || length > 64) return false;
  storageMutex.lock();
  Payload next = current;
  next.auth.iterations = PASSWORD_ITERATIONS;
  bool ok = durable && secureRandom(next.auth.salt, sizeof(next.auth.salt)) &&
            hashPassword(password, next.auth, next.auth.verifier) && store.save(next);
  if (ok) current = next;
  memset(&next.auth, 0, sizeof(next.auth));
  storageMutex.unlock();
  Serial.println(ok ? "AUTH: operator password stored" : "AUTH: password provisioning failed");
  return ok;
}

bool checkOperatorPassword(const char* password) {
  storageMutex.lock();
  uint8_t actual[32] = {};
  bool ok = durable && hashPassword(password, current.auth, actual);
  uint8_t difference = 0;
  for (size_t i = 0; i < sizeof(actual); ++i) difference |= actual[i] ^ current.auth.verifier[i];
  memset(actual, 0, sizeof(actual));
  storageMutex.unlock();
  return ok && difference == 0;
}
