#!/usr/bin/env python3
"""Fetch Dreo cloud WiFi module firmware 3.2.6 for diagnostic /module upload."""

import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STOCK_URL = (
    "https://d13h33p641vwpi.cloudfront.net/data/upgrade/202410/18/"
    "002d3016a3f049ba938ce20c4a2638ba.rbl"
)
STOCK_RBL = ROOT / "tools" / "dreo_module_3.2.6.rbl"


def main() -> int:
    STOCK_RBL.parent.mkdir(parents=True, exist_ok=True)
    if not STOCK_RBL.is_file():
        print("Downloading stock module 3.2.6 ...")
        urllib.request.urlretrieve(STOCK_URL, STOCK_RBL)
    print(f"Ready: {STOCK_RBL} ({STOCK_RBL.stat().st_size} bytes)")
    print("Upload raw RBL to /module (NOT OTAU-wrapped):")
    print(f"  python3 upload.py {STOCK_RBL} --raw")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
