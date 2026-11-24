# Manual AZ3166 OTA

**Scope:** this document's existing procedures describe the hardware-verified
vendor-bootloader builds (`az3166_app`, `mxchip_az3166`). A separate opt-in
`az3166_recovery` build implements experimental automatic rollback with a
source-built bootloader. Controlled one-board tests demonstrate three-attempt
watchdog rollback and interrupted recovery; broader hardware/power-loss
qualification is not claimed. Read [bootloader/README.md](bootloader/README.md); do not
mix images or apply the vendor bootstrap procedure to the new bootloader.

## Trust boundary

This is an operator-initiated updater for a **trusted LAN only**. HTTP Basic
authentication exposes the password to network observers. HTTP permits active
tampering. Firmware is deliberately **unsigned**: SHA-256 and the vendor CRC16
detect transfer/readback errors, not publisher identity. Never expose port 80
to the Internet or reuse this password elsewhere. The operator owns this
accepted transport/authenticity risk.

The vendor builds have **no automatic rollback, A/B slot, secure boot, or anti-downgrade**.
An authorized operator can install an incompatible image. Power failure during
bootloader copying may require ST-Link recovery. Keep stable power connected.

## Bootstrap and password

1. Retain the known-good raw application BIN and a private configuration backup.
2. Flash the OTA-capable BIN using ST-Link at **`0x0800C000`**, not `0x08000000`.
   Preserve the vendor bootloader. Do not program an ELF using automatic segment
   addresses with this legacy linker.
3. Check USB serial at 115200 baud for configuration migration and MQTT PUBACKs.
4. During normal operation send uppercase `P`, then a unique 16-64 character
   printable non-space ASCII password followed by Enter. Complete entry within
   30 seconds; disable local terminal echo. Firmware does not echo it.
5. Administrative pages now use browser Basic authentication: username `admin`,
   password as provisioned. Without a password, administration returns 503;
   there is no default or unauthenticated network bootstrap.

Change a forgotten password through the same physical USB `P` flow. Restart
clears browser sessions/CSRF tokens but does not erase the verifier. Credentials
are stored as salted PBKDF2-HMAC-SHA256 verifiers (16-byte random salt,
10,000 iterations), not plaintext. Authentication took approximately 1.9 seconds
on the tested board. Wrong credentials impose a two-second retry block.
The framework's prebuilt mbed TLS omits PBKDF2; the matching upstream 2.15.0
function is included with its Apache-2.0 attribution and license.

`/` and `/telemetry` remain public. `/setup`, `/control`, OTA and all other
administrative routes require authentication. Mutations require POST and the
boot-local random CSRF token. The supplied forms handle this with JavaScript.
WiFi passwords are no longer returned in setup HTML; blank preserves the current
password. Configuration fields are HTML-escaped. USB WiFi configuration also
suppresses password/default echo.

## Browser procedure

1. Build `pio run -e az3166_app` with the original GCC 6.3.1-compatible framework.
   The `mxchip_az3166` environment is an equivalent alias.
2. Open `http://<device-ip>/ota`, authenticate, and select
   `.pio/build/az3166_app/firmware.bin`.
3. Click **Upload BIN**. The browser computes SHA-256 and sends raw bytes.
   A tested checksum fallback supports ordinary LAN HTTP, where Web Crypto is
   unavailable. No cloud service or multipart parser is involved.
4. Wait for verification, then click **Apply and reboot** and confirm.
   Apply near the start of the maintenance window; expired updates must be
   uploaded again. **Discard** invalidates a staged image without installing it.
5. Wait for reboot, check the home-page version and serial MQTT PUBACKs.

Only raw application BINs linked at `0x0800C000` are supported: 8-999,424 bytes,
aligned SRAM initial stack pointer and Thumb reset vector inside the image.
ELF files, bootloader/combined images and mismatched hashes are rejected.
Vector checks do not prove application compatibility.

Receiving writes only external OTA staging. The device validates the declared
length, vectors, readback SHA-256, vendor CRC16 and durable configuration. Apply
is a separate authenticated action bound to the staged hash; it re-verifies
flash before asking the main loop to commit boot metadata. The vendor wrapper
discards persistence errors, so this implementation uses its underlying context
update and verifies both SDK parameter copies before reset.

Routes: `GET /ota`, `GET /ota/status`, `POST /ota/upload`,
`POST /ota/abort`, `POST /ota/apply`. Mutations use `X-CSRF-Token`; upload and
apply also use a 64-digit `X-Firmware-SHA256`. Upload needs `Content-Length` and
raw bytes. Header parsing rejects ambiguous lengths, transfer encoding, duplicate
authorization/CSRF/hash headers, unsupported `Expect`, and unbounded headers.
The web server processes one client at a time; stalled headers time out after
five seconds, stalled bodies after ten, and each OTA window after three minutes.
Keep-alive/pipelining and automatic URL downloads are not supported.

## Flash ownership and migration

| Region | Address/offset | Owner |
|---|---|---|
| Internal bootloader | `0x08000000` | Vendor; never written by receiver |
| Internal application | `0x0800C000`, `0xF4000` bytes | Bootloader installs here |
| Legacy internal settings | `0x080C0000` | Migration input only |
| External SDK parameters | `0x0000`, `0x1000` | Vendor persistence API only |
| External RF firmware | `0x2000`, `0x6E000` bytes | Vendor; untouched |
| External OTA staging | `0x70000`, `0xF4000` bytes | Updater |
| External configuration | `0x164000`, `0x165000`, 4 KiB each | This application |
| Remaining FILESYS space | `0x166000` onward | Reserved by opt-in recovery; unused by vendor build |

**Future filesystem support must exclude configuration and all enabled recovery
reservations (the entire FILESYS extent in recovery builds)**. Never
mount/format the entire partition. Runtime checks require the known partition
map; first migration also verifies both complete sectors are unused/erased.

Records have a version, payload length, generation, CRC32 and commit-last
marker. Saving erases/writes the inactive sector and read-verifies it, preserving
the previous committed record. Loading chooses the newest structurally and
semantically valid record, including generation rollover. A valid legacy record
is migrated without erasing its original. Corruption/I/O failures do not silently
restore defaults. If available, legacy settings support explicitly logged
read-only recovery with OTA and external writes locked; otherwise ST-Link
recovery is required.

The stock bootloader erases the **entire application partition**, including the
legacy settings sector, even for a small update. Do not downgrade to firmware
that only understands internal settings without restoring/reprovisioning them.

## Failure and recovery

An incomplete/bad upload never commits an apply flag. A reboot discards the
RAM-only staging session, so leftover staged bytes cannot be applied without a
new upload and verification. Wrong-image/stale apply requests are denied.
The three-minute OTA window temporarily defers MQTT-watchdog resets; normal
monitoring resumes with a fresh grace period after abort/failure/expiry and
preserves its prior enabled/disabled setting.

If installed firmware cannot boot, use ST-Link with a known-good **BIN** at
`0x0800C000`. Preserve the bootloader and external configuration. Firmware 1.0.3
cannot read the new external records; restoring it may also require privately
restoring legacy settings or using its USB configuration flow. Backups contain
WiFi credentials and must never be published. If the bootloader itself is
damaged, restore the matching vendor bootloader separately before the app.

## Verification scope

Host sanitizer tests cover redundant records, interrupted commits, corruption,
read failure, semantic fallback, generation rollover, legacy validation, bounded
fragmented HTTP, duplicate/invalid headers, timeouts, image vectors and hex
parsing. Node verifies browser fallback SHA-256 against its crypto implementation
through the maximum image size. Existing MQTT protocol/watchdog tests still pass.

Hardware testing exercises authentication/CSRF denial, password rate limiting,
ELF/empty/oversized/hash-mismatched images, interrupted and stalled uploads,
wrong-image/stale apply, maintenance locking, browser upload/apply, and
configuration/password/MQTT operation after reboot. See `test/README`.
On the connected AZ3166, ST-Link bootstrapped 1.1.0 and three browser-driven
installations ran 1.1.1 (including the final implementation). The final
end-to-end test passed with unchanged settings, persistent authentication and
two post-boot MQTT PUBACKs. A staged update expired after three minutes while
telemetry continued, preserved a previously disabled watchdog, and denied stale
apply. The watchdog was restored to enabled afterward.

ST-Link readback matched the final application BIN exactly and the bootloader
matched its pre-test checksum. The legacy settings bytes at `0x080C0000` were
erased, demonstrating that successful restarts now depend on external
configuration rather than the old copy. A final controlled reset loaded external
settings, accepted the stored password and produced two more MQTT PUBACKs.
Both build aliases produced the same BIN: 278,596 bytes, with 46,484 bytes of
static RAM reported by PlatformIO.

The vendor-path tests did not exercise post-install automatic recovery.
The separate recovery build's controlled watchdog/interruption results are
recorded in [bootloader/README.md](bootloader/README.md). Neither test campaign
demonstrates physical power-loss/brownout recovery or failed physical flash cells.
