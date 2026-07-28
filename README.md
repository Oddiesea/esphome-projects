# ESPHome monorepo

Shared [ESPHome](https://esphome.io) projects and external components in one repo.

```
components/          # shared external components (import from any project)
devices/             # ESPHome device projects
  solar-plant/       # mini solar plant (relay board + EPEVER + Valence)
  dreo-nomad/        # Dreo DR-HTF007S tower fan (BK7231N)
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

## Projects

| Folder | Device | Docs |
|--------|--------|------|
| [`devices/solar-plant/`](devices/solar-plant/) | ESP32 relay board — EPEVER + Valence | [README](devices/solar-plant/README.md) |
| [`devices/dreo-nomad/`](devices/dreo-nomad/) | Dreo DR-HTF007S tower fan (BK7231N) | [README](devices/dreo-nomad/README.md) |

## Development

```bash
make test      # component host unit tests (all)
make smoke     # ESPHome smoke compiles (ci/smoke/)
make ci        # test + smoke
```

Host tests: `components/<name>/tests/`. Smoke configs: [`ci/smoke/`](ci/smoke/). See [`ci/README.md`](ci/README.md).

CI on GitHub: parallel unit tests + smoke compiles; component zips on release.

## Secrets

Single `secrets.yaml` at repo root (gitignored). Template:

```yaml
wifi_ssid: "..."
wifi_password: "..."
fallback_wifi_password: "..."
api_encryption_key: "..."   # solar-plant
dreo_api_encryption_key: "..."   # dreo-nomad
dreo_ota_password: "..."
```

Projects reference these with `!secret` in their device YAML.
