# Flash Storage — Architecture, Test Interface, and Validation Protocol

This document covers the SPI flash storage layer of the SmartCitizen Kit 2.x SAM firmware and the tooling used to validate it end-to-end over the serial interface.

---

## 1. Flash Architecture

### Hardware

| Parameter | Value |
|---|---|
| Chip | W25Q64JV (or compatible) |
| Total capacity | 8 MB (8,388,608 bytes) |
| Sector size | 4,096 bytes |
| Page size | 256 bytes |
| Interface | SPI @ 133 MHz |
| Erase granularity | Sector (4 KB minimum) |
| Write constraint | Bits can only transition 1 → 0; erase resets all bits to 1 |

### Sector allocation

```
Sector 0                                            Sector 2047
│◄──────────────── 2040 data sectors ───────────────►│◄── 8 reserved ──►│
│  0x000000                            0x7F7FFF       │  0x7F8000        │
```

- **Data sectors:** 0–2039 (`SCKLIST_SECTOR_NUM = 2040`)
- **Reserved sectors:** 2040–2047 (last 32 KB, not managed by `SckList`)
- The write cursor (`_currSector`) advances forward; when it reaches the end it wraps to sector 0 and erases it

### Sector layout (per 4 KB sector)

```
Offset  Size  Field
──────  ────  ─────────────────────────────────────────
0x00    1 B   SECTOR_STATE  (0xFF = empty, 0x00 = used)
0x01    1 B   SECTOR_NET    (0xFF = unpublished, 0x00 = published)
0x02    1 B   SECTOR_SD     (0xFF = not saved,   0x00 = saved)
0x03    …     Group 0, Group 1, … (variable-length, written sequentially)
```

### Group layout (per reading group)

Each group is a variable-length record. Minimum size is 11 bytes.

```
Offset  Size  Field
──────  ────  ─────────────────────────────────────────
+0x00   2 B   GROUP_SIZE    (total group length including this field)
+0x02   1 B   GROUP_NET     (0xFF = unpublished, 0x00 = published)
+0x03   1 B   GROUP_SD      (0xFF = not saved,   0x00 = saved)
+0x04   4 B   GROUP_TIME    (Unix epoch, uint32 LE)
+0x08   …     Readings (one or more variable-length reading entries)
```

Each reading entry inside a group:

```
Offset  Size  Field
──────  ────  ─────────────────────────────────────────
+0x00   1 B   readingSize   (total entry size including this byte)
+0x01   1 B   sensorType    (SensorType enum)
+0x02   …     ASCII value string
```

### Sentinel values

| Value | Meaning |
|---|---|
| `GROUP_SIZE == 0xFFFF` | End of written data in this sector |
| `GROUP_SIZE == 0x0000` | Corrupted entry (power-loss mid-write) — sector walker must not advance |
| `SECTOR_STATE == 0xFF` | Sector never written (erased) |
| `readingSize == 0x00` | Corrupt reading entry — reading walker must not advance |

### Sector-walking invariants

Every function that traverses a sector by following `address += groupSize` must guard both sentinels:

```cpp
uint16_t groupSize = flash.readWord(address);
if (groupSize == 0xFFFF) break;          // end of written data
if (groupSize == 0)      return false;   // corrupted — would infinite-loop
```

This applies to `_getGrpAddr`, `_countSectGroups` (both overloads), and `_getSectFreeSpace`. All four are guarded as of this branch.

### Wrap-around behaviour

When `_currSector` reaches `SCKLIST_SECTOR_NUM - 1`, the firmware wraps to sector 0 and erases it before writing. This implements a simple circular log with no explicit garbage collection. There is no wear-levelling beyond this ring buffer.

### Power-loss resilience

The firmware writes `GROUP_SIZE` first. A power loss between starting a write and completing `GROUP_SIZE` leaves either `0x0000` (partially written) or `0xFFFF` (not yet written). Both are treated as end-of-data by the sector walker. Readings within a partial group are therefore silently discarded on the next scan, which is the intended behaviour.

### CRC / integrity

The firmware does not write per-group CRCs. Integrity is validated by:
- The `0xFFFF` / `0x0000` sentinels on `GROUP_SIZE`
- The `0x00` / `0xFF` sentinels on publish flags
- The epoch-0 guard in `saveGroup()` (rejects groups with timestamp 0)
- The `_write()` bounds check (validates metadata write addresses against the live write cursor)

---

## 2. Serial Flash Test Interface

Three serial commands expose raw flash access for validation. They are extensions of the existing `flash` command handler and follow the same text-based protocol as all other serial commands.

**Prerequisite:** all three commands require shell mode to be active. Shell mode suspends sensor saves, preventing concurrent flash access during testing.

```
shell -on       ← required before any of the commands below
shell -off      ← restore normal operation when done
```

### `flash -read <addr_hex> <len>`

Reads `len` bytes (1–4096) starting at `addr_hex` and streams them to SerialUSB.

**Output format (machine-parseable):**

```
DATA <ADDR_HEX> <HEX_BYTES>     ← one line per 64-byte chunk, no spaces in hex
DATA <ADDR_HEX> <HEX_BYTES>
…
OK <TOTAL_BYTES> CRC16:0x<CRC>  ← CRC-16/CCITT-FALSE over all returned bytes
```

**Error:**
```
ERROR <reason>
```

**Example:**
```
SCK > flash -read 0x004000 128
DATA 004000 FFFFFFFFFFFFFFFFFFFFFFFF...
DATA 004040 FFFFFFFFFFFFFFFFFFFFFFFF...
OK 128 CRC16:0x1D0F
```

**Notes:**
- `addr_hex` may be given with or without `0x` prefix
- `len` is decimal
- Maximum `len` is 4,096 (one full sector); larger reads require multiple calls

---

### `flash -write <addr_hex> <hex_payload>`

Writes raw bytes to flash at `addr_hex`. The payload is a hex string (no `0x` prefix, no spaces, even number of characters).

**The target area must be erased first.** Flash bits can only transition 1 → 0. Writing to an un-erased area produces undefined results.

**Output format:**
```
OK <BYTES_WRITTEN> CRC16:0x<CRC>   ← CRC of the bytes actually written
```

**Error:**
```
ERROR <reason>
```

**Example:**
```
SCK > flash -write 0x004003 0000
OK 2 CRC16:0x29B1
```

**Constraints:**
- Maximum payload: 256 bytes (512 hex characters) per call
- For larger writes, split into 256-byte chunks aligned to 256-byte page boundaries

---

### `flash -erase <sector>`

Erases a single 4 KB sector by sector number.

**Output format:**
```
OK sector:<N> addr:0x<ADDR_HEX>
```

**Error:**
```
ERROR <reason>
```

**Example:**
```
SCK > flash -erase 4
OK sector:4 addr:0x004000
```

**Notes:**
- Valid range: 0–2047 (including the 8 reserved sectors)
- Erase time on W25Q64: ~20 ms typical, ~400 ms worst case; the command blocks until complete

---

### CRC algorithm

All three commands use **CRC-16/CCITT-FALSE**:
- Polynomial: `0x1021`
- Initial value: `0xFFFF`
- Input/output reflection: none

Reference vector: `crc16(b'123456789') == 0x29B1`

The Python tool (`flash_test.py`) implements the same algorithm and verifies it against this vector on import.

---

## 3. Python Validation Tool

`flash_test.py` lives in the [`smartcitizen-tools`](https://github.com/fablabbcn/smartcitizen-tools) repository (also available as the `tools/` submodule of this repo). It imports `sck.py` and `serialtools/serialdevice.py` from the same repo for device discovery and serial connection management.

Source: [`flash_test.py` in smartcitizen-tools](https://github.com/fablabbcn/smartcitizen-tools/blob/master/flash_test.py)

### Requirements

```
pyserial>=3.5    # already in tools/requirements_serial_only.txt
```

No additional dependencies beyond the Python standard library.

### Setup

```bash
# Option A: use the tools submodule inside smartcitizen-kit-2x
git submodule update --init tools
cd tools
python flash_test.py --help

# Option B: clone smartcitizen-tools directly
git clone https://github.com/fablabbcn/smartcitizen-tools.git
cd smartcitizen-tools
python flash_test.py --help
```

### Command reference

```
python flash_test.py [--port PORT] [--sector N] [--seed INT] [--log-dir DIR]
                     <command> [options]
```

| Command | Description |
|---|---|
| `read --addr ADDR --len N` | Read N bytes from ADDR, print as hex |
| `write --addr ADDR --data HEX` | Write hex bytes to ADDR (area must be erased) |
| `erase [--sector N]` | Erase sector N (default from global `--sector`) |
| `test roundtrip` | Erase → structured write → read back → byte-exact diff |
| `test zero-group` | Inject zero-length group, trigger sector scan, verify no hang |
| `test boundary` | Read last 64 bytes of data area |
| `test error-cases` | Verify firmware rejects commands outside shell mode and bad payloads |
| `test all` | Run full suite; exits 0 on pass, 1 on any failure |

### Session output

Each run creates two files in `--log-dir` (default: current directory):

```
flash_test_YYYYMMDD_HHMMSS_<session-id>_host.log      human-readable timestamped log
flash_test_YYYYMMDD_HHMMSS_<session-id>_results.json  machine-readable pass/fail per test
```

Session metadata logged on start: serial port, serial number, git hash, sector, PRNG seed, flash geometry.

### Reproducibility

Pass `--seed INT` to fix the PRNG used for payload generation. The same seed produces the same byte sequence across runs, enabling exact comparison of logs from different sessions or firmware builds.

---

## 4. Test Protocol

Run from the repository root with the kit connected over USB.

### Prerequisite check

```bash
# Run from the smartcitizen-tools directory (or tools/ submodule)
python flash_test.py read --addr 0x000000 --len 8
```

Expected: `OK 8 CRC16:0x????` (any CRC). Confirms serial link, shell mode, and read command all work. If this fails, stop and diagnose the connection before running any write tests.

---

### Test suite

#### T1 — Erase and blank verify

```bash
python flash_test.py test --sector 4 roundtrip
```

*What it exercises:* `flash -erase` followed by `flash -read`; verifies all bytes return as `0xFF` after erase.

*Pass condition:* `erase_and_verify PASS` in output.

---

#### T2 — Structured write / read round-trip

Included in `test roundtrip` above.

*What it exercises:* `flash -write` with a structured 128-byte payload (magic header, record index, session seed, CRC32, random bytes), then `flash -read` of the same range. Byte-exact diff performed host-side. CRC16 from firmware compared against host-computed value.

*Pass condition:* zero mismatches, CRC16 agreement.

---

#### T3 — Zero-length group guard (targets PR fix)

```bash
python flash_test.py test --sector 4 zero-group
```

*What it exercises:* The `groupSize == 0` corruption guards in `_countSectGroups` and `_getGrpAddr`.

*Procedure:*
1. Erase sector
2. Write `0x00` at sector base (`SECTOR_STATE = SECTOR_USED`)
3. Write `0x0000` at sector base + 3 (zero-length group at first group slot)
4. Run `flash -sector N` — this calls `_countSectGroups` on the sector
5. Assert response arrives within timeout (firmware did not hang)
6. Assert no ERROR in response

*Pass condition:* `zero_group_guard PASS`. A hang would cause the test to time out after 10 s and report `FAIL`.

*Without the fix:* `address += 0` never advances; the `while (address < endAddr)` loop runs indefinitely, blocking the main loop.

---

#### T4 — Boundary read

```bash
python flash_test.py test --sector 4 boundary
```

*What it exercises:* `flash -read` at the last 64 bytes of the data sector area (just before the 8 reserved sectors).

*Pass condition:* 64 bytes returned, CRC16 agrees.

---

#### T5 — Error rejection

```bash
python flash_test.py test --sector 4 error-cases
```

*What it exercises:*
- Firmware rejects `flash -read` when shell mode is off
- Firmware rejects `flash -write` with an odd-length hex payload

*Pass condition:* both cases return `ERROR` lines from firmware.

---

#### T6 — Full suite

```bash
python flash_test.py test --sector 4 all --seed 0xDEADBEEF --log-dir ./test-logs
```

Runs T1–T5 in sequence. Exits 0 if all pass, 1 if any fail. CI-friendly.

**Important:** T3 (zero-group) writes a corrupted sector state. The test automatically erases the sector before injecting the corruption, but after the test the sector contains test data. Run `flash -format` or `python flash_test.py erase --sector 4` if you need to restore normal operation on that sector.

---

### Soak / stress scenarios

These are manual, not automated by `flash_test.py` yet.

| Scenario | Procedure |
|---|---|
| Repeated sector overwrite | Loop: `erase --sector N`, `write`, `read`, verify. Run 1,000+ iterations. Monitor CRC failures. |
| Full-flash fill | `flash -format`, start normal sensor capture, monitor `flash` (info) until `0 free sectors`. Verify no data loss. |
| Power-loss resilience | Run `flash -write`, physically disconnect power mid-transfer, reconnect, verify firmware recovers cleanly (no hang on boot, `testFlash` passes). |
| Serial reconnect | Unplug USB mid-`flash -read`. Reconnect. Verify `flash_test.py` reconnects (device discovery re-runs via `sck.update_serial()`) and the interrupted read can be retried. |

---

## 5. Known Limitations

| Limitation | Detail |
|---|---|
| No per-group CRC in firmware | Corruption within a group body is not detectable by the firmware itself; only the host-side diff in `flash_test.py` catches it |
| 256-byte write limit per call | Imposed by `outBuff` size (512 bytes) and hex encoding. Multi-page writes require multiple `flash -write` calls |
| No wear tracking | The ring-buffer wrap-around erases and rewrites sector 0 most frequently. No wear counter is maintained |
| `flash -write` requires pre-erase | The command does not verify the target area is erased; writing to non-erased flash silently ANDs bits and will not be caught until `flash -read` compares |
| Shell mode is manual | `flash_test.py` sends `shell -on` at startup and `shell -off` at exit, but an interrupted run may leave the kit in shell mode. Reset or send `shell -off` manually if the kit stops publishing |
