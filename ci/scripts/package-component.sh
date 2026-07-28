#!/usr/bin/env bash
# Package one external component as a distributable zip (tests/Makefile stripped).
set -euo pipefail

component="${1:?usage: package-component.sh <component> [version]}"
version="${2:-}"

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
src="${repo_root}/components/${component}"
dist_root="${repo_root}/dist/components/${component}"

if [[ ! -d "$src" ]]; then
  echo "Component not found: $src" >&2
  exit 1
fi

if [[ -z "$version" ]]; then
  version="$("${repo_root}/ci/scripts/next-version.sh")"
fi

archive="${repo_root}/dist/${component}-${version}.zip"
rm -rf "$dist_root"
mkdir -p "$dist_root"

cp -a "$src/." "$dist_root/"
rm -rf "$dist_root/tests" "$dist_root/Makefile"
find "$dist_root" -type d -name '__pycache__' -exec rm -rf {} + 2>/dev/null || true
find "$dist_root" -type f -name '*.pyc' -delete 2>/dev/null || true

mkdir -p "${repo_root}/dist"
rm -f "$archive"
(
  cd "${repo_root}/dist"
  zip -rq "$(basename "$archive")" "components/${component}"
)

echo "$archive" >&2
ls -lah "$archive" >&2
unzip -l "$archive" >&2

# Machine-readable path on stdout (for CI capture).
echo "$archive"
