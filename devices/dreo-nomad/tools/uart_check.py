#!/usr/bin/env python3
"""Probe HeFi UART (115200). Finds CP2102 ports on macOS if PORT not set."""

from __future__ import annotations

import glob
import sys
import time

import serial

BAUD = 115200


def make_pkt(seq: int, cmd: int, data: bytes = b"") -> bytes:
    p = bytes([0x55, 0xAA, 0x00, seq & 0xFF, cmd, 0x00, len(data) >> 8, len(data) & 0xFF]) + data
    return p + bytes([sum(p) & 0xFF])


def probe(port: str) -> bool:
    print(f"Trying {port} @ {BAUD} ...")
    try:
        ser = serial.Serial(port, BAUD, timeout=0.3)
    except serial.SerialException as e:
        print(f"  open failed: {e}")
        return False
    ser.reset_input_buffer()
    for i in range(10):
        ser.write(make_pkt(i, 0x00, b"\x01"))
        time.sleep(0.4)
        raw = ser.read(4096)
        if raw:
            print(f"  OK — module responded ({len(raw)} bytes): {raw[:64].hex(' ')}")
            if len(raw) > 64:
                print(f"       ... ({len(raw)} bytes total)")
            ser.close()
            return True
    ser.close()
    print("  no response (check wiring: fan TX→adapter RX, fan RX→adapter TX, GND; fan powered)")
    return False


def default_ports() -> list[str]:
    patterns = [
        "/dev/cu.usbserial-*",
        "/dev/cu.SLAB_USBtoUART*",
        "/dev/cu.wchusbserial*",
        "/dev/tty.usbserial-*",
    ]
    found: list[str] = []
    for pat in patterns:
        found.extend(sorted(glob.glob(pat)))
    # Prefer cu.* (callout) on macOS
    cu = [p for p in found if "/cu." in p]
    return cu or found


def main() -> int:
    ports = [sys.argv[1]] if len(sys.argv) > 1 else default_ports()
    if not ports:
        print("No serial ports found.")
        print("CP2102 on macOS is usually /dev/cu.usbserial-XXXX (install Silicon Labs driver if missing).")
        print("  ls /dev/cu.usbserial-*")
        return 1
    if len(sys.argv) <= 1:
        print("CP2102 / USB-serial candidates:", ", ".join(ports))
        print()
    for port in ports:
        if probe(port):
            print(f"\nUse: PORT={port}")
            return 0
    print("\nNo module answered on any port.")
    print("If stock firmware is hung: ground CEN briefly, or power-cycle the fan.")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
