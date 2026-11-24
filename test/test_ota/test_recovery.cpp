#include "RecoveryHash.h"
#include "RecoveryHealth.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <vector>

using namespace Recovery;
struct Reset {};
using Page = std::array<uint8_t, SECTOR>;
struct Flash {
  std::array<std::map<uint32_t, Page>, 4> pages;
  std::array<uint8_t, 8192> config;
  int operations = 0, resetAt = -1, prefix = 0;
  bool readFailed = false, writeFailed = false;
  bool failCommitReadback = false;
  bool failInternalWriteOnce = false;
  unsigned readFailures = 0;
  Flash() { config.fill(0xA5); }
  void progress() {}
  bool read(Region r, uint32_t offset, void* output, uint32_t size) {
    assert(bounds(r, offset, size));
    if (readFailed) return false;
    if (readFailures) { --readFailures; return false; }
    uint8_t* out = static_cast<uint8_t*>(output);
    auto& region = pages[unsigned(r)];
    for (uint32_t i = 0; i < size; ++i) {
      auto p = region.find((offset + i) / SECTOR);
      out[i] = p == region.end() ? 0xFF : p->second[(offset + i) % SECTOR];
    }
    return true;
  }
  void seed(Region r, uint32_t offset, const void* input, uint32_t size) {
    assert(bounds(r, offset, size));
    const uint8_t* bytes = static_cast<const uint8_t*>(input);
    for (uint32_t i = 0; i < size; ++i) {
      uint32_t index = (offset + i) / SECTOR;
      auto& region = pages[unsigned(r)];
      if (!region.count(index)) { Page page; page.fill(0xFF); region[index] = page; }
      region[index][(offset + i) % SECTOR] = bytes[i];
    }
  }
  bool mutate(Region r, uint32_t offset, const uint8_t* bytes, uint32_t size) {
    assert(bounds(r, offset, size) && r != Region::Staging);
    bool reset = operations++ == resetAt;
    uint32_t count = !reset || prefix == 4 ? size : prefix == 0 ? 0 :
                     prefix == 1 ? 1 : prefix == 2 ? size / 2 : size - 1;
    auto& region = pages[unsigned(r)];
    if (!bytes) {
      for (uint32_t i = 0; i < count;) {
        uint32_t page = (offset + i) / SECTOR, position = (offset + i) % SECTOR;
        uint32_t chunk = std::min(SECTOR - position, count - i);
        auto found = region.find(page);
        if (found != region.end()) {
          if (position == 0 && chunk == SECTOR) region.erase(found);
          else std::fill(found->second.begin() + position,
                         found->second.begin() + position + chunk, 0xFF);
        }
        i += chunk;
      }
      if (reset) throw Reset{};
      return true;
    }
    for (uint32_t i = 0; i < count; ++i) {
      uint32_t page = (offset + i) / SECTOR, position = (offset + i) % SECTOR;
      if (!region.count(page) && !bytes) continue;
      if (!region.count(page)) { Page p; p.fill(0xFF); region[page] = p; }
      if (bytes) {
        assert((region[page][position] & bytes[i]) == bytes[i]);
        region[page][position] &= bytes[i];
      } else region[page][position] = 0xFF;
    }
    if (reset) throw Reset{};
    return true;
  }
  bool write(Region r, uint32_t offset, const void* input, uint32_t size) {
    if (writeFailed) return false;
    if (r == Region::Internal && failInternalWriteOnce) {
      failInternalWriteOnce = false;
      return false;
    }
    bool ok = mutate(r, offset, static_cast<const uint8_t*>(input), size);
    if (ok && failCommitReadback && r == Region::Journal && size == 4 &&
        offset % SECTOR == offsetof(DurableRecord::Record<Payload>, commit)) {
      failCommitReadback = false;
      readFailures = 1;
    }
    return ok;
  }
  bool erase(Region r, uint32_t offset, uint32_t size) {
    assert(bounds(r, offset, size));
    if (r == Region::Internal) {
      assert(offset == 0 && size == APP_SIZE);
      const uint32_t sizes[] = {0x4000, 0x10000, 0x20000, 0x20000, 0x20000,
                                0x20000, 0x20000, 0x20000, 0x20000};
      for (uint32_t count : sizes) { mutate(r, offset, nullptr, count); offset += count; }
      assert(offset == APP_SIZE);
    } else {
      assert(offset % SECTOR == 0 && size % SECTOR == 0);
      for (uint32_t end = offset + size; offset < end; offset += SECTOR)
        mutate(r, offset, nullptr, SECTOR);
    }
    return true;
  }
  bool hash(Region r, uint32_t size, uint8_t output[32]) {
    return Recovery::hash(*this, r, size, output);
  }
};

std::vector<uint8_t> imageBytes(uint8_t fill, uint32_t length = 1024) {
  std::vector<uint8_t> bytes(length, fill);
  uint32_t vectors[2] = {0x20040000, APP_BASE + TAG_OFFSET + sizeof(Tag) + 1};
  memcpy(bytes.data(), vectors, sizeof(vectors));
  memcpy(bytes.data() + TAG_OFFSET, &IMAGE_TAG, sizeof(IMAGE_TAG));
  return bytes;
}
Image put(Flash& flash, Region region, const std::vector<uint8_t>& bytes) {
  flash.seed(region, 0, bytes.data(), bytes.size());
  Image image = {};
  image.length = bytes.size();
  assert(flash.hash(region, image.length, image.hash));
  return image;
}
struct Fixture {
  Flash flash;
  Image a, b;
  Fixture() {
    a = put(flash, Region::Internal, imageBytes(0xA1));
    b = put(flash, Region::Staging, imageBytes(0xB2));
    Engine<Flash> engine(flash);
    assert(engine.prepare(a));
    flash.operations = 0;
  }
};
Payload payload(Flash& flash) {
  Engine<Flash> engine(flash);
  assert(engine.load() == DurableRecord::Load::Loaded);
  return engine.status();
}
Image settle(Flash& flash, bool allowUnprovisioned = false) {
  flash.resetAt = -1;
  for (unsigned i = 0; i < 5; ++i) {
    Engine<Flash> engine(flash);
    Boot result = engine.boot();
    if (result == Boot::Stop) {
      assert(allowUnprovisioned && engine.error() == Error::Journal);
      return {};
    }
    if (result == Boot::Confirmed) {
      assert(engine.verify(Region::Internal, engine.status().active));
      assert(engine.verify(Region::Fallback, engine.status().fallback));
      return engine.status().active;
    }
  }
  assert(false && "Unbounded recovery or repeated rejected candidate");
  return {};
}
unsigned inject(const Flash& seed, const std::function<void(Flash&)>& scenario,
                const Image& a, const Image& b, bool provisioning = false) {
  Flash completed = seed;
  completed.operations = 0;
  scenario(completed);
  unsigned cases = 0;
  for (int operation = 0; operation < completed.operations; ++operation) {
    for (int prefix = 0; prefix < 5; ++prefix) {
      Flash reset = seed;
      reset.operations = 0; reset.resetAt = operation; reset.prefix = prefix;
      try { scenario(reset); assert(false && "Missed injection"); } catch (const Reset&) {}
      Engine<Flash> before(reset);
      bool committedNew = before.load() == DurableRecord::Load::Loaded &&
          (before.status().state == State::Promoting ||
           (before.status().state == State::Ready && same(before.status().active, b)));
      Image final = settle(reset, provisioning);
      assert(final.length == 0 || same(final, a) || same(final, b));
      if (committedNew) assert(same(final, b));
      assert(reset.config == seed.config);
      ++cases;
    }
  }
  return cases;
}

void testLifecycle() {
  Fixture fixture;
  Engine<Flash> engine(fixture.flash);
  assert(engine.request(fixture.b));
  for (unsigned attempt = 1; attempt <= 3; ++attempt) {
    Engine<Flash> reboot(fixture.flash);
    assert(reboot.boot() == Boot::Trial);
    assert(reboot.status().attempts == attempt);
    assert(!reboot.request(fixture.a));
  }
  Engine<Flash> rejected(fixture.flash);
  assert(rejected.boot() == Boot::Confirmed);
  assert(same(rejected.status().active, fixture.a));
  assert(rejected.status().reason == Reason::TrialExhausted);
  assert(rejected.boot() == Boot::Confirmed);

  assert(rejected.request(fixture.b));
  assert(rejected.boot() == Boot::Trial);
  assert(rejected.confirm());
  Engine<Flash> promote(fixture.flash);
  assert(promote.boot() == Boot::Confirmed);
  assert(same(promote.status().fallback, fixture.b));
  // The next authenticated transaction can reuse staging.
  fixture.flash.pages[unsigned(Region::Staging)].clear();
  put(fixture.flash, Region::Staging, imageBytes(0xA1));
  assert(promote.request(fixture.a));
  assert(promote.boot() == Boot::Trial);
}

void testRejectionAndCorruption() {
  Fixture f;
  Engine<Flash> engine(f.flash);
  Image oversized = f.b; oversized.length = IMAGE_LIMIT + 4;
  assert(!engine.request(oversized) && f.flash.operations == 0);
  Tag wrong = IMAGE_TAG; wrong.configSchema = 2;
  f.flash.seed(Region::Staging, TAG_OFFSET, &wrong, sizeof(wrong));
  Image wrongSchema = f.b;
  assert(f.flash.hash(Region::Staging, wrongSchema.length, wrongSchema.hash));
  assert(!engine.request(wrongSchema) && f.flash.operations == 0);
  put(f.flash, Region::Staging, imageBytes(0xB2));
  assert(engine.request(f.b));
  uint8_t bad = 0;
  f.flash.seed(Region::Staging, 800, &bad, 1);
  assert(engine.boot() == Boot::Confirmed);
  assert(same(engine.status().active, f.a));
  assert(engine.status().reason == Reason::CandidateCorrupt);
  f.flash.seed(Region::Internal, 800, &bad, 1);
  assert(engine.boot() == Boot::Confirmed);
  assert(engine.status().reason == Reason::InternalCorrupt);
  f.flash.seed(Region::Fallback, 800, &bad, 1);
  f.flash.operations = 0;
  assert(engine.boot() == Boot::Stop && f.flash.operations == 0);
  f.flash.readFailed = true;
  assert(engine.boot() == Boot::Stop);
  Fixture failed;
  Engine<Flash> writer(failed.flash);
  failed.flash.writeFailed = true;
  assert(!writer.request(failed.b) && writer.error() == Error::Flash);
  failed.flash.writeFailed = false;
  assert(writer.boot() == Boot::Confirmed && same(writer.status().active, failed.a));

  Fixture committed;
  Engine<Flash> confirmation(committed.flash);
  assert(confirmation.request(committed.b)); assert(confirmation.boot() == Boot::Trial);
  committed.flash.failCommitReadback = true;
  assert(!confirmation.confirm());
  committed.flash.failCommitReadback = false;
  Engine<Flash> afterReadFailure(committed.flash);
  assert(afterReadFailure.boot() == Boot::Confirmed);
  assert(same(afterReadFailure.status().active, committed.b));

  for (bool metadataFailure : {false, true}) {
    Fixture io;
    Engine<Flash> install(io.flash);
    assert(install.request(io.b));
    io.flash.failInternalWriteOnce = !metadataFailure;
    io.flash.failCommitReadback = metadataFailure;
    assert(install.boot() == Boot::Confirmed);
    assert(same(install.status().active, io.a));
    assert(install.status().reason == Reason::InterruptedInstall);
    assert(install.boot() == Boot::Confirmed);
  }

  for (bool metadataFailure : {false, true}) {
    Fixture trial;
    Engine<Flash> attempt(trial.flash);
    assert(attempt.request(trial.b)); assert(attempt.boot() == Boot::Trial);
    if (metadataFailure) trial.flash.failCommitReadback = true;
    else trial.flash.seed(Region::Internal, 800, &bad, 1);
    assert(attempt.boot() == Boot::Confirmed);
    assert(same(attempt.status().active, trial.a));
    assert(attempt.status().reason == (metadataFailure ?
        Reason::InterruptedInstall : Reason::InternalCorrupt));
  }

  Flash occupied;
  Image baseline = put(occupied, Region::Internal, imageBytes(0xA1));
  uint8_t claimed = 0;
  occupied.seed(Region::Fallback, IMAGE_LIMIT - 1, &claimed, 1);
  Engine<Flash> ownership(occupied);
  assert(!ownership.prepare(baseline) && occupied.operations == 0);
  assert(ownership.error() == Error::Ownership);
  occupied.pages[unsigned(Region::Fallback)].clear();
  occupied.seed(Region::Journal, 2 * SECTOR - 1, &claimed, 1);
  assert(!ownership.prepare(baseline) && occupied.operations == 0);

  Fixture invalidAbi;
  for (unsigned field : {0u, 2u, 4u, 5u}) {
    std::vector<uint8_t> bytes = imageBytes(0xB2);
    uint32_t value = 0;
    memcpy(bytes.data() + TAG_OFFSET + field * 4, &value, sizeof(value));
    Image incompatible = put(invalidAbi.flash, Region::Staging, bytes);
    Engine<Flash> reject(invalidAbi.flash);
    assert(!reject.request(incompatible) && invalidAbi.flash.operations == 0);
  }
  std::vector<uint8_t> invalid = imageBytes(0xB2);
  uint32_t reset = APP_BASE + 0x220;
  memcpy(invalid.data() + 4, &reset, sizeof(reset));
  Image evenVector = put(invalidAbi.flash, Region::Staging, invalid);
  Engine<Flash> rejectVector(invalidAbi.flash);
  assert(!rejectVector.request(evenVector) && invalidAbi.flash.operations == 0);
}

void testJournal() {
  Fixture fixture;
  Payload p = payload(fixture.flash);
  DurableRecord::Record<Payload> record = {};
  record.magic = DurableRecord::MAGIC; record.version = 1; record.length = sizeof(Payload);
  record.generation = 0xFFFFFFFF; record.payload = p;
  record.commit = DurableRecord::COMMITTED;
  auto checksum = [&]() {
    record.crc = DurableRecord::crc32(&record, offsetof(DurableRecord::Record<Payload>, crc));
  };
  checksum();
  fixture.flash.pages[unsigned(Region::Journal)].clear();
  fixture.flash.seed(Region::Journal, 0, &record, sizeof(record));
  Engine<Flash> rollover(fixture.flash);
  assert(rollover.request(fixture.b));
  assert(rollover.boot() == Boot::Trial);
  fixture.flash.pages[unsigned(Region::Journal)].clear();
  fixture.flash.seed(Region::Journal, 0, &record, sizeof(record));
  record.payload.reason = Reason::InternalCorrupt; checksum();
  fixture.flash.seed(Region::Journal, SECTOR, &record, sizeof(record));
  Engine<Flash> ambiguous(fixture.flash);
  assert(ambiguous.boot() == Boot::Stop);
  record.generation = 0x7FFFFFFF; checksum();
  fixture.flash.seed(Region::Journal, SECTOR, &record, sizeof(record));
  assert(ambiguous.boot() == Boot::Stop);
  fixture.flash.pages[unsigned(Region::Journal)].clear();
  assert(ambiguous.boot() == Boot::Stop);
}

int main() {
  testLifecycle(); testRejectionAndCorruption(); testJournal();
  assert(bootOptionsReady(0x0FF8AA00));
  assert(bootOptionsReady(0x0FF8AAFF));
  for (unsigned sector = 0; sector < 12; ++sector)
    assert(!bootOptionsReady(0x0FF8AA00 ^ (1u << (16 + sector))));
  assert(!bootOptionsReady(0x0FF85500));
  assert(!bootOptionsReady(0x0FF8CC00));
  Health health;
  assert(!health.observe(0xFFFFFF00, false));
  for (unsigned i = 0; i < 10; ++i) assert(!health.observe(0xFFFFFF00 + i * 50, true));
  assert(health.observe(0xFFFFFF00 + 1000, true));
  assert(!health.observe(1200, false));
  assert(!health.observe(1300, true));
  Fixture f;
  unsigned cases = inject(f.flash, [&](Flash& flash) {
    Engine<Flash> engine(flash);
    assert(engine.request(f.b));
    for (unsigned i = 0; i < 3; ++i) assert(engine.boot() == Boot::Trial);
    assert(engine.boot() == Boot::Confirmed);
  }, f.a, f.b);
  cases += inject(f.flash, [&](Flash& flash) {
    Engine<Flash> engine(flash);
    assert(engine.request(f.b)); assert(engine.boot() == Boot::Trial);
    assert(engine.confirm()); assert(engine.boot() == Boot::Confirmed);
  }, f.a, f.b);
  Flash empty;
  Image baseline = put(empty, Region::Internal, imageBytes(0xA1));
  cases += inject(empty, [&](Flash& flash) {
    Engine<Flash> engine(flash); assert(engine.prepare(baseline));
  }, baseline, baseline, true);
  // Reset recovery itself, including restoration and promotion.
  Fixture promoting;
  Engine<Flash> promotion(promoting.flash);
  assert(promotion.request(promoting.b)); assert(promotion.boot() == Boot::Trial);
  assert(promotion.confirm());
  cases += inject(promoting.flash, [](Flash& flash) {
    Engine<Flash> engine(flash); assert(engine.boot() == Boot::Confirmed);
  }, promoting.a, promoting.b);
  Fixture restoring;
  Payload restore = payload(restoring.flash);
  restore.state = State::Restoring; restore.reason = Reason::InterruptedInstall;
  Journal<Flash> journal{restoring.flash};
  DurableRecord::Store<Payload, Journal<Flash>> store(journal);
  Payload loaded;
  assert(store.load(loaded, validPayload) == DurableRecord::Load::Loaded);
  assert(store.save(restore));
  cases += inject(restoring.flash, [](Flash& flash) {
    Engine<Flash> engine(flash); assert(engine.boot() == Boot::Confirmed);
  }, restoring.a, restoring.b);
  Fixture maximum;
  Image large = put(maximum.flash, Region::Staging, imageBytes(0xC3, IMAGE_LIMIT));
  Engine<Flash> limit(maximum.flash);
  assert(limit.request(large)); assert(limit.boot() == Boot::Trial);
  assert(limit.confirm()); assert(limit.boot() == Boot::Confirmed);
  assert(same(limit.status().active, large));
  std::cout << "Recovery engine passed " << cases
            << " real-geometry operation-prefix reset cases, lifecycle, bounds and corruption tests\n";
}
