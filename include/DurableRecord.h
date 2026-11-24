#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace DurableRecord {
const uint32_t MAGIC = 0x415A4346;
const uint32_t COMMITTED = 0x434F4D54;
const uint32_t SECTOR_SIZE = 4096;

inline uint32_t crc32(const void* data, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  const uint8_t* bytes = static_cast<const uint8_t*>(data);
  while (length--) {
    crc ^= *bytes++;
    for (unsigned bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xEDB88320 & (0u - (crc & 1)));
    }
  }
  return ~crc;
}

template <typename Payload> struct Record {
  uint32_t magic;
  uint32_t version;
  uint32_t length;
  uint32_t generation;
  Payload payload;
  uint32_t crc;
  uint32_t commit;
};

enum class Load { Loaded, Empty, Failed };

template <typename Payload, typename Flash> class Store {
public:
  explicit Store(Flash& flash) : flash_(flash) {}

  Load load(Payload& payload) {
    return load(payload, [](const Payload&) { return true; });
  }

  template <typename Validator> Load load(Payload& payload, Validator validate) {
    Record<Payload> records[2];
    bool valid[2] = {false, false};
    bool erased[2] = {false, false};
    for (unsigned i = 0; i < 2; ++i) {
      if (!flash_.read(i * SECTOR_SIZE, &records[i], sizeof(records[i]))) {
        return Load::Failed;
      }
      const Record<Payload>& r = records[i];
      valid[i] = r.magic == MAGIC && r.version == 1 &&
                 r.length == sizeof(Payload) && r.commit == COMMITTED &&
                 r.crc == crc32(&r, offsetof(Record<Payload>, crc)) &&
                 validate(r.payload);
      erased[i] = true;
      const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&r);
      for (size_t j = 0; j < sizeof(r); ++j) {
        if (bytes[j] != 0xFF) erased[i] = false;
      }
    }
    if (!valid[0] && !valid[1]) {
      return erased[0] && erased[1] ? Load::Empty : Load::Failed;
    }
    unsigned slot = valid[1] && (!valid[0] ||
        (records[1].generation != records[0].generation &&
         uint32_t(records[1].generation - records[0].generation) < 0x80000000u))
        ? 1 : 0;
    payload = records[slot].payload;
    slot_ = int(slot);
    generation_ = records[slot].generation;
    return Load::Loaded;
  }

  bool save(const Payload& payload) {
    static_assert(sizeof(Record<Payload>) <= SECTOR_SIZE, "Record exceeds sector");
    const unsigned slot = slot_ == 0 ? 1 : 0;
    Record<Payload> record = {};
    record.magic = MAGIC;
    record.version = 1;
    record.length = sizeof(Payload);
    record.generation = generation_ + 1;
    record.payload = payload;
    record.crc = crc32(&record, offsetof(Record<Payload>, crc));
    record.commit = 0xFFFFFFFF;
    const uint32_t offset = slot * SECTOR_SIZE;
    if (!flash_.erase(offset, SECTOR_SIZE) ||
        !flash_.write(offset, &record, offsetof(Record<Payload>, commit))) {
      return false;
    }
    Record<Payload> verify;
    if (!flash_.read(offset, &verify, sizeof(verify)) ||
        memcmp(&verify, &record, sizeof(record)) != 0) return false;
    record.commit = COMMITTED;
    if (!flash_.write(offset + offsetof(Record<Payload>, commit),
                      &record.commit, sizeof(record.commit)) ||
        !flash_.read(offset, &verify, sizeof(verify)) ||
        memcmp(&verify, &record, sizeof(record)) != 0) return false;
    slot_ = int(slot);
    generation_ = record.generation;
    return true;
  }

private:
  Flash& flash_;
  int slot_ = -1;
  uint32_t generation_ = 0;
};
} // namespace DurableRecord
