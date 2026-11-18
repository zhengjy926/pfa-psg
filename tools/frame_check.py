#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Fill and verify CRC fields in ECSP example frames.

Frame layout (see spec/ecsp-general.md §3.1):
    FA | LEN_L LEN_H | 产品地址 | 命令码 | 板卡地址 | 载荷... | CRC_L CRC_H | 0D
CRC16-MODBUS covers LEN_L .. last payload byte.

Example lines carry ``{{CRC}}`` where the two CRC bytes belong:

    FA | 0A 00 | 03 | 30 | 02 | 00 01 | {{CRC}} | 0D

Usage:
    python tools/frame_check.py docs/spec/*.md      # fill + verify
    python tools/frame_check.py --check docs/**/*.md # verify only
"""
import re
import sys
from pathlib import Path

TOKEN = re.compile(r"\b[0-9A-Fa-f]{2}\b")
PLACEHOLDER = "{{CRC}}"


def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def frame_body(line: str) -> str:
    """Extract just the frame bytes, ignoring trailing prose.

    Examples are written either inside backticks or followed by a full-width
    parenthesis; either way the prose after the frame must not be parsed as
    hexadecimal bytes.
    """
    match = re.search(r"`([^`]*\{\{CRC\}\}[^`]*)`", line)
    if match:
        return match.group(1)
    return line.split("（", 1)[0]


def analyse(line: str):
    """Return (left, right, frame_bytes) or raise ValueError."""
    body = frame_body(line)
    left, right = body.split(PLACEHOLDER, 1)
    left_bytes = bytes(int(t, 16) for t in TOKEN.findall(left))
    right_bytes = bytes(int(t, 16) for t in TOKEN.findall(right))
    if len(left_bytes) < 6:
        raise ValueError("frame shorter than 6 bytes before CRC")
    if left_bytes[0] != 0xFA:
        raise ValueError("missing 0xFA frame head")
    if right_bytes != b"\x0d":
        raise ValueError("frame must end with 0x0D after CRC")
    length = left_bytes[1] | (left_bytes[2] << 8)
    total = len(left_bytes) + 2 + len(right_bytes)
    if length != total:
        raise ValueError(f"LEN field says {length}, frame has {total} bytes")
    return left, right, left_bytes, right_bytes


def verify_filled(line: str):
    """Recompute the CRC of an already-filled frame. Returns None if not a frame."""
    idx = line.find("FA")
    if idx < 0:
        return None
    tail = line[idx:]
    if PLACEHOLDER in tail:
        return None
    tokens = TOKEN.findall(tail)
    if len(tokens) < 4:
        return None
    values = [int(t, 16) for t in tokens]
    if values[0] != 0xFA:
        return None
    length = values[1] | (values[2] << 8)
    if length > len(values):
        return None  # symbolic payload / unexpanded block: no CRC value exists
    raw = bytes(values[:length])
    if raw[-1] != 0x0D:
        raise ValueError("frame must end with 0x0D")
    length = raw[1] | (raw[2] << 8)
    if length != len(raw):
        raise ValueError(f"LEN says {length}, frame has {len(raw)} bytes")
    claimed = raw[-3] | (raw[-2] << 8)
    expected = crc16_modbus(raw[1:-3])
    if claimed != expected:
        raise ValueError(f"CRC {claimed:04X} != computed {expected:04X}")
    return claimed


def process(path: Path, check_only: bool):
    text = path.read_text(encoding="utf-8")
    if PLACEHOLDER not in text:
        return 0, 0
    out_lines, filled, errors = [], 0, 0
    for line in text.splitlines(keepends=True):
        if PLACEHOLDER not in line:
            out_lines.append(line)
            continue
        tick = re.search(r"`([^`]*)`", line)
        if tick and not tick.group(1).strip().startswith("FA"):
            out_lines.append(line)  # prose that mentions the placeholder, not a frame
            continue
        try:
            left, right, left_bytes, right_bytes = analyse(line)
        except ValueError as exc:
            errors += 1
            print(f"  ERROR {path.name}: {exc}\n        {line.strip()}")
            out_lines.append(line)
            continue
        crc = crc16_modbus(left_bytes[1:])
        full = left_bytes + bytes((crc & 0xFF, crc >> 8)) + right_bytes
        declared = full[1] | (full[2] << 8)
        if declared != len(full):
            errors += 1
            print(f"  ERROR {path.name}: LEN says {declared}, frame has {len(full)} bytes")
            out_lines.append(line)
            continue
        if check_only:
            out_lines.append(line)
        else:
            out_lines.append(line.replace(PLACEHOLDER, f"{crc & 0xFF:02X} {crc >> 8:02X}", 1))
            filled += 1
    if not check_only and filled:
        path.write_text("".join(out_lines), encoding="utf-8")
    return filled, errors


def main(argv):
    mode = "fill"
    if argv and argv[0] in ("--check", "--verify"):
        mode = argv[0][2:]
        argv = argv[1:]
    args = argv
    if not args:
        print(__doc__)
        return 2

    if mode == "verify":
        checked = errors = 0
        for pattern in args:
            paths = sorted(Path().glob(pattern)) if any(c in pattern for c in "*?") else [Path(pattern)]
            for path in paths:
                for line in path.read_text(encoding="utf-8").splitlines():
                    try:
                        result = verify_filled(line)
                    except ValueError as exc:
                        errors += 1
                        print(f"  ERROR {path.name}: {exc}\n        {line.strip()}")
                        continue
                    if result is not None:
                        checked += 1
        print(f"TOTAL: verified {checked}, errors {errors}")
        return 1 if errors else 0

    check_only = mode == "check"
    total_filled = total_errors = 0
    for pattern in args:
        for path in sorted(Path().glob(pattern)) if any(c in pattern for c in "*?") else [Path(pattern)]:
            filled, errors = process(path, check_only)
            if filled or errors:
                print(f"{path}: {'checked' if check_only else 'filled'} {filled}, errors {errors}")
            total_filled += filled
            total_errors += errors
    print(f"TOTAL: {'checked' if check_only else 'filled'} {total_filled}, errors {total_errors}")
    return 1 if total_errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
