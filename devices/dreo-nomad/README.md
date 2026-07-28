# Dreo DR-HTF007S Tower Fan

Replace the stock **BK7231N** WiFi module firmware with ESPHome. The fan MCU (CMS80F7518) stays in place — ESPHome talks to it over UART using the standard Tuya MCU protocol.

Device name: `bedroom-smart-fan`

Based on [dreo-cloudcutter](https://github.com/alxgmpr/dreo-cloudcutter) work (fork of [ouaibe/dreo-cloudcutter](https://github.com/ouaibe/dreo-cloudcutter)).

## Hardware

| Part | Notes |
|------|--------|
| Model | Dreo DR-HTF007S tower fan |
| WiFi module | WZ07-W / MBL01 (BL2028N / BK7231N) |
| Fan MCU | CMS80F7518 (8051, UART only — not flashable without CMS-ICE8) |
| UART to MCU | **115200 8N1** on module TX1/RX1 |
| UART debug / flash | **9600** on TX2/RX2 for ltchiptool (module flash read/write) |
| OTA crypto | Key `0123456789ABCDEF0123456789ABCDEF`, IV `0123456789ABCDEF` |

### UART wiring (FTDI / CP2102 @ 3.3V)

| Fan | Adapter |
|-----|---------|
| TX | → RX |
| RX | ← TX |
| GND | GND |

macOS port: `/dev/cu.usbserial-XXXX` (`ls /dev/cu.usbserial-*`)

**Bootloader:** ground **CEN** (back of display PCB) ~0.5s when ltchiptool prompts.

## Protocol

Standard Tuya MCU framing at 115200:

```
55 AA [ver] [cmd] [??] [len_hi] [len_lo] [data...] [checksum]
```

Checksum = sum of all bytes before checksum, `& 0xFF`.

### Datapoints (`dreo_tuya_mcu` component)

| DP | Name | Type | Description |
|----|------|------|-------------|
| 1 | poweron | bool | Fan on/off |
| 2 | ledalwayson | bool | Display LED always on |
| 3 | windtype | enum | 1=Normal, 2=Natural, 3=Sleep, 4=Auto |
| 4 | windlevel | enum | Speed 1–8 |
| 5 | voiceon | bool | Beeper |
| 6 | timeron | value | Turn-on timer (minutes, 0–480) |
| 7 | timeroff | value | Turn-off timer (minutes) |
| 8 | shakehorizon | bool | Oscillation |
| 9 | wrong | enum | Fault: 0=OK, 1=E1, 2=EU |
| 11 | temperature | value | Ambient temp (°F when tempunit=1) |
| 12 | tempunit | enum | 0=°C, 1=°F |

### Known quirks

- **WiFi LED** on the panel is driven by the MCU internal state machine — not controllable via UART/WiFi status commands.
- Stock HeFi OTA uses **`/module`** with a raw encrypted **RBL** — not the HTML upload form. **Do not use `curl -F`** (corrupts multipart → "download error").

## Secrets

Repo root `secrets.yaml` (symlinked automatically):

```yaml
wifi_ssid: "..."
wifi_password: "..."
fallback_wifi_password: "..."
dreo_api_encryption_key: "..."
dreo_ota_password: "..."
```

## Flashing workflow (BK7231N replacement)

### 0. Backup first

```bash
make backup PORT=/dev/cu.usbserial-XXXX
```

### 1. Stage-1 via stock Dreo AP (recommended first hop)

Stock firmware exposes `http://192.168.0.1/module` in pairing mode.

```bash
make build-stage1-wifi-nogzip    # WiFi-only minimal image (avoids MCU UART crash on first boot)
make ota-upload                  # fan in pairing mode, phone/laptop on Dreo AP
```

Wait ~5 minutes after upload (do not poll the AP). Success: `dreo-FALLBACK` AP or device joins home Wi‑Fi.

UART-assisted OTA (keeps MCU heartbeat alive during upload):

```bash
make ota-upload-uart PORT=/dev/cu.usbserial-XXXX
```

### 2. Full ESPHome firmware

After stage-1 boots and is on your LAN:

```bash
make build
make ota DEVICE=bedroom-smart-fan.local
```

### 3. Direct UART flash (skip stage-1)

If you already have UART access and a backup:

```bash
make uart-flash PORT=/dev/cu.usbserial-XXXX
# or ltchiptool path:
make build && make uart-flash-lt PORT=/dev/cu.usbserial-XXXX
```

## Day-to-day

```bash
make config          # validate YAML
make build           # compile full firmware
make test            # host dreo_tuya_protocol unit tests (repo root)
make smoke COMPONENT=dreo_tuya_mcu
make local-config    # validate dreo-nomad.local.yml (local component)
make local-build     # compile dreo-nomad.local.yml (local component)
make ota             # OTA to bedroom-smart-fan.local
make logs            # follow device logs
make uart-check      # probe module UART
```

From repo root: `make -C devices/dreo-nomad <target>`

## Tools

| Script | Purpose |
|--------|---------|
| [`upload.py`](upload.py) | Correct multipart upload to `/module` |
| [`tools/build_stage1.py`](tools/build_stage1.py) | Build stage-1 RBL images |
| [`tools/ota_upload.py`](tools/ota_upload.py) | UART heartbeat + stock OTA |
| [`tools/hefi_uart.py`](tools/hefi_uart.py) | Monitor UART, factory reset, init |
| [`tools/uart_check.py`](tools/uart_check.py) | Auto-detect / probe serial port |

```bash
make deps
python3 tools/hefi_uart.py /dev/cu.usbserial-XXXX monitor
python3 tools/hefi_uart.py /dev/cu.usbserial-XXXX reset
```

Stage-1 YAMLs: [`minimal_wifi_only.yaml`](minimal_wifi_only.yaml) (recommended), [`minimal_flash.yaml`](minimal_flash.yaml) (includes Tuya).

## Config

Primary device config: [`dreo-nomad.yml`](dreo-nomad.yml) — references `dreo_tuya_mcu` as an external GitHub component (placeholder repo URL to update).

Local dev/smoke config: [`dreo-nomad.local.yml`](dreo-nomad.local.yml) — references local `../../components` for quick component iteration and compile checks.

## Disclaimer

Flashing custom firmware can brick the WiFi module. Keep a UART backup and be prepared to recover via ltchiptool.
