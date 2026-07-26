# FRAM_I2C_FM24CL16B — Agent Guide

## What this is

Arduino library (single header `FRAM_I2C_FM24CL16B.h`) for Cypress FM24CL16B 16-Kbit FRAM over I2C. Depends on `Wire.h` and `Stream.h` (Arduino built-ins).

## Architecture

- **`FRAM_FM24CL16B`** — main class. Write: 16-byte chunks with auto page-boundary split. Read: 32-byte chunks with auto page-boundary split. All ops handle 256-byte I2C page boundaries transparently.
- **`FRAM_Manager`** — manages up to 8 chips in one address space (16 KB total). Auto-detects devices via `begin()`.
- **Multi-chip I2C addressing**: base address `0x50`, upper 3 address bits select chip page: `_deviceAddr | ((memAddr >> 8) & 0x07)`.
- **`_baseAddress`** — all constructors accept an offset; all read/write/clear/fill/dump auto-offset by it. `clear()`/`fill()` calculate `relativeAddr = currentAddr - _baseAddress` before delegating to `write()`.

## Key quirks

- `yield()` called in write/read loops (for ESP8266/ESP32 watchdog).
- `delayMicroseconds(250)` between multi-page write chunks.
- `clear()`/`fill()` use 16-byte staging blocks, not single-byte writes.
- `read()` uses `Wire.requestFrom()` with `stop=true`.
- `dump()` prints hex+ASCII via `Stream&` (defaults to `Serial`).
- No `library.properties` — this is a non-standard layout (single header at root, no `src/`).
- No test framework, no CI, no build system. For Arduino IDE / PlatformIO, include the `.h` and link against the standard Arduino libraries.

## Error handling

All read/write operations return `fram_error_t`. Check return values; `FRAM_OK` (0) means success.

## Version history (from header)

- v2.4 — fixed `uint16_t pageBoundary` to avoid overflow, cross-page read handling, `_baseAddress` compat in clear/fill/dump, `yield()` for ESP8266.
