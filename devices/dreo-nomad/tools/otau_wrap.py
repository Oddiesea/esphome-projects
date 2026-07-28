#!/usr/bin/env python3
"""Wrap a CMS8051 MCU firmware .bin in an OTAU container for HeFi /mcu upload.

WiFi module OTA (/module) uses raw encrypted .ota.rbl files — see upload.py --raw.
OTAU is only for the motor MCU path (/mcu).
"""

import hashlib
import struct
import sys


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def pack_product_id(header: bytearray, product_id: int) -> None:
    """Split 64-bit Dreo product ID into high (0x04) + low (0x08) per HeFi OTAU spec."""
    struct.pack_into(">I", header, 0x04, (product_id >> 32) & 0xFFFFFFFF)
    struct.pack_into(">Q", header, 0x08, product_id & 0xFFFFFFFF)


def make_otau(
    rbl_path: str,
    output_path: str,
    product_id: int = 0x142B293711D19001,
    version: tuple = (9, 9, 9),
    min_version: tuple = (0, 0, 0),
):
    with open(rbl_path, "rb") as f:
        firmware = f.read()

    md5 = hashlib.md5(firmware).digest()
    fw_size = len(firmware)

    header = bytearray(128)
    header[0:4] = b"OTAU"
    pack_product_id(header, product_id)
    struct.pack_into(">I", header, 0x10, fw_size)
    header[0x19] = version[0]
    header[0x1A] = version[1]
    header[0x1B] = version[2]
    header[0x1D] = min_version[0]
    header[0x1E] = min_version[1]
    header[0x1F] = min_version[2]
    header[0x20:0x30] = md5
    crc = crc16(bytes(header[:126]))
    struct.pack_into(">H", header, 0x7E, crc)

    with open(output_path, "wb") as f:
        f.write(bytes(header))
        f.write(firmware)

    print(f"OTAU container: {output_path}")
    print(f"  Magic: OTAU")
    print(f"  Product ID: 0x{product_id:016X}")
    print(f"    high @0x04: 0x{(product_id >> 32) & 0xFFFFFFFF:08X}")
    print(f"    low  @0x08: 0x{product_id & 0xFFFFFFFF:08X}")
    print(f"  FW size: {fw_size} bytes")
    print(f"  Version: {version[0]}.{version[1]}.{version[2]}")
    print(f"  Min version: {min_version[0]}.{min_version[1]}.{min_version[2]}")
    print(f"  MD5: {md5.hex()}")
    print(f"  Header CRC16: 0x{crc:04X}")
    print(f"  Total size: {128 + fw_size} bytes")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(
            f"Usage: {sys.argv[0]} <input.rbl> <output.otau> "
            f"[product_id_hex] [major.minor.patch]"
        )
        sys.exit(1)
    pid = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x142B293711D19001
    ver = (9, 9, 9)
    if len(sys.argv) > 4:
        ver = tuple(int(x) for x in sys.argv[4].split("."))
    make_otau(sys.argv[1], sys.argv[2], product_id=pid, version=ver)
