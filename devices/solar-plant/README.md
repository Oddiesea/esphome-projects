# Solar Plant — ESPHome monitoring + 4 Way Relay

One ESP32_Relay_X4_Modbus_v1.3 board runs everything via [`solar-plant.yml`](solar-plant.yml) (production) or [`solar-plant.local.yml`](solar-plant.local.yml) (local component dev):

- Onboard relays + dry-contact inputs
- EPEVER MPPT Solar Charge Controller 15A Tracer-AN over the board’s SP3485E RS485
- Valence U1-12RJ 45AH LiFePo4 12v Battery over an external MAX485 on Pad TX2/RX2
- 100w Bifacial 12v Solar Panel

Home Assistant talks to this device over the ESPHome API.

```
  Home Assistant
        |
        | ESPHome API (Wi-Fi)
        v
  +------------------+
  |   RELAY ESP32    |
  |  Relay_X4 board  |
  +--------+---------+---------+
           |                   |
           | onboard SP3485E   | external MAX485
           | RS485 A/B         | (Pad TX2/RX2)
           v                   v
      EPEVER Tracer-AN    Valence U1-12RJ
```

---

## Build / flash

Secrets live at the **repo root** (`../secrets.yaml`). The Makefile symlinks them here automatically.

```bash
make build              # production config (GitHub components)
make local-build        # local component dev
make ota DEVICE=mini-solar-plant.local
# or from repo root:
make -C devices/solar-plant run
```

---

## 1. Waterproof EPEVER 15A ↔ onboard RS485

Use the board’s built-in SP3485E and screw terminals. DE is wired to GPIO32 on the PCB.

| Relay board | EPEVER RS485 / COM |
|-------------|-------------------|
| RS485 **A** | 2 |
| RS485 **B** | 3 |
| GND (optional) | GND if needed |

- Baud: **115200 8N1**
- Modbus slave address: **0x01**

Do not connect EPEVER M12 pin 1 (+5V) to the relay board. An RJ45 connection is used in the non-waterproof version.

---

## 2. Valence ↔ MAX485 on Pad TX2/RX2

| Relay (Pad) | MAX485 |
|-------------|--------|
| **Pad TX2** GPIO17 | DI |
| **Pad RX2** GPIO16 | RO |
| GPIO14 | DE and /RE (tied) |
| Pad 5V | VCC |
| Pad GND | GND |

| MAX485 | Valence RJ45 / Superseal |
|--------|--------------------------|
| A | RS485 A |
| B | RS485 B |
| GND | ground |

Wake quirk: Valence RS485 only talks while the pack is charging or discharging. The heartbeat LED flashes faster when active.

RS485 is on RJ45 pins 4/5 (A + B) on some models.

---

## 3. External component

Uses shared [`valence_rt`](../../components/valence_rt/) (`role: direct`).

**Production** (`solar-plant.yml`):

```yaml
external_components:
  - source: github://Oddiesea/esphome-projects@main
    components: [valence_rt]
```

**Local dev** (`solar-plant.local.yml`):

```yaml
external_components:
  - source:
      type: local
      path: ../../components
    components: [valence_rt]
```

---

## 4. Quick checklist

- [ ] EPEVER A/B on relay onboard RS485 terminals
- [ ] MAX485 on Pad TX2/RX2; DE+/RE tied to GPIO14
- [ ] MAX485 A/B to Valence RS485
- [ ] Root `secrets.yaml` filled in; `make build`
