# ESPHome monorepo

Shared [ESPHome](https://esphome.io) projects and external components in one repo.

```
components/          # shared external components (import from any project)
configurators/       # static web tools (pixel_layout preview)
devices/             # ESPHome device projects
  solar-plant/       # mini solar plant (relay board + EPEVER + Valence)
  dreo-nomad/        # Dreo DR-HTF007S tower fan (BK7231N)
  waveshare-hub75/   # Waveshare ESP32-S3-RGB-Matrix (HUB75)
secrets.yaml         # gitignored — Wi-Fi, API keys, etc. (see secrets.yaml.example)
mk/                  # shared Makefile includes
```

## Quick start

```bash
cp secrets.yaml.example secrets.yaml   # fill in your values
make -C devices/solar-plant build
make -C devices/solar-plant ota DEVICE=mini-solar-plant.local
```

Each project Makefile symlinks `secrets.yaml` from the repo root before running ESPHome.

## Using shared components

From a project YAML in this repo:

```yaml
external_components:
  - source:
      type: local
      path: ../../components
```

From another machine / repo (GitHub):

```yaml
external_components:
  - source: github://Oddiesea/esphome-projects@main
    components: [valence_rt]
```

Available components:

| Component | Description |
|-----------|-------------|
| [`valence_rt`](components/valence_rt/) | Valence U1-12RT RS485 battery telemetry |
| [`dreo_tuya_mcu`](components/dreo_tuya_mcu/) | Dreo DR-HTF007S Tuya MCU UART bridge |
| [`hub75_dma`](components/hub75_dma/) | ESP32-S3 HUB75 LED matrix (ESP32-HUB75-MatrixPanel-DMA) |
| [`pixel_layout`](components/pixel_layout/) | Widget-tree compositor for ESPHome displays (clock themes, sprites) |

Preview layouts: [`configurators/pixel_layout`](configurators/pixel_layout/) (`make serve` builds the Vite/Svelte UI then serves it).

## Projects

| Folder | Device | Docs |
|--------|--------|------|
| [`devices/solar-plant/`](devices/solar-plant/) | ESP32 relay board — EPEVER + Valence | [README](devices/solar-plant/README.md) |
| [`devices/dreo-nomad/`](devices/dreo-nomad/) | Dreo DR-HTF007S tower fan (BK7231N) | [README](devices/dreo-nomad/README.md) |
| [`devices/waveshare-hub75/`](devices/waveshare-hub75/) | Waveshare ESP32-S3-RGB-Matrix | [README](devices/waveshare-hub75/README.md) |

## Development

```bash
make test      # component host unit tests (all)
make smoke     # ESPHome smoke compiles (ci/smoke/)
make ci        # test + smoke
```

Host tests: `components/<name>/tests/`. Smoke configs: [`ci/smoke/`](ci/smoke/). See [`ci/README.md`](ci/README.md).

Layout preview: [`configurators/pixel_layout/`](configurators/pixel_layout/) (`make -C configurators/pixel_layout serve`).

CI on GitHub: parallel unit tests + smoke compiles; component zips on release.

## Secrets

Single `secrets.yaml` at repo root (gitignored). Template:

```yaml
wifi_ssid: "..."
wifi_password: "..."
fallback_wifi_password: "..."

# solar-plant (mini-solar-plant)
api_encryption_key: "..."

# dreo-nomad (bedroom-smart-fan)
dreo_api_encryption_key: "..."
dreo_ota_password: "..."

# waveshare-hub75 (waveshare-matrix)
waveshare_hub75_api_encryption_key: "..."
```

Projects reference these with `!secret` in their device YAML.
