#!/usr/bin/env bash
# Derive CI matrix outputs from repo sources of truth:
#   components      <- mk/components.mk (COMPONENTS := ...)
#   esphome_version <- requirements.txt (esphome==...)
#
# Writes GITHUB_OUTPUT keys when running in Actions; otherwise prints KEY=value.
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"

comps_raw="$(sed -n 's/^COMPONENTS[[:space:]]*:=[[:space:]]*//p' "${repo_root}/mk/components.mk")"
comps_raw="${comps_raw%%#*}"
read -r -a comps <<< "$comps_raw"
if [[ ${#comps[@]} -eq 0 ]]; then
  echo "error: COMPONENTS is empty in mk/components.mk" >&2
  exit 1
fi

version="$(sed -n 's/^esphome==//p' "${repo_root}/requirements.txt" | head -n1 | tr -d '[:space:]')"
if [[ -z "$version" ]]; then
  echo "error: no esphome==VERSION pin in requirements.txt" >&2
  exit 1
fi

components_json="$(python3 -c 'import json, sys; print(json.dumps(sys.argv[1:]))' "${comps[@]}")"

emit() {
  local key="$1" value="$2"
  if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
    echo "${key}=${value}" >> "${GITHUB_OUTPUT}"
  else
    echo "${key}=${value}"
  fi
}

emit components "${components_json}"
emit esphome_version "${version}"
