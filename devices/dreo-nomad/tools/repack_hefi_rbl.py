#!/usr/bin/env python3
"""Repack an ESPHome .ota.rbl with HeFi-compatible metadata (stock version string)."""

import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
KEY = "0123456789ABCDEF0123456789ABCDEF"
IV = "0123456789ABCDEF"
LT = ROOT / ".venv/bin/ltchiptool"
# Version string used in Dreo cloud / factory WiFi module RBLs for this platform.
HEFI_RBL_VERSION = "20191018"


def repack(in_rbl: Path, out_rbl: Path, compress: str = "gzip") -> None:
    with tempfile.TemporaryDirectory() as tmp:
        dec = Path(tmp) / "dec.bin"
        subprocess.run(
            [
                str(LT), "soc", "bkpackager", "deota",
                "--key", KEY, "--iv", IV,
                str(in_rbl), str(dec),
            ],
            check=True,
        )
        subprocess.run(
            [
                str(LT), "soc", "bkpackager", "ota",
                str(dec), str(out_rbl),
                compress, "aes256",
                "--key", KEY, "--iv", IV,
                "-v", HEFI_RBL_VERSION,
            ],
            check=True,
        )


def main() -> int:
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input.ota.rbl> <output.rbl> [gzip|none]")
        return 1
    compress = sys.argv[3] if len(sys.argv) > 3 else "gzip"
    repack(Path(sys.argv[1]), Path(sys.argv[2]), compress=compress)
    print(f"Repacked ({compress}) with RBL version {HEFI_RBL_VERSION!r}: {sys.argv[2]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
