#include "RecoveryService.h"

#ifdef AZ3166_RECOVERY
#include <Arduino.h>
#include "ConfigStorage.h"
#include "RecoveryHash.h"
#include "RecoveryHealth.h"
#include "stm32f4xx_hal.h"
#include "rtos.h"
#include "mico.h"

__attribute__((section(".recovery_abi"), used))
const Recovery::Tag recoveryImageTag = Recovery::IMAGE_TAG;
extern "C" uint8_t __recovery_image_end;

namespace {
using namespace Recovery;
rtos::Mutex statusMutex;
bool bootPresent = false, faulted = false, preparation = false;
bool localReady = false;
uint32_t faultCode = 0;
RecoveryStatus cached = {"unprepared", 0, 0, 0, false};
Health health;

bool protectedBoot() { return bootOptionsReady(FLASH->OPTCR); }
struct Ops {
  void progress() { recoveryStartupProgress(); }
  bool read(Region region, uint32_t offset, void* output, uint32_t size) {
    if (!bounds(region, offset, size)) return false;
    if (region == Region::Internal) {
      memcpy(output, reinterpret_cast<const void*>(APP_BASE + offset), size);
      return true;
    }
    mico_partition_t partition = region == Region::Staging ? MICO_PARTITION_OTA_TEMP :
        static_cast<mico_partition_t>(MICO_PARTITION_FILESYS);
    uint32_t address = region == Region::Journal ? JOURNAL_BASE - 0x164000 + offset :
        region == Region::Fallback ? FALLBACK_BASE - 0x164000 + offset : offset;
    uint32_t start = address;
    return MicoFlashRead(partition, &address, static_cast<uint8_t*>(output), size) == kNoErr &&
        address == start + size;
  }
  bool write(Region region, uint32_t offset, const void* input, uint32_t size) {
    if ((region != Region::Journal && region != Region::Fallback) ||
        !bounds(region, offset, size)) return false;
    uint32_t address = (region == Region::Journal ? JOURNAL_BASE : FALLBACK_BASE) - 0x164000 + offset;
    uint32_t start = address;
    return MicoFlashWrite(static_cast<mico_partition_t>(MICO_PARTITION_FILESYS), &address,
        const_cast<uint8_t*>(static_cast<const uint8_t*>(input)), size) == kNoErr &&
        address == start + size;
  }
  bool erase(Region region, uint32_t offset, uint32_t size) {
    if ((region != Region::Journal && region != Region::Fallback) ||
        !bounds(region, offset, size) || offset % SECTOR || size % SECTOR) return false;
    uint32_t base = (region == Region::Journal ? JOURNAL_BASE : FALLBACK_BASE) - 0x164000;
    for (uint32_t end = offset + size; offset < end; offset += SECTOR) {
      if (MicoFlashErase(static_cast<mico_partition_t>(MICO_PARTITION_FILESYS),
                         base + offset, SECTOR) != kNoErr) return false;
      progress();
    }
    return true;
  }
  bool hash(Region region, uint32_t size, uint8_t output[32]) {
    return Recovery::hash(*this, region, size, output);
  }
} ops;
Engine<Ops> engine(ops);

void publish(bool loaded) {
  static const char* const names[] = {
    "ready", "pending", "installing", "trial", "restoring", "promoting", "preparing"
  };
  statusMutex.lock();
  cached.state = faulted ? "fault" : loaded ? names[uint32_t(engine.status().state)] : "unprepared";
  cached.attempts = loaded ? engine.status().attempts : 0;
  cached.reason = loaded ? uint32_t(engine.status().reason) : 0;
  cached.error = faultCode;
  cached.bootloader = bootPresent;
  statusMutex.unlock();
}
void failed(const char* operation, Error error = Error::None) {
  faulted = true;
  faultCode = uint32_t(error == Error::None ? engine.error() : error);
  Serial.print("RECOVERY: "); Serial.print(operation);
  Serial.print(" failed; updates locked, error "); Serial.println(faultCode);
  publish(false);
}
} // namespace

void beginRecovery() {
  bootPresent = memcmp(reinterpret_cast<const void*>(0x08000200),
                       &Recovery::BOOT_TAG, sizeof(Recovery::BOOT_TAG)) == 0;
  if (!otaPartitionsValid() || (bootPresent && !protectedBoot())) {
    failed("partition/protection preflight", Error::Ownership); return;
  }
  DurableRecord::Load loaded = engine.load();
  if (loaded == DurableRecord::Load::Failed ||
      (bootPresent && loaded != DurableRecord::Load::Loaded)) {
    failed("journal load"); return;
  }
  publish(loaded == DurableRecord::Load::Loaded);
  Serial.println(bootPresent ? "RECOVERY: boot ABI detected; hardware watchdog required" :
                             "RECOVERY: vendor bootstrap mode; OTA locked; USB B prepares fallback");
}
void recoveryStartupProgress() {
  if (bootPresent && !faulted) IWDG->KR = 0xAAAA;
}
RecoveryStatus recoveryStatus() {
  statusMutex.lock();
  RecoveryStatus result = cached;
  statusMutex.unlock();
  return result;
}
bool recoveryTrial() {
  return strcmp(recoveryStatus().state, "trial") == 0;
}
bool recoveryLocalOnly() {
  RecoveryStatus status = recoveryStatus();
  return status.bootloader && strcmp(status.state, "ready") != 0;
}
bool recoveryCanUpload() {
  RecoveryStatus status = recoveryStatus();
  return status.bootloader && protectedBoot() && strcmp(status.state, "ready") == 0;
}
bool requestRecoveryPreparation() {
  statusMutex.lock();
  bool ok = !bootPresent && !faulted && !preparation && localReady &&
      (strcmp(cached.state, "unprepared") == 0 || strcmp(cached.state, "preparing") == 0);
  if (ok) { preparation = true; cached.state = "preparing"; }
  statusMutex.unlock();
  return ok;
}
bool requestRecoveryInstall(uint32_t length, const uint8_t hash[32]) {
  if (!recoveryCanUpload()) return false;
  statusMutex.lock();
  cached.state = "verifying";
  statusMutex.unlock();
  Image image = {};
  image.length = length;
  memcpy(image.hash, hash, sizeof(image.hash));
  if (!engine.request(image)) { failed("activation"); return false; }
  publish(true);
  return true;
}
void pollRecovery(bool localHealthy) {
  localReady = localHealthy && storedConfigDurable() && operatorConfigured();
  if (!localReady || faulted) { health.observe(millis(), false); return; }
  bool prepare;
  statusMutex.lock();
  prepare = preparation;
  preparation = false;
  statusMutex.unlock();
  if (prepare) {
    Image baseline = {};
    baseline.length = uint32_t(reinterpret_cast<uintptr_t>(&__recovery_image_end) - APP_BASE);
    if (!Recovery::validLength(baseline.length) ||
        !ops.hash(Region::Internal, baseline.length, baseline.hash)) {
      failed("baseline identity", Error::InvalidImage); return;
    }
    if (!engine.prepare(baseline)) { failed("baseline preparation"); return; }
    publish(true);
    Serial.println("RECOVERY: fallback and redundant journal readback verified; bootstrap still requires approval");
  }
  if (bootPresent && recoveryTrial() && health.observe(millis(), true)) {
    if (!engine.confirm()) { failed("local confirmation"); return; }
    publish(true);
    Serial.println("RECOVERY: local health confirmed; rebooting for restartable promotion");
    Serial.flush();
    NVIC_SystemReset();
  }
  recoveryStartupProgress();
}

#else
void beginRecovery() {}
void pollRecovery(bool) {}
void recoveryStartupProgress() {}
bool recoveryTrial() { return false; }
bool recoveryLocalOnly() { return false; }
bool recoveryCanUpload() { return false; }
bool requestRecoveryPreparation() { return false; }
bool requestRecoveryInstall(uint32_t, const uint8_t[32]) { return false; }
RecoveryStatus recoveryStatus() { return {"disabled", 0, 0, 0, false}; }
#endif
