# Waveshare ESP32-S3 RGB LED Matrix

ESPHome on a [Waveshare ESP32-S3-RGB-Matrix](https://www.waveshare.com/esp32-s3-rgb-matrix.htm) driver board, driving a HUB75 panel through the shared [`hub75_dma`](../../components/hub75_dma/) component (wraps [ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA)).

Device name: `waveshare-matrix`

The component itself is generic for any ESP32-S3 HUB75 wiring. This project supplies the Waveshare pin preset plus a [`pixel_layout`](../../components/pixel_layout/) plumber-clock screen (overworld backdrop, drifting clouds, typeface clock, gameman weather). Sprite PNGs live under `assets/` next to the device YAML, not in the shared component.

Layout preview: [`configurators/pixel_layout`](../../configurators/pixel_layout/).

## Hardware

| Part | Notes |
|------|--------|
| Driver | Waveshare ESP32-S3-RGB-Matrix (ESP32-S3-N32R16) |
| Panel | P2 indoor SMD1515, 256×128 mm, **128×64**, 1/32 scan, DC 5 V |
| Power | Dedicated 5 V supply. This module is ~20 W peak (~4 A at 5 V); do not power LEDs from USB. |
| USB | Type-C — flashing and `USB_SERIAL_JTAG` logs |

HUB75 pins are fixed on this board (`board: waveshare-esp32-s3-rgb-matrix`). `e_pin` is required for 1/32-scan (64-row) panels and is included in the preset. `clock_phase` defaults to `false` (avoids a 1-pixel horizontal shift).

The board can drive **any HUB75 module size**, stacked in a **grid**, up to **24576 pixels** (neither logical edge above 384). DMA always clocks a single horizontal chain; `chain_type` remaps that into rows.

| Layout | YAML |
|--------|------|
| One 128×64 | `panel_width: 128`, `panel_height: 64` |
| Three 128×64 in a row | `chain_length: 3` (same as `chain_cols: 3`) |
| Two 128×64 stacked (128×128) | `chain_rows: 2`, `use_psram: true` |
| Two 64×32 stacked (64×64) | `panel_width: 64`, `panel_height: 32`, `chain_rows: 2` |
| 2×2 of 64×32 (128×64) | `chain_cols: 2`, `chain_rows: 2` |
| Six 64×64 in a row | `panel_width: 64`, `chain_cols: 6` |
| Portrait of a 384×64 chain | `chain_length: 3`, `chain_rotation: 90` |
| Zigzag (all panels upright) | `chain_type: top_right_down_zz` |

`chain_type` is how the data cable snakes, viewed from the LED face: `top_right_down` (default when `chain_rows` > 1), `top_left_down`, `bottom_left_up`, `bottom_right_up`, plus `_zz` variants that keep every panel upright. `chain_rotation` is 0/90/180/270 of the assembled canvas. Above 128×64 total pixels, set `use_psram: true`.

```yaml
display:
  - platform: hub75_dma
    board: waveshare-esp32-s3-rgb-matrix
    panel_width: 64
    panel_height: 32
    chain_cols: 2
    chain_rows: 2          # 128×64 canvas; DMA chain_length becomes 4
    # chain_type: top_right_down   # default for a stack
```

The example uses [`pixel_layout`](../../components/pixel_layout/) via `plumber-clock.package.yml`. Remap `ha_example_weather_condition_entity` to your Home Assistant weather entity. Preview layouts in [`configurators/pixel_layout/`](../../configurators/pixel_layout/). If colors look swapped or the image is garbled, try `driver: FM6126A` (common on indoor ICN2038S panels).

## Build / flash

Secrets live at the **repo root** (`../secrets.yaml`). The Makefile symlinks them here automatically.

```bash
make local-build                          # local hub75_dma component
make build                                # production (GitHub components)
make flash                                # USB serial upload + logs
make ota DEVICE=waveshare-matrix.local
```

From the repo root:

```bash
make -C devices/waveshare-hub75 local-config
```

## Another ESP32-S3 board

`hub75_dma` is not Waveshare-specific. Drop `board:` and set every pin:

```yaml
display:
  - platform: hub75_dma
    id: matrix
    panel_width: 128
    panel_height: 64
    r1_pin: 25
    g1_pin: 26
    b1_pin: 27
    r2_pin: 14
    g2_pin: 12
    b2_pin: 13
    a_pin: 23
    b_pin: 19
    c_pin: 5
    d_pin: 17
    e_pin: 18   # required for 1/32-scan (64-row) panels
    lat_pin: 4
    oe_pin: 15
    clk_pin: 16
```

If the image is shifted by one pixel, set `clock_phase: false`.
