# CI

Continuous integration for this monorepo is driven from the repo root:

```bash
make test              # host GoogleTest (components/*/tests)
make smoke             # ESPHome compile smoke (ci/smoke/*.yml)
make ci                # test + smoke
make package           # zip components to dist/
```

Run a single component:

```bash
make test COMPONENT=valence_rt
make smoke COMPONENT=dreo_tuya_mcu
```

## Layout

| Path | Purpose |
|------|---------|
| [`smoke/`](smoke/) | Minimal ESPHome YAMLs — one per component, no secrets |
| [`scripts/package-component.sh`](scripts/package-component.sh) | Build a distributable component zip |

Smoke configs use `path: ../../components` (repo-root `components/`).

## GitHub Actions

[`.github/workflows/ci.yml`](../.github/workflows/ci.yml):

1. **setup** — resolves matrix from [`mk/components.mk`](../mk/components.mk) and ESPHome version from [`requirements.txt`](../requirements.txt) via [`scripts/resolve-ci-matrix.sh`](scripts/resolve-ci-matrix.sh)
2. **test** — matrix over those components, runs `make test COMPONENT=<name>`
3. **smoke** — matrix over those components, runs `esphome compile ci/smoke/<name>.yml` in the official ESPHome Docker image (compile-only; avoids `build-action` requiring `firmware.factory.bin`, which BK72xx does not produce)
4. **package** — on `release` or `workflow_dispatch` only; uploads zip artifacts and attaches to the GitHub release

`test` and `smoke` run in parallel after `setup`. Adding a component only requires updating `COMPONENTS` in `mk/components.mk` (plus its `ci/smoke/<name>.yml`).
