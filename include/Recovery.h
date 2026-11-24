#pragma once

#include "DurableRecord.h"

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "Recovery ABI 1 requires little-endian wire representation"
#endif

namespace Recovery {
constexpr uint32_t APP_BASE = 0x0800C000;
constexpr uint32_t APP_SIZE = 0xF4000;
constexpr uint32_t IMAGE_LIMIT = 0x98000;
constexpr uint32_t TAG_OFFSET = 512;
constexpr uint32_t JOURNAL_BASE = 0x166000;
constexpr uint32_t FALLBACK_BASE = 0x168000;
constexpr uint32_t STAGING_BASE = 0x70000;
constexpr uint32_t SECTOR = 4096;
constexpr uint32_t TRIAL_LIMIT = 3;

inline bool bootOptionsReady(uint32_t options) {
  return (options & 0x0FFF0000u) == 0x0FF80000u &&
      (options & 0x0000FF00u) == 0x0000AA00u;
}

struct Tag {
  uint32_t magic, version, application, limit, configSchema, watchdogAbi, reserved[2];
};
constexpr Tag IMAGE_TAG = {0x415A5231, 1, APP_BASE, IMAGE_LIMIT, 1, 1, {0, 0}};
constexpr Tag BOOT_TAG = {0x415A4231, 1, APP_BASE, IMAGE_LIMIT, 1, 1, {0, 0}};
static_assert(sizeof(Tag) == 32, "Recovery tag wire format");

enum class Region { Internal, Staging, Fallback, Journal };
enum class State : uint32_t { Ready, Pending, Installing, Trial, Restoring, Promoting, Preparing };
enum class Reason : uint32_t { None, TrialExhausted, InterruptedInstall, InternalCorrupt, CandidateCorrupt };
enum class Error : uint32_t { None, Journal, InvalidImage, Flash, State, Ownership };
enum class Boot { Stop, Confirmed, Trial };

struct Image {
  uint32_t length;
  uint8_t hash[32];
};
struct Payload {
  uint32_t version;
  State state;
  uint32_t attempts;
  Reason reason;
  Image active, fallback, candidate;
};
static_assert(sizeof(Payload) == 124, "Recovery journal wire format");

inline bool same(const Image& a, const Image& b) {
  return a.length == b.length && memcmp(a.hash, b.hash, sizeof(a.hash)) == 0;
}
inline bool validLength(uint32_t length) {
  return length >= TAG_OFFSET + sizeof(Tag) && length <= IMAGE_LIMIT && length % 4 == 0;
}
inline bool validPayload(const Payload& p) {
  if (p.version != 1 || uint32_t(p.state) > uint32_t(State::Preparing) ||
      p.attempts > TRIAL_LIMIT || uint32_t(p.reason) > uint32_t(Reason::CandidateCorrupt) ||
      !validLength(p.active.length) || !validLength(p.fallback.length)) return false;
  if (p.state == State::Ready || p.state == State::Preparing)
    return same(p.active, p.fallback) && p.candidate.length == 0;
  if (p.state == State::Restoring)
    return same(p.active, p.fallback) &&
        (p.candidate.length == 0 || validLength(p.candidate.length));
  if (!validLength(p.candidate.length)) return false;
  if (p.state == State::Trial) return same(p.active, p.fallback);
  if (p.state == State::Promoting) return same(p.active, p.candidate) && p.attempts > 0;
  return same(p.active, p.fallback);
}
inline bool validVectors(const uint32_t vectors[2], uint32_t length) {
  return validLength(length) && vectors[0] > 0x200001C4 &&
      vectors[0] <= 0x20040000 && (vectors[0] & 7) == 0 &&
      (vectors[1] & 1) && (vectors[1] & ~1u) >= APP_BASE &&
      (vectors[1] & ~1u) < APP_BASE + length;
}
inline uint32_t capacity(Region region) {
  return region == Region::Journal ? 2 * SECTOR :
         region == Region::Fallback ? IMAGE_LIMIT : APP_SIZE;
}
inline bool bounds(Region region, uint32_t offset, uint32_t length) {
  return offset <= capacity(region) && length <= capacity(region) - offset;
}

template <typename Ops> struct Journal {
  Ops& ops;
  bool read(uint32_t offset, void* data, uint32_t size) {
    return bounds(Region::Journal, offset, size) && ops.read(Region::Journal, offset, data, size);
  }
  bool write(uint32_t offset, const void* data, uint32_t size) {
    return bounds(Region::Journal, offset, size) && ops.write(Region::Journal, offset, data, size);
  }
  bool erase(uint32_t offset, uint32_t size) {
    return offset % SECTOR == 0 && size == SECTOR &&
        bounds(Region::Journal, offset, size) && ops.erase(Region::Journal, offset, size);
  }
};

// Ops supplies bounded read/write/erase/hash/progress; no OS or allocator is required.
template <typename Ops> class Engine {
public:
  explicit Engine(Ops& ops) : ops_(ops), journal_{ops}, store_(journal_) {}
  Error error() const { return error_; }
  const Payload& status() const { return payload_; }

  DurableRecord::Load load() {
    error_ = Error::None;
    DurableRecord::Load result = store_.load(payload_, validPayload);
    if (result != DurableRecord::Load::Loaded) error_ = Error::Journal;
    return result;
  }

  bool verify(Region region, const Image& image) {
    uint32_t vectors[2];
    Tag tag;
    uint8_t hash[32];
    if (!validLength(image.length) ||
        !ops_.read(region, 0, vectors, sizeof(vectors)) ||
        !validVectors(vectors, image.length) ||
        !ops_.read(region, TAG_OFFSET, &tag, sizeof(tag)) ||
        memcmp(&tag, &IMAGE_TAG, sizeof(tag)) != 0 ||
        !ops_.hash(region, image.length, hash) ||
        memcmp(hash, image.hash, sizeof(hash)) != 0) return fail(Error::InvalidImage);
    return true;
  }

  bool prepare(const Image& baseline) {
    DurableRecord::Load loaded = load();
    if (loaded == DurableRecord::Load::Loaded) {
      if (payload_.state != State::Preparing || !same(payload_.active, baseline))
        return fail(Error::State);
    } else {
      if (loaded != DurableRecord::Load::Empty ||
          !erased(Region::Journal) || !erased(Region::Fallback))
        return fail(Error::Ownership);
      if (!verify(Region::Internal, baseline)) return false;
      payload_ = {};
      payload_.version = 1;
      payload_.state = State::Preparing;
      payload_.active = payload_.fallback = baseline;
      if (!save()) return false;
    }
    return finishPreparing();
  }

  bool request(const Image& candidate) {
    if (load() != DurableRecord::Load::Loaded || payload_.state != State::Ready)
      return fail(Error::State);
    if (!verify(Region::Internal, payload_.active) ||
        !verify(Region::Fallback, payload_.fallback) ||
        !verify(Region::Staging, candidate)) return false;
    payload_.candidate = candidate;
    payload_.state = State::Pending;
    payload_.attempts = 0;
    payload_.reason = Reason::None;
    return save();
  }

  bool confirm() {
    if (load() != DurableRecord::Load::Loaded || payload_.state != State::Trial ||
        payload_.attempts == 0) return fail(Error::State);
    if (!verify(Region::Internal, payload_.candidate) ||
        !verify(Region::Staging, payload_.candidate) ||
        !verify(Region::Fallback, payload_.fallback)) return false;
    payload_.active = payload_.candidate;
    payload_.state = State::Promoting;
    return save();
  }

  Boot boot() {
    if (load() != DurableRecord::Load::Loaded) return Boot::Stop;
    for (unsigned transitions = 0; transitions < 12; ++transitions) {
      switch (payload_.state) {
      case State::Ready:
        if (!verify(Region::Fallback, payload_.fallback)) return Boot::Stop;
        if (verify(Region::Internal, payload_.active)) return Boot::Confirmed;
        payload_.reason = Reason::InternalCorrupt;
        if (!restore()) return Boot::Stop;
        break;
      case State::Preparing:
        if (!finishPreparing()) return Boot::Stop;
        break;
      case State::Pending: {
        if (!verify(Region::Fallback, payload_.fallback)) return Boot::Stop;
        if (!verify(Region::Staging, payload_.candidate)) {
          payload_.reason = Reason::CandidateCorrupt;
          if (!restore()) return Boot::Stop;
          break;
        }
        payload_.state = State::Installing;
        bool installed = save() && copy(Region::Staging, Region::Internal, payload_.candidate);
        if (installed) {
          payload_.state = State::Trial;
          installed = save();
        }
        if (!installed) {
          payload_.reason = Reason::InterruptedInstall;
          if (!restore()) return Boot::Stop;
        }
        break;
      }
      case State::Installing:
        payload_.reason = Reason::InterruptedInstall;
        if (!restore()) return Boot::Stop;
        break;
      case State::Trial:
        if (!verify(Region::Fallback, payload_.fallback)) return Boot::Stop;
        if (!verify(Region::Internal, payload_.candidate)) {
          payload_.reason = Reason::InternalCorrupt;
          if (!restore()) return Boot::Stop;
        } else if (payload_.attempts == TRIAL_LIMIT) {
          payload_.reason = Reason::TrialExhausted;
          if (!restore()) return Boot::Stop;
        } else {
          ++payload_.attempts;
          if (!save()) {
            payload_.reason = Reason::InterruptedInstall;
            if (!restore()) return Boot::Stop;
            break;
          }
          return Boot::Trial;
        }
        break;
      case State::Restoring:
        if (!restore()) return Boot::Stop;
        break;
      case State::Promoting:
        if (!verify(Region::Staging, payload_.candidate)) return Boot::Stop;
        if (!verify(Region::Internal, payload_.candidate) &&
            !copy(Region::Staging, Region::Internal, payload_.candidate)) return Boot::Stop;
        if (!copy(Region::Staging, Region::Fallback, payload_.candidate)) return Boot::Stop;
        payload_.fallback = payload_.active = payload_.candidate;
        payload_.candidate = {};
        payload_.state = State::Ready;
        if (!save()) return Boot::Stop;
        break;
      }
    }
    fail(Error::State);
    return Boot::Stop;
  }

private:
  Ops& ops_;
  Journal<Ops> journal_;
  DurableRecord::Store<Payload, Journal<Ops>> store_;
  Payload payload_ = {};
  Error error_ = Error::None;

  bool fail(Error error) { error_ = error; return false; }
  bool save() {
    if (!validPayload(payload_)) return fail(Error::State);
    if (!store_.save(payload_)) return fail(Error::Flash);
    error_ = Error::None;
    ops_.progress();
    return true;
  }
  bool erased(Region region) {
    uint8_t bytes[128];
    for (uint32_t offset = 0; offset < capacity(region); offset += sizeof(bytes)) {
      if (!ops_.read(region, offset, bytes, sizeof(bytes))) return false;
      for (unsigned i = 0; i < sizeof(bytes); ++i) if (bytes[i] != 0xFF) return false;
      ops_.progress();
    }
    return true;
  }
  bool copy(Region source, Region target, const Image& image) {
    if (!verify(source, image)) return false;
    if (!ops_.erase(target, 0, capacity(target))) return fail(Error::Flash);
    uint8_t bytes[512], readback[512];
    for (uint32_t offset = 0; offset < image.length;) {
      uint32_t size = image.length - offset;
      if (size > sizeof(bytes)) size = sizeof(bytes);
      if (!ops_.read(source, offset, bytes, size) ||
          !ops_.write(target, offset, bytes, size) ||
          !ops_.read(target, offset, readback, size) ||
          memcmp(bytes, readback, size) != 0) return fail(Error::Flash);
      offset += size;
      ops_.progress();
    }
    return verify(target, image);
  }
  bool finishPreparing() {
    if (!copy(Region::Internal, Region::Fallback, payload_.active)) return false;
    payload_.state = State::Ready;
    return save();
  }
  bool restore() {
    if (!verify(Region::Fallback, payload_.fallback)) return false;
    payload_.state = State::Restoring;
    payload_.active = payload_.fallback;
    if (!save() || !copy(Region::Fallback, Region::Internal, payload_.fallback)) return false;
    payload_.candidate = {};
    payload_.state = State::Ready;
    return save();
  }
};
} // namespace Recovery
