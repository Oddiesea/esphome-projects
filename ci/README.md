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
3. **smoke** — matrix over those components, runs `esphome compile ci/smoke/<name>.yml` in the official ESPHome Docker image. PlatformIO state lives in `.esphome-cache` (mounted at `/cache`, with `HOME=/cache`) and is restored via `actions/cache` keyed by component + ESPHome version + smoke/component hashes. Compile-only — avoids `build-action` requiring `firmware.factory.bin` (BK72xx does not produce it)
4. **package-version** — on `release` or `workflow_dispatch`; resolves `vYY-M-N` (2-digit year, month, Nth release that month). On `workflow_dispatch` it creates the next tag; on `release` it uses the release tag
5. **package** — zips each component as `<name>-vYY-M-N.zip`, uploads a versioned GitHub Actions artifact, and attaches to the GitHub release when applicable

`test` and `smoke` run in parallel after `setup`. Smoke does **not** upload firmware binaries — success is compile-only. PlatformIO toolchains are cached in `.esphome-cache` via `actions/cache`.

Adding a component only requires updating `COMPONENTS` in `mk/components.mk` (plus its `ci/smoke/<name>.yml`).

Local packaging uses the same calver helper:

```bash
make package   # → dist/<component>-v26-7-1.zip (next free N for this month)
ci/scripts/next-version.sh
```
