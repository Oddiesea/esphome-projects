#!/usr/bin/env python3
"""Probe HeFi HTTP endpoints while connected to the Dreo AP."""

import sys
import urllib.error
import urllib.request

BASE = "http://192.168.0.1"
PATHS = ["/", "/module", "/mcu", "/ota"]


def fetch(path: str) -> None:
    url = BASE + path
    try:
        req = urllib.request.Request(url, method="GET")
        with urllib.request.urlopen(req, timeout=5) as resp:
            body = resp.read(4096).decode(errors="replace")
            print(f"GET {path} -> HTTP {resp.status} ({len(body)} bytes)")
            for line in body.splitlines()[:12]:
                print(f"  {line}")
    except urllib.error.HTTPError as e:
        body = e.read(512).decode(errors="replace")
        print(f"GET {path} -> HTTP {e.code}: {body[:200]!r}")
    except urllib.error.URLError as e:
        print(f"GET {path} -> {e}")


def main() -> int:
    print(f"Probing {BASE} ...")
    for path in PATHS:
        fetch(path)
    print(
        "\n/module (WiFi): upload raw .ota.rbl with upload.py --raw\n"
        "  python3 upload.py firmware-stage1.rbl --raw\n"
        "/mcu (motor): OTAU-wrapped .bin only — not for ESPHome\n"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
