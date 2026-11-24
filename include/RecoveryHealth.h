#pragma once
#include <stdint.h>

namespace Recovery {
class Health {
public:
  bool observe(uint32_t now, bool healthy) {
    if (!healthy) { count_ = 0; started_ = now; return false; }
    if (!count_) started_ = now;
    if (count_ < 10) ++count_;
    return count_ >= 10 && uint32_t(now - started_) >= 1000;
  }
private:
  uint32_t count_ = 0, started_ = 0;
};
} // namespace Recovery
