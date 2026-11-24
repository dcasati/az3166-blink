#pragma once
#include <stdint.h>

struct RecoveryStatus {
  const char* state;
  uint32_t attempts, reason, error;
  bool bootloader;
};
void beginRecovery();
void pollRecovery(bool localHealthy);
void recoveryStartupProgress();
bool recoveryTrial();
bool recoveryLocalOnly();
bool recoveryCanUpload();
bool requestRecoveryPreparation();
bool requestRecoveryInstall(uint32_t length, const uint8_t hash[32]);
RecoveryStatus recoveryStatus();
