#!/usr/bin/env python3
"""Compile stage-1 ESPHome and produce HeFi-compatible raw RBL(s) for /module."""

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPO_ROOT = ROOT.parents[1]
ESPHOME_CANDIDATES = (
    REPO_ROOT / ".venv/bin/esphome",
    ROOT / ".venv/bin/esphome",
)
PROFILES = {
    "default": (ROOT / "minimal_flash.yaml", ROOT / "firmware-stage1.rbl"),
    "wifi-only": (ROOT / "minimal_wifi_only.yaml", ROOT / "firmware-stage1-wifi-only.rbl"),
}
BUILD_RBL = (
    ROOT
    / ".esphome/build/bedroom-smart-fan/.pioenvs/bedroom-smart-fan/image_bk7231n_app.ota.rbl"
)
REPACK = ROOT / "tools/repack_hefi_rbl.py"


def find_esphome() -> Path:
    for candidate in ESPHOME_CANDIDATES:
        if candidate.is_file():
            return candidate
    raise SystemExit(
        "Missing esphome — create repo .venv: python3 -m venv .venv && .venv/bin/pip install esphome"
    )


def build_one(esphome: Path, yaml: Path, out: Path, *, nogzip: bool) -> None:
    print(f"Compiling {yaml.name} (download partition 0x133000+0x9F220) ...")
    subprocess.run([str(esphome), "compile", str(yaml)], cwd=ROOT, check=True)
    if not BUILD_RBL.is_file():
        raise SystemExit(f"RBL not found at {BUILD_RBL}")

    compress = "none" if nogzip else "gzip"
    print(f"Repacking RBL (version 20191018, compression={compress}) ...")
    subprocess.run(
        [sys.executable, str(REPACK), str(BUILD_RBL), str(out), compress],
        cwd=ROOT,
        check=True,
    )

    otau_out = out.with_suffix(".otau")
    subprocess.run(
        [sys.executable, str(ROOT / "tools/otau_wrap.py"), str(out), str(otau_out)],
        cwd=ROOT,
        check=True,
    )

    with open(out, "rb") as f:
        magic = f.read(4)
        ver = f.read(0x40)[0x1C - 4 : 0x40 - 4].split(b"\x00")[0]
    print(f"  {out.name}  ({out.stat().st_size} bytes)  magic={magic!r} version={ver.decode()}")
    print(f"  {otau_out.name} ({otau_out.stat().st_size} bytes)")


def main() -> int:
    esphome = find_esphome()

    parser = argparse.ArgumentParser(description="Build stage-1 HeFi OTA images")
    parser.add_argument(
        "--profile",
        choices=sorted(PROFILES),
        default="default",
        help="default=minimal+tuya; wifi-only=no UART (pairing-crash workaround)",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Build default + wifi-only, gzip and nogzip variants",
    )
    parser.add_argument("--nogzip", action="store_true", help="Use uncompressed RBL (older HeFi 3.7.7)")
    args = parser.parse_args()

    if args.all:
        for _name, (yaml, out) in PROFILES.items():
            build_one(esphome, yaml, out, nogzip=False)
            nogzip_out = out.with_name(out.stem + "-nogzip.rbl")
            build_one(esphome, yaml, nogzip_out, nogzip=True)
    else:
        yaml, out = PROFILES[args.profile]
        if args.nogzip:
            out = out.with_name(out.stem + "-nogzip.rbl")
        build_one(esphome, yaml, out, nogzip=args.nogzip)

    print("\nUpload (fan in pairing mode, join Dreo AP 192.168.0.1):")
    print("  make ota-upload FW=firmware-stage1-wifi-only-nogzip.rbl")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
