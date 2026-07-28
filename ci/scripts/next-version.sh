#!/usr/bin/env bash
# Next calendar package version: vYY-M-N
#   YY = 2-digit year, M = month (no leading zero), N = Nth release that month
# Counts existing git tags matching vYY-M-* and increments N.
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"

# GNU and BSD date both support %-y / %-m (no zero-pad).
year="$(date +%-y)"
month="$(date +%-m)"
prefix="v${year}-${month}-"

max=0
if git -C "$repo_root" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  while IFS= read -r tag; do
    [[ -n "$tag" ]] || continue
    n="${tag#"${prefix}"}"
    [[ "$n" =~ ^[0-9]+$ ]] || continue
    if (( n > max )); then
      max=$n
    fi
  done < <(git -C "$repo_root" tag -l "${prefix}*")
fi

echo "${prefix}$((max + 1))"
