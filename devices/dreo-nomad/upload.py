#!/usr/bin/env python3
"""Upload firmware to Dreo HeFi /module OTA (correct multipart boundary).

curl -F corrupts the payload and returns "download error". Use this script instead.
"""

from __future__ import annotations

import argparse
import os
import sys
import urllib.error
import urllib.request
from typing import Tuple

DEFAULT_BOUNDARY = "0123456789abcdef0123456789abcdef12"  # exactly 38 chars


def build_multipart_body(path: str, boundary: str = DEFAULT_BOUNDARY) -> Tuple[bytes, str]:
    filename = os.path.basename(path)
    with open(path, "rb") as f:
        payload = f.read()
    body = (
        f"--{boundary}\r\n"
        f'Content-Disposition: form-data; name="firmware"; filename="{filename}"\r\n'
        f"Content-Type: application/octet-stream\r\n\r\n"
    ).encode() + payload + f"\r\n--{boundary}--\r\n".encode()
    return body, filename


def upload_file(
    path: str,
    endpoint: str = "/module",
    boundary: str = DEFAULT_BOUNDARY,
    host: str = "192.168.0.1",
) -> Tuple[int, str]:
    """POST firmware to HeFi. Returns (exit_code, response_text)."""
    body, filename = build_multipart_body(path, boundary)
    url = f"http://{host}{endpoint}"
    print(f"POST {url}")
    print(f"  file: {path} ({len(body)} bytes multipart body)")
    print(f"  multipart filename: {filename!r}")

    req = urllib.request.Request(
        url,
        data=body,
        method="POST",
        headers={"Content-Type": f"multipart/form-data; boundary={boundary}"},
    )
    try:
        with urllib.request.urlopen(req, timeout=180) as resp:
            text = resp.read().decode(errors="replace")
    except urllib.error.HTTPError as e:
        text = e.read().decode(errors="replace")
    except urllib.error.URLError as e:
        print(f"Connection failed: {e}")
        return 1, str(e)

    if text.strip():
        print(text.strip())
    if "download error" in text.lower():
        return 1, text
    if "successfully" in text.lower() or "waiting for the installation" in text.lower():
        print("\nDownload OK. Do NOT poll the AP or switch to home WiFi yet.")
        print("Wait 5 minutes (60s unplugged, plug in, wait 5 min).")
        print("Success: dreo-FALLBACK AP or device on home WiFi.")
        print("Failure: stock Dreo AP returns with MODEL OTA 3.7.7.")
    return 0, text


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Upload firmware to Dreo HeFi OTA")
    p.add_argument("firmware", help="Path to .rbl (with --raw) or .otau file")
    p.add_argument(
        "--raw",
        action="store_true",
        help="Upload raw repacked RBL to /module (first flash from stock)",
    )
    p.add_argument("--endpoint", default="/module", help="OTA path (default /module)")
    return p.parse_args()


def main() -> int:
    args = parse_args()
    if args.raw and not args.firmware.endswith(".otau"):
        print("Uploading raw RBL to /module.")
    code, body = upload_file(args.firmware, endpoint=args.endpoint)
    if "download error" in body.lower():
        return 1
    return code


if __name__ == "__main__":
    raise SystemExit(main())
