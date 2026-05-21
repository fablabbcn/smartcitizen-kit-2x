#!/usr/bin/env python3
"""
flash_test.py — End-to-end flash storage validation for SmartCitizen Kit 2.x

Builds on the serial infrastructure already in tools/sck.py and
tools/serialtools/serialdevice.py rather than reimplementing device
discovery, connection management, or the console handshake.

Firmware prerequisites (on branch fix/flash-countsectgroups-guard):
  flash -read  <addr_hex> <len>           — stream flash bytes over serial
  flash -write <addr_hex> <hex_payload>   — write raw bytes (area must be erased)
  flash -erase <sector>                   — erase one 4 KB sector

All three require the device to be in shell mode (shell -on).

Usage examples:
  python flash_test.py read  --addr 0x001000 --len 128
  python flash_test.py write --addr 0x004003 --data DEADBEEF
  python flash_test.py erase --sector 4
  python flash_test.py test  --sector 4 roundtrip
  python flash_test.py test  --sector 4 zero-group
  python flash_test.py test  --sector 4 all
"""

import argparse
import json
import os
import random
import re
import struct
import sys
import time
import uuid
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import List, Optional

# ---------------------------------------------------------------------------
# Path bootstrap — mirrors make.py convention: run from repo root
# ---------------------------------------------------------------------------
_REPO_ROOT = Path(__file__).resolve().parent
sys.path.append(str(_REPO_ROOT / 'tools'))   # tools/ is a git submodule

try:
    from sck import sck
except ImportError as exc:
    sys.exit(f"Cannot import sck module from tools/: {exc}\n"
             "Run from the repo root and ensure 'git submodule update --init tools'.")

# ---------------------------------------------------------------------------
# Flash geometry constants (must match firmware SckList.h)
# ---------------------------------------------------------------------------
FLASH_SIZE       = 8_388_608   # 8 MB
SECTOR_SIZE      = 4_096       # 4 KB
TOTAL_SECTORS    = FLASH_SIZE // SECTOR_SIZE          # 2048
DATA_SECTORS     = TOTAL_SECTORS - 8                  # 2040 (reserved at end)

# Sector layout offsets (SckList.h SectorAddr / GroupAddr enums)
SECTOR_STATE_OFF = 0x00   # 1 byte: 0xFF = empty, 0x00 = used
SECTOR_NET_OFF   = 0x01   # 1 byte: publish flag
SECTOR_SD_OFF    = 0x02   # 1 byte: publish flag
GROUP_DATA_OFF   = 0x03   # first group starts here

SECTOR_USED      = 0x00
NOT_PUBLISHED    = 0xFF


# ---------------------------------------------------------------------------
# CRC-16/CCITT-FALSE (poly=0x1021, init=0xFFFF) — mirrors firmware _crc16Update
# ---------------------------------------------------------------------------
def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if (crc & 0x8000) else (crc << 1)
        crc &= 0xFFFF
    return crc


# ---------------------------------------------------------------------------
# Result types
# ---------------------------------------------------------------------------
@dataclass
class ReadResult:
    data: bytes
    expected_crc: int       # CRC reported by firmware
    computed_crc: int       # CRC computed locally over returned data
    crc_ok: bool
    raw_lines: List[str] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        return self.crc_ok and len(self.data) > 0


@dataclass
class WriteResult:
    bytes_written: int
    expected_crc: int
    input_crc: int          # CRC of the bytes we sent
    crc_ok: bool
    raw_lines: List[str] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        return self.crc_ok and self.bytes_written > 0


@dataclass
class EraseResult:
    sector: int
    addr: int
    success: bool
    raw_lines: List[str] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        return self.success


@dataclass
class TestResult:
    name: str
    passed: bool
    detail: str
    duration_s: float
    extra: dict = field(default_factory=dict)


# ---------------------------------------------------------------------------
# FlashProtocol — thin serial layer over the three firmware commands
# ---------------------------------------------------------------------------
class FlashProtocol:
    """
    Wraps a connected sck instance and exposes typed flash operations.

    The sck instance must already be open (begin() + update_serial() called).
    """

    # Per-line read timeout. 2 s is generous even for a 4 KB streaming read.
    LINE_TIMEOUT_S  = 2.0
    # Overall command timeout (covers firmware erase latency ~20 ms/sector).
    CMD_TIMEOUT_S   = 10.0

    def __init__(self, kit: sck):
        self.kit = kit
        # Raise per-line timeout so streaming reads don't cut off mid-burst.
        kit.serialPort.timeout = self.LINE_TIMEOUT_S

    # ------------------------------------------------------------------
    # Low-level I/O
    # ------------------------------------------------------------------
    def _send(self, command: str, timeout: float = None) -> List[str]:
        """
        Send one text command and collect response lines until OK/ERROR or timeout.

        Skips the echoed command line (firmware echoes each character).
        """
        if timeout is None:
            timeout = self.CMD_TIMEOUT_S

        port = self.kit.serialPort
        port.reset_input_buffer()
        port.write(f'{command}\r\n'.encode())

        lines: List[str] = []
        deadline = time.monotonic() + timeout
        cmd_bare = command.strip().lower()

        while time.monotonic() < deadline:
            raw = port.readline()
            if not raw:
                # readline timed out — if we already have a terminal line, done
                if lines and self._is_terminal(lines[-1]):
                    break
                continue

            line = raw.decode('utf-8', errors='replace').strip()
            if not line:
                continue
            # Skip the echoed command and the SCK prompt
            if line.lower() == cmd_bare or line.startswith('SCK >'):
                continue
            lines.append(line)
            if self._is_terminal(line):
                break

        return lines

    @staticmethod
    def _is_terminal(line: str) -> bool:
        return line.startswith('OK ') or line.startswith('ERROR')

    # ------------------------------------------------------------------
    # Shell mode
    # ------------------------------------------------------------------
    def shell_on(self) -> bool:
        lines = self._send('shell -on', timeout=5.0)
        return any('shell' in l.lower() or 'on' in l.lower() for l in lines) \
               or True   # firmware may not echo confirmation; assume success

    def shell_off(self) -> bool:
        lines = self._send('shell -off', timeout=5.0)
        return True

    # ------------------------------------------------------------------
    # flash -read
    # ------------------------------------------------------------------
    def read(self, addr: int, length: int) -> ReadResult:
        """
        Read `length` bytes starting at `addr`.

        Response protocol (firmware):
          DATA <ADDR_HEX> <HEX_BYTES>   (64 bytes per line)
          OK <TOTAL_BYTES> CRC16:0x<CRC_HEX>
        """
        lines = self._send(f'flash -read {addr:#08x} {length}',
                           timeout=max(self.CMD_TIMEOUT_S, length / 512 + 3))

        data = bytearray()
        expected_crc = 0
        error = False

        for line in lines:
            if line.startswith('DATA '):
                parts = line.split(' ', 2)
                if len(parts) == 3:
                    try:
                        data.extend(bytes.fromhex(parts[2]))
                    except ValueError:
                        pass  # malformed hex — will show as CRC mismatch
            elif line.startswith('OK '):
                m = re.search(r'CRC16:0x([0-9A-Fa-f]{4})', line)
                if m:
                    expected_crc = int(m.group(1), 16)
            elif line.startswith('ERROR'):
                error = True

        if error:
            return ReadResult(bytes(data), expected_crc, 0, False, lines)

        data_bytes = bytes(data)
        computed = crc16(data_bytes)
        return ReadResult(data_bytes, expected_crc, computed, computed == expected_crc, lines)

    # ------------------------------------------------------------------
    # flash -write
    # ------------------------------------------------------------------
    def write(self, addr: int, data: bytes) -> WriteResult:
        """
        Write `data` bytes at flash address `addr`.

        The area must be erased first. Max 256 bytes per call.

        Response protocol (firmware):
          OK <BYTES_WRITTEN> CRC16:0x<CRC_HEX>
        """
        if len(data) > 256:
            raise ValueError("Max 256 bytes per write call")

        hex_payload = data.hex().upper()
        input_crc = crc16(data)
        lines = self._send(f'flash -write {addr:#08x} {hex_payload}')

        bytes_written = 0
        expected_crc  = 0
        error = False

        for line in lines:
            if line.startswith('OK '):
                m_bytes = re.search(r'OK (\d+)', line)
                m_crc   = re.search(r'CRC16:0x([0-9A-Fa-f]{4})', line)
                if m_bytes:
                    bytes_written = int(m_bytes.group(1))
                if m_crc:
                    expected_crc = int(m_crc.group(1), 16)
            elif line.startswith('ERROR'):
                error = True

        if error:
            return WriteResult(0, expected_crc, input_crc, False, lines)

        crc_ok = (expected_crc == input_crc) and (bytes_written == len(data))
        return WriteResult(bytes_written, expected_crc, input_crc, crc_ok, lines)

    # ------------------------------------------------------------------
    # flash -erase
    # ------------------------------------------------------------------
    def erase(self, sector: int) -> EraseResult:
        """
        Erase one 4 KB sector by sector number (0–2047).

        Response protocol (firmware):
          OK sector:<N> addr:0x<ADDR_HEX>
        """
        lines = self._send(f'flash -erase {sector}',
                           timeout=5.0)   # erase takes ~20 ms on W25Q64

        success = False
        erase_addr = sector * SECTOR_SIZE

        for line in lines:
            if line.startswith('OK '):
                success = True
                m = re.search(r'addr:0x([0-9A-Fa-f]+)', line)
                if m:
                    erase_addr = int(m.group(1), 16)
            elif line.startswith('ERROR'):
                success = False

        return EraseResult(sector, erase_addr, success, lines)

    # ------------------------------------------------------------------
    # flash -sector (existing command — used to trigger _countSectGroups)
    # ------------------------------------------------------------------
    def sector_info(self, sector: int) -> List[str]:
        """Run the existing `flash -sector` command and return its output lines."""
        return self._send(f'flash -sector {sector}', timeout=10.0)


# ---------------------------------------------------------------------------
# Session logger
# ---------------------------------------------------------------------------
class FlashSession:
    """
    Creates timestamped log files for a test run.

    Host log  : flash_test_YYYYMMDD_HHMMSS_<id>_host.log   (human-readable)
    Result log: flash_test_YYYYMMDD_HHMMSS_<id>_results.json
    """

    def __init__(self, log_dir: Path = Path('.')):
        ts = datetime.now().strftime('%Y%m%d_%H%M%S')
        self.session_id = uuid.uuid4().hex[:8]
        stem = f'flash_test_{ts}_{self.session_id}'
        log_dir.mkdir(parents=True, exist_ok=True)
        self.host_log_path    = log_dir / f'{stem}_host.log'
        self.results_log_path = log_dir / f'{stem}_results.json'
        self._results: List[dict] = []
        self._fh = open(self.host_log_path, 'w')
        self.log(f'Session {self.session_id} started at {datetime.now().isoformat()}')

    def log(self, msg: str):
        ts = datetime.now().strftime('%H:%M:%S.%f')[:-3]
        line = f'[{ts}] {msg}'
        print(line)
        self._fh.write(line + '\n')
        self._fh.flush()

    def record(self, result: TestResult):
        self._results.append({
            'name':       result.name,
            'passed':     result.passed,
            'detail':     result.detail,
            'duration_s': round(result.duration_s, 3),
            'extra':      result.extra,
        })
        status = '✓ PASS' if result.passed else '✗ FAIL'
        self.log(f'{status}  {result.name}  ({result.duration_s:.2f}s)  — {result.detail}')

    def metadata(self, **kwargs):
        self.log('--- Session metadata ---')
        for k, v in kwargs.items():
            self.log(f'  {k}: {v}')

    def close(self, summary: bool = True):
        if summary:
            total  = len(self._results)
            passed = sum(1 for r in self._results if r['passed'])
            self.log(f'--- {passed}/{total} tests passed ---')
        with open(self.results_log_path, 'w') as f:
            json.dump({'session': self.session_id,
                       'results': self._results}, f, indent=2)
        self.log(f'Results written to {self.results_log_path}')
        self._fh.close()


# ---------------------------------------------------------------------------
# FlashTester — test scenarios
# ---------------------------------------------------------------------------
class FlashTester:
    """
    High-level test scenarios built on FlashProtocol.

    Each test method returns a TestResult. Call run_all() to execute the
    full suite and receive a list of TestResult objects.
    """

    def __init__(self, proto: FlashProtocol, session: FlashSession,
                 sector: int = 4, seed: Optional[int] = None):
        self.proto   = proto
        self.session = session
        self.sector  = sector
        self.seed    = seed if seed is not None else random.randint(0, 0xFFFFFFFF)
        self.rng     = random.Random(self.seed)
        session.log(f'FlashTester init — sector={sector} seed={self.seed:#010x}')

    def _sector_base(self) -> int:
        return self.sector * SECTOR_SIZE

    # ------------------------------------------------------------------
    # Individual tests
    # ------------------------------------------------------------------
    def test_erase_and_verify(self) -> TestResult:
        """Erase sector and confirm all bytes read back as 0xFF."""
        name  = 'erase_and_verify'
        start = time.monotonic()
        base  = self._sector_base()

        self.session.log(f'[{name}] Erasing sector {self.sector} (addr={base:#08x})')
        erase = self.proto.erase(self.sector)
        if not erase.ok:
            return TestResult(name, False,
                              f'Erase failed: {erase.raw_lines}',
                              time.monotonic() - start)

        self.session.log(f'[{name}] Reading back {SECTOR_SIZE} bytes')
        read = self.proto.read(base, SECTOR_SIZE)
        if not read.ok:
            return TestResult(name, False,
                              f'Read failed: {read.raw_lines}',
                              time.monotonic() - start)

        non_ff = sum(1 for b in read.data if b != 0xFF)
        passed = non_ff == 0
        detail = 'all bytes 0xFF' if passed else f'{non_ff}/{SECTOR_SIZE} bytes not 0xFF'
        return TestResult(name, passed, detail, time.monotonic() - start,
                          {'non_ff_count': non_ff})

    def test_roundtrip(self) -> TestResult:
        """
        Erase sector, write structured pseudo-random payload, read back,
        verify exact byte match and CRC agreement.

        Payload structure per 32-byte record:
          [0:4]   magic 0xDEADBEEF
          [4:8]   record index (uint32 LE)
          [8:12]  session seed (uint32 LE)
          [12:16] CRC32 of previous 12 bytes (uint32 LE)
          [16:32] 16 random bytes
        """
        name  = 'roundtrip'
        start = time.monotonic()
        base  = self._sector_base()

        # Erase first
        self.session.log(f'[{name}] Erasing sector {self.sector}')
        erase = self.proto.erase(self.sector)
        if not erase.ok:
            return TestResult(name, False, 'Erase failed', time.monotonic() - start)

        # Build a 128-byte payload (4 records × 32 bytes, fits in one write call)
        records = []
        for i in range(4):
            magic  = struct.pack('<I', 0xDEADBEEF)
            idx    = struct.pack('<I', i)
            seed_b = struct.pack('<I', self.seed)
            header = magic + idx + seed_b
            crc_b  = struct.pack('<I', crc16(header))
            rand_b = bytes(self.rng.randint(0, 255) for _ in range(16))
            records.append(header + crc_b + rand_b)

        payload = b''.join(records)
        write_addr = base + GROUP_DATA_OFF   # byte 3: skip sector state bytes

        self.session.log(f'[{name}] Writing {len(payload)} bytes at {write_addr:#08x}')
        write = self.proto.write(write_addr, payload)
        if not write.ok:
            return TestResult(name, False,
                              f'Write failed (lines={write.raw_lines})',
                              time.monotonic() - start)

        # Read back the same range
        self.session.log(f'[{name}] Reading back {len(payload)} bytes')
        read = self.proto.read(write_addr, len(payload))
        if not read.ok:
            return TestResult(name, False,
                              f'Read failed or CRC mismatch: {read.raw_lines}',
                              time.monotonic() - start)

        # Byte-exact comparison
        mismatch = [(i, payload[i], read.data[i])
                    for i in range(len(payload)) if payload[i] != read.data[i]]
        passed = len(mismatch) == 0
        detail = (f'exact match ({len(payload)} bytes, CRC 0x{read.computed_crc:04X})'
                  if passed else
                  f'{len(mismatch)} byte mismatches; first at offset {mismatch[0][0]}: '
                  f'expected {mismatch[0][1]:#04x} got {mismatch[0][2]:#04x}')

        return TestResult(name, passed, detail, time.monotonic() - start,
                          {'bytes': len(payload), 'mismatches': len(mismatch),
                           'write_crc': f'0x{write.expected_crc:04X}',
                           'read_crc':  f'0x{read.computed_crc:04X}'})

    def test_zero_group_guard(self) -> TestResult:
        """
        Reproduce the exact condition fixed in PR #112 and verify the guard works.

        Steps:
          1. Erase sector.
          2. Mark sector as SECTOR_USED (write 0x00 at byte 0).
          3. Write 0x0000 at GROUP_DATA_OFF (byte 3) — a zero-length group entry.
          4. Trigger `flash -sector N` which calls _countSectGroups.
          5. Verify a response arrives within timeout (firmware didn't hang).
          6. Confirm no ERROR in the response.
        """
        name  = 'zero_group_guard'
        start = time.monotonic()
        base  = self._sector_base()

        # Step 1: erase
        self.session.log(f'[{name}] Erasing sector {self.sector}')
        erase = self.proto.erase(self.sector)
        if not erase.ok:
            return TestResult(name, False, 'Erase failed', time.monotonic() - start)

        # Step 2: mark sector as SECTOR_USED (0x00) so _countSectGroups walks it
        self.session.log(f'[{name}] Writing SECTOR_USED marker at {base:#08x}')
        mark = self.proto.write(base, bytes([SECTOR_USED]))
        if not mark.ok:
            return TestResult(name, False,
                              f'Could not write sector marker: {mark.raw_lines}',
                              time.monotonic() - start)

        # Step 3: plant zero-length group (0x0000) at first group slot
        group_addr = base + GROUP_DATA_OFF
        self.session.log(f'[{name}] Planting zero-length group at {group_addr:#08x}')
        plant = self.proto.write(group_addr, b'\x00\x00')
        if not plant.ok:
            return TestResult(name, False,
                              f'Could not plant zero-group: {plant.raw_lines}',
                              time.monotonic() - start)

        # Step 4 + 5: trigger sector scan with a hard timeout
        self.session.log(f'[{name}] Running `flash -sector {self.sector}` '
                         f'(should return, not hang)')
        scan_start = time.monotonic()
        info_lines = self.proto.sector_info(self.sector)
        scan_duration = time.monotonic() - scan_start

        self.session.log(f'[{name}] Sector scan returned in {scan_duration:.2f}s')
        for l in info_lines:
            self.session.log(f'[{name}]   > {l}')

        # Step 6: assert response is non-empty and not an ERROR
        if not info_lines:
            return TestResult(name, False,
                              'No response from firmware — possible hang',
                              time.monotonic() - start)

        has_error = any('ERROR' in l for l in info_lines)
        if has_error:
            return TestResult(name, False,
                              f'Firmware returned error: {info_lines}',
                              time.monotonic() - start)

        # A hang would exceed FlashProtocol.CMD_TIMEOUT_S; quick return proves the guard fired
        passed = scan_duration < self.proto.CMD_TIMEOUT_S * 0.9
        detail = (f'guard fired — scan returned in {scan_duration:.3f}s, no hang'
                  if passed else
                  f'scan took {scan_duration:.1f}s — possible hang or very slow flash')

        return TestResult(name, passed, detail, time.monotonic() - start,
                          {'scan_duration_s': round(scan_duration, 3),
                           'info_lines': info_lines})

    def test_boundary_read(self) -> TestResult:
        """Read the last 64 bytes of the data area (just before reserved sectors)."""
        name  = 'boundary_read'
        start = time.monotonic()
        # Last byte of data sector range
        end_addr  = DATA_SECTORS * SECTOR_SIZE   # exclusive
        read_addr = end_addr - 64

        self.session.log(f'[{name}] Reading 64 bytes at flash boundary ({read_addr:#08x})')
        read = self.proto.read(read_addr, 64)

        passed = read.crc_ok and len(read.data) == 64
        detail = (f'64 bytes read, CRC OK (0x{read.computed_crc:04X})'
                  if passed else
                  f'read failed or CRC mismatch: {read.raw_lines}')
        return TestResult(name, passed, detail, time.monotonic() - start,
                          {'addr': f'{read_addr:#08x}', 'crc': f'0x{read.computed_crc:04X}'})

    def test_error_no_shell(self) -> TestResult:
        """
        Verify that flash -read is rejected when shell mode is off.
        Temporarily disables shell mode, sends the command, re-enables shell.
        """
        name  = 'error_no_shell'
        start = time.monotonic()

        self.session.log(f'[{name}] Disabling shell mode')
        self.proto.shell_off()
        time.sleep(0.3)

        lines = self.proto._send('flash -read 0x000000 8', timeout=5.0)
        self.session.log(f'[{name}] Response: {lines}')

        # Re-enable shell before asserting so subsequent tests still work
        self.proto.shell_on()
        time.sleep(0.3)

        passed = any('shell mode' in l.lower() or 'ERROR' in l for l in lines)
        detail = ('firmware correctly rejected command outside shell mode'
                  if passed else
                  f'expected shell-mode rejection, got: {lines}')
        return TestResult(name, passed, detail, time.monotonic() - start,
                          {'response': lines})

    def test_error_bad_payload(self) -> TestResult:
        """Send an odd-length hex payload to flash -write and expect an ERROR."""
        name  = 'error_bad_payload'
        start = time.monotonic()

        self.session.log(f'[{name}] Sending odd-length hex payload')
        lines = self.proto._send(
            f'flash -write {self._sector_base() + GROUP_DATA_OFF:#08x} ABC',
            timeout=5.0)
        self.session.log(f'[{name}] Response: {lines}')

        passed = any('ERROR' in l for l in lines)
        detail = ('firmware correctly rejected odd-length payload'
                  if passed else
                  f'expected ERROR, got: {lines}')
        return TestResult(name, passed, detail, time.monotonic() - start)

    # ------------------------------------------------------------------
    # Suite runner
    # ------------------------------------------------------------------
    def run_all(self) -> List[TestResult]:
        """
        Run the full validation suite and return all results.

        Order matters: erase_and_verify and roundtrip leave the sector erased
        for zero_group_guard which needs a clean sector.
        """
        results = []
        suite = [
            self.test_erase_and_verify,
            self.test_roundtrip,
            self.test_zero_group_guard,
            self.test_boundary_read,
            self.test_error_no_shell,
            self.test_error_bad_payload,
        ]
        for test_fn in suite:
            self.session.log(f'Running {test_fn.__name__}')
            try:
                result = test_fn()
            except Exception as exc:
                result = TestResult(test_fn.__name__, False,
                                    f'Unhandled exception: {exc}', 0.0)
            self.session.record(result)
            results.append(result)
        return results


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description='SmartCitizen Kit flash storage validation tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__)

    p.add_argument('--port', default=None,
                   help='Serial port (auto-detected if omitted)')
    p.add_argument('--force', action='store_true',
                   help='Force port even if not detected as SCK')
    p.add_argument('--sector', type=int, default=4,
                   help='Sector number used for read/write/erase/test commands (default 4)')
    p.add_argument('--log-dir', type=Path, default=Path('.'),
                   help='Directory for session log files (default: current dir)')
    p.add_argument('--seed', type=lambda x: int(x, 0), default=None,
                   help='PRNG seed for reproducible payloads (default: random)')
    p.add_argument('--verbose', type=int, choices=[0, 1, 2], default=2,
                   help='Verbosity: 0=silent 1=errors 2=all (default 2)')

    sub = p.add_subparsers(dest='command', required=True)

    # read
    r = sub.add_parser('read', help='Read flash bytes and print as hex')
    r.add_argument('--addr', required=True,
                   type=lambda x: int(x, 0), help='Start address (hex or decimal)')
    r.add_argument('--len', dest='length', required=True,
                   type=int, help='Number of bytes to read (1–4096)')

    # write
    w = sub.add_parser('write', help='Write hex bytes to flash (area must be erased)')
    w.add_argument('--addr', required=True,
                   type=lambda x: int(x, 0), help='Start address (hex or decimal)')
    w.add_argument('--data', required=True,
                   help='Hex payload string, no spaces, no 0x prefix (e.g. DEADBEEF)')

    # erase
    e = sub.add_parser('erase', help='Erase a 4 KB flash sector')
    e.add_argument('--sector', type=int, dest='erase_sector',
                   help='Sector number (overrides global --sector)')

    # test
    t = sub.add_parser('test', help='Run validation test scenarios')
    t.add_argument('scenario',
                   choices=['roundtrip', 'zero-group', 'boundary',
                            'error-cases', 'all'],
                   help='Which scenario to run')

    return p


def connect(port: Optional[str], force: bool, verbose: int) -> sck:
    """Find, open and verify a SmartCitizen Kit serial connection."""
    kit = sck(check_pio_sam=False, check_pio_esp=False, verbose=verbose)
    print(f'Searching for SCK on {"auto" if port is None else port}...')
    if not kit.begin(port=port, force=force):
        sys.exit('Device not found. Check USB connection or specify --port.')
    kit.update_serial(timeout_ser=2.0)   # longer timeout for flash streaming
    if not kit.checkConsole():
        sys.exit('Device found but not responding to console. Reset the kit?')
    print(f'Connected on {kit.serialPort_name} (S/N {kit.serialNumber})')
    return kit


def main():
    parser = build_parser()
    args   = parser.parse_args()

    kit     = connect(args.port, args.force, args.verbose)
    session = FlashSession(log_dir=args.log_dir)

    # Log session metadata
    try:
        git_hash = __import__('subprocess').check_output(
            ['git', 'rev-parse', '--short', 'HEAD'],
            stderr=__import__('subprocess').DEVNULL).decode().strip()
    except Exception:
        git_hash = 'unknown'

    session.metadata(
        port=kit.serialPort_name,
        serial_number=kit.serialNumber,
        git_hash=git_hash,
        sector=args.sector,
        seed=f'{args.seed:#010x}' if args.seed else 'random',
        flash_size_bytes=FLASH_SIZE,
        sector_size_bytes=SECTOR_SIZE,
    )

    proto = FlashProtocol(kit)

    # All raw commands and tests need shell mode
    session.log('Enabling shell mode')
    proto.shell_on()
    time.sleep(0.2)

    try:
        if args.command == 'read':
            result = proto.read(args.addr, args.length)
            if result.ok:
                print(f'\n{result.data.hex().upper()}')
                print(f'OK  {len(result.data)} bytes  CRC16=0x{result.computed_crc:04X}')
            else:
                sys.exit(f'Read failed: {result.raw_lines}')

        elif args.command == 'write':
            try:
                data = bytes.fromhex(args.data)
            except ValueError:
                sys.exit('--data must be a valid hex string (even length, no spaces)')
            result = proto.write(args.addr, data)
            if result.ok:
                print(f'OK  {result.bytes_written} bytes written  CRC16=0x{result.expected_crc:04X}')
            else:
                sys.exit(f'Write failed: {result.raw_lines}')

        elif args.command == 'erase':
            sector = args.erase_sector if args.erase_sector is not None else args.sector
            result = proto.erase(sector)
            if result.ok:
                print(f'OK  sector {result.sector} erased (addr={result.addr:#08x})')
            else:
                sys.exit(f'Erase failed: {result.raw_lines}')

        elif args.command == 'test':
            tester = FlashTester(proto, session, sector=args.sector, seed=args.seed)

            scenario_map = {
                'roundtrip':   [tester.test_erase_and_verify, tester.test_roundtrip],
                'zero-group':  [tester.test_zero_group_guard],
                'boundary':    [tester.test_boundary_read],
                'error-cases': [tester.test_error_no_shell, tester.test_error_bad_payload],
                'all':         None,   # handled below
            }

            if args.scenario == 'all':
                results = tester.run_all()
            else:
                results = []
                for fn in scenario_map[args.scenario]:
                    session.log(f'Running {fn.__name__}')
                    try:
                        r = fn()
                    except Exception as exc:
                        r = TestResult(fn.__name__, False,
                                       f'Unhandled exception: {exc}', 0.0)
                    session.record(r)
                    results.append(r)

            failed = [r for r in results if not r.passed]
            sys.exit(0 if not failed else 1)

    finally:
        proto.shell_off()
        session.close()
        kit.end()


if __name__ == '__main__':
    main()
