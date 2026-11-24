# Experimental source-built recovery bootloader

**Experimental recovery has passed controlled tests on one AZ3166**, including
automatic restoration after three actual watchdog resets and interrupted
installation/restoration/promotion. This is not broad hardware qualification or
a physical power-loss/brownout guarantee. Every bootstrap still requires explicit
approval and private recovery backups before programming boot or protection.
Existing `az3166_app` / `mxchip_az3166` builds still use
the vendor updater. The opt-in `az3166_recovery` application cannot activate
OTA under the vendor bootloader.

## Build, without programming

Use the existing Linux/amd64 GCC 6.3.1 toolchain and
`framework-arduinostm32mxchip` 2.0.0 headers. Native Apple Silicon is not a
supported replacement for that toolchain. In the supported build environment:

```console
python3 bootloader/build.py --core-dir /root/.platformio
pio run -e az3166_recovery
python3 test/test_ota/run_recovery.py
```

Outputs are `.pio/build/recovery_boot/boot.bin`, its ELF/map/build JSON, and
`.pio/build/az3166_recovery/firmware.bin`. `build.py` checks the boot size,
vectors, ABI tag, undefined symbols, absence of SDK/RTOS/allocation and
option-byte programming entry points. It has no upload operation.

After recovery bootstrap, use authenticated OTA with an `az3166_recovery` BIN.
Ordinary USB application programming does not update the journal and will be
treated as unexpected internal corruption, restoring the recorded fallback.
Vendor images are incompatible. Returning to the vendor bootloader requires a
separately approved ST-Link boot/protection restoration, not an ordinary upload.

**Never program generic ELF segments.** The legacy application linker/toolchain
can include load information outside its application slot. Any later approved
programming must use raw BINs with explicit addresses and verified readback.

## Ownership and image contract

| Region | Address | Size |
|---|---|---|
| Recovery component, internal sectors 0-2 | `0x08000000` | 48 KiB |
| Application update domain, sectors 3-11 | `0x0800C000` | `0xF4000` bytes |
| Candidate, external | `0x70000` | Existing staging partition |
| Configuration/authentication, unchanged | `0x164000`, `0x165000` | 4 KiB each |
| Recovery journal, external | `0x166000`, `0x167000` | 4 KiB each |
| Confirmed fallback, external | `0x168000` | 608 KiB (`0x98000` bytes) |

SDK parameters and RF firmware below staging are untouched. Do not mount or
format FILESYS: recovery now reserves its entire extent. The application
receiver never writes internal flash, SDK boot flags, or boot protection.
The recovery writer can erase only application sectors 3-11.

Application images are 544-622,592 bytes, divisible by four, linked at
`0x0800C000`. At offset 512 they contain the 32-byte `Recovery::IMAGE_TAG`
defined in `include/Recovery.h`: magic, ABI version, application address, image
cap, configuration schema, hardware-watchdog ABI, and two zero reserved words.
The boot image has its distinct tag at `0x08000200`.

All wire words are little-endian 32-bit values; static assertions enforce
structure sizes. The application linker reserves the tag after the existing
452-byte vector table and caps all loadable application data. The boot tag,
candidate/fallback SHA-256, vectors, lengths, and compatibility tag must agree.
Legacy raw BINs, including 1.1.1, are not automatic fallbacks or valid candidates.
SHA-256 is integrity checking, **not signing or publisher authentication**.

## Persistence and failure behavior

The shared production `Recovery::Engine` is used by boot code and application.
Journal records reuse commit-last CRC/readback storage; ambiguous equal or
half-range generations are rejected. An invalid journal never causes defaults,
guessed image lengths, or automatic reinitialization.

1. USB `B`, only in vendor bootstrap mode after local health/settings/password
   are valid, checks **all** proposed journal/fallback bytes are erased. It
   persists `Preparing` before changing fallback, copies the running
   recovery-aware baseline, and read-verifies it before `Ready`. An interrupted
   committed preparation can be resumed with `B` on the same baseline.
2. Authenticated upload/apply is allowed only with the new boot tag, protected
   boot sectors, and `Ready`. Apply verifies current, fallback, and candidate
   before durably recording `Pending`; it never calls the vendor installer.
3. Boot persists `Installing` before erasing the application. An interrupted
   install restores fallback, rather than retrying the candidate indefinitely.
   A detected install/attempt-recording failure also tries to restore fallback
   before declaring a fatal stop; an unrecoverable restoration error is latched.
4. Each of **three** trial attempts is committed before the application jump.
   Missing confirmation, early crashes, hangs, and resets consume attempts.
   Exhaustion restores fallback. A rejected transaction does not retry itself.
5. Local health durably records `Promoting`, then resets. Boot retains staging,
   verifies/copies the confirmed candidate as necessary, refreshes fallback,
   read-verifies it, and commits `Ready` before accepting another upload.
6. Interrupted restoration restarts the complete copy from validated fallback.
   Interrupted promotion retains a complete confirmed source in staging.

Configuration and passwords are never part of image copying. New firmware must
keep schema 1 compatible; a schema/watchdog ABI change requires a new separately
qualified boot contract, not just changing a browser limit.

Ordinary detected boot errors print `RECOVERY STOP <hex>` at 115200 baud and
record a diagnostic latch in reserved RAM `0x200001B0` through `0x200001BC`.
After an IWDG reset, a valid latch stops further destructive retries and reports
`RECOVERY latched fault`. Inspect with ST-Link; after repairing the cause, a
separately approved recovery can clear the first latch word to retry. Reset
injection during successful copying does not set this latch. Physical failures
that stall the CPU before it can record an error can still cause repeated
watchdog resets; ST-Link remains essential.

## Watchdog and local health

Boot starts IWDG with prescaler 256 and reload 4095 and supplies a standalone
SysTick timebase, without the RTOS. Feeding follows completed reads, verified
writes, or completed bounded erase steps, not an unconditional interrupt.
Handoff clears pending interrupts/SysTick, installs application vectors, and
transfers to its reset handler with IWDG running.

The application checks durable settings/authentication and sensor initialization
results, then exercises at least ten real main-loop iterations spanning one
second **before any network startup during trial**. Confirmation and promotion
therefore do not require WiFi, DNS, a web task, or MQTT. Failed local health
does not feed the watchdog. Normal main-loop/startup progress services IWDG;
the MQTT watchdog's web toggle does not disable it. The blocking startup `C`
menu is disabled in this opt-in build; configure via the authenticated web UI
after confirmation, or provision with vendor firmware before bootstrap.

OTA staging erases one external sector per operation so the main loop can access
durable configuration and continue making progress between erases. During
trial/promotion, staging is pinned regardless of the HTTP session's expiry.

## Hardware evidence and bootstrap gate

Controlled tests on one STM32F412 AZ3166 at approximately 3.3 V:

- **Verified:** actual NOR JEDEC `C22315`, Macronix MX25V1635F.
  The driver accepts only `C22015`, `C22315`, and `C22815`, checks block-protection
  bits, uses 24-bit single-line SPI, wakes deep power-down, and holds WP#/HOLD#
  high. The other accepted variants have not been tested on this board.
- **Verified:** QSPI PB1/PC11/PA6/PA7/PC4/PC5 uses **bank 2**, not
  bank 1. An SRAM-only probe identified and fixed the bank-selection error,
  then read the complete 2 MiB device. Configuration envelopes/CRCs validated.
  SDK console USART6 TX PA11, standalone initialization, source erase/program,
  application handoff and subsequent MQTT operation passed full-path tests.
- Manufacturer timing data was retrieved after earlier official-site transport
  failures: STM32F412xE/G DocID028087 Rev 7, Table 44 (page 112), specifies LSI
  17/32/47 kHz under its stated conditions. Register settings correspond to
  approximately **22.31-61.68 seconds** over those frequency limits, nominally
  32.768 seconds. Table 49 (page 117) gives x32 erase maxima of 0.5/1.1/2 seconds
  for 16/64/128 KiB sectors. The MX25V1635F datasheet gives 4 ms page-program
  and 240 ms sector-erase maxima. These are specification-derived bounds, not
  measured startup/watchdog/erase results; validate supply/temperature conditions.
  Worst-case startup/operation timing across operating conditions remains
  unqualified. Observed trial-jump to application recovery diagnostics was
  approximately 2.27 seconds; local confirmation and reset followed at roughly
  4.14 seconds. These UART observations are not exact instruction/feed timings.

| Physical test | Verified outcome |
|---|---|
| Raw recovery application under vendor loader | Application readback matched; all vendor boot bytes preserved; settings/authentication/sensors/MQTT healthy |
| Explicit USB `B` baseline provisioning | Complete blank ownership checked, fallback and journals read-verified before boot replacement |
| Source boot/protection bootstrap | Boot BIN and erased padding verified; options `0x0FFFAAED` to `0x0FF8AAED`; only sectors 0-2 protected, RDP/user fields preserved |
| Healthy authenticated OTA, repeated updates | One trial, durable confirmation/promotion and return to Ready; subsequent uploads reuse staging |
| Separate early-hang fixture, no IWDG feeds | Exactly three trial jumps, then automatic fallback restoration without reflashing; reason TrialExhausted and MQTT PUBACK |
| RAM-injected unhealthy local main-loop gate | Confirmation refused; three actual IWDG reset flags checked, then automatic fallback restoration and MQTT PUBACK |
| Install and restore copy interruptions | Reset after first 512 programmed bytes in each phase; automatic restoration, no candidate trial |
| Install and restore erase interruptions | FLASH BSY observed immediately before each controlled reset; automatic restoration |
| Promotion interruptions | Reset after first fallback-sector erase and again after first 512 fallback bytes; pinned staging permits completion |
| RAM-only unavailable SSID | 90-second outage after promotion, no rollback or unsolicited reset; normal MQTT after clearing the RAM injection by reset |
| RAM-only unresolvable broker name | 90-second DNS failure, no rollback or unsolicited reset; authentication/MQTT normal after reset |
| RAM-only unavailable MQTT endpoint | 190-second observation, existing MQTT delivery watchdog rebooted without rollback or another trial; authentication/MQTT normal afterward |

The three hang-banner to next-boot-banner intervals were approximately
**32.319, 32.269 and 32.284 seconds**. They demonstrate actual hardware watchdog
resets at the tested supply/environment, not guaranteed timeout limits.
Confirmation's last UART diagnostic may be lost at reset; durable Ready state,
attempt/reason fields and the subsequent confirmed boot establish success.
All fault fixtures and debugger breakpoints are separate from shipping images.

Final full 1 MiB internal and 2 MiB external readbacks verified:
protected boot bytes unchanged since bootstrap; exact production application
and fallback bytes; erased application/fallback tails; both recovery journal
CRCs; Ready/one attempt/reason 0 and no candidate pending; byte-identical
configuration/password records and RF firmware. After the final SRAM readback
probe, authenticated status was idle/Ready and MQTT PUBACK was observed.
Private original/final backups and detailed serial/debugger logs remain outside
git with restrictive permissions.

Tested raw BIN SHA-256 identities:

```text
boot.bin      10284 bytes  22ee6cb135776b2dc2eb9090110bc5f27b0296ee11d84499773232b925b90dbe
firmware.bin 281980 bytes  174efaba3c35cb13fc729a436552528ab1d30cae740239ab414627a205c30b20
```

Tests use controlled resets, not interrupted supply power. Torn physical word
programming, failed flash cells, brownouts and operating-condition extremes
remain untested. The initial local-health fixture armed its breakpoint too early
and was corrected/retested; final readbacks verify no fixture remains installed.

Before any bootstrap on another device:

- Refresh private mode-0600 internal/external/configuration/parameter backups
  and retain exact raw vendor boot/application restoration images.
  Preserve or clear pending vendor OTA metadata as appropriate before restoring
  the vendor bootloader; do not let stale metadata install an unintended image.
- Under separate approval, install a recovery-aware application baseline
  under the vendor loader, provision `P` if needed, and explicitly run USB `B`.
  Verify `ready`, `bootloader:false`, unchanged settings, and fallback readback.
- Under separate approval, program recovery sectors 0-2 and enable their
  write-protection bits while preserving all unrelated option fields. Do not
  enable readout protection, mass erase, or update recovery through OTA. Boot
  requires exactly sectors 0-2 protected, application sectors writable, and
  existing readout-protection level 0 (`AA`). An unexpected protection state
  fails before destructive work; do not silently undo other protections.
  Do not run an unprotected intermediate boot.
- Repeat healthy confirmation, three failures, early hangs, interruptions during
  install and restore, interrupted promotion, offline networks, repeated updates,
  settings/password persistence, and unchanged recovery bytes. Leave only a
  healthy confirmed production build. Fault hooks must not ship.

No deliberate power cuts, brownout guarantee, secure boot, or unattended
bootloader self-update is included. This board's bootstrap and controlled
nonvolatile recovery tests were explicitly approved; that approval is not
permission to program a different board or make additional option-byte changes.

## Verified build/host results

- Production engine: **2,815 real-geometry operation-prefix reset cases** under
  AddressSanitizer/UndefinedBehaviorSanitizer, plus lifecycle, corruption,
  compatibility, ownership, protection-state, readback-failure, and full-size
  image cases. Five selected byte prefixes are injected per modeled operation;
  this is not exhaustive arbitrary bit tearing or every hardware word write.
- Existing sanitized OTA/configuration/HTTP and MQTT suites pass. Browser
  checksum tests match Node crypto through both image limits.
- All three application profiles build with GCC 6.3.1. Recovery application:
  **281,980 BIN bytes**, versus its 622,592-byte cap. Both vendor aliases
  produce identical 278,932-byte BINs without the recovery ABI tag.
- Complete source bootloader: **10,284 BIN bytes** in its 49,152-byte reservation,
  108 bytes of static RAM, excluding stack. Expected vectors/tag, no unresolved
  symbols, and source/header/linker hashes checked. Unchanged ST HAL sources
  emit unused `Banks` parameter warnings; application SDK linkage retains its
  existing `.bss` alignment warnings.
- Actual recovery BIN length matches `__recovery_image_end`; vectors and tags
  match the contract. Both linkers reject oversized synthetic inputs. Inspection
  of the recovery application found one IWDG register-address reference, in its
  explicit progress feeder, not an SDK background watchdog.

These are software/build checks; physical evidence is listed separately above.

Read-only hardware preflight additionally verified 1 MiB internal flash and
3.3 V operation, made private complete internal/QSPI backups, checked the entire
proposed recovery reservation is blank, and compared internal flash
byte-for-byte before/after SRAM probing. Original 1.1.1 remains a private manual
ST-Link recovery artifact. It is not an ABI-compatible automatic fallback.

Timing/part references:
[STM32F412 manufacturer datasheet, university mirror](https://ie.isc.heia-fr.ch/documentation/assets/datasheets/stm32/stm32f412zg-datasheet.pdf),
[MX25V1635F manufacturer datasheet v1.6](https://www.macronix.com/Lists/Datasheet/Attachments/8999/MX25V1635F,%202.5V,%2016Mb,%20v1.6.pdf).
