#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

mkdir -p "$TMP/project/out"
cp "$ROOT/examples/gp_demo.flash" "$TMP/project/app.flash"
mkdir -p "$TMP/project/examples"
cp "$ROOT/tests/fixtures/external_package_check.flash" "$TMP/project/examples/check.flash"

(
    cd "$TMP/project"
    unset ZNC FLASHZ
    FLASH_HOME="$ROOT" "$ROOT/flashc" "examples/check.flash" --check
    FLASH_HOME="$ROOT" "$ROOT/flashc" "app.flash" -o "out/app"
)
test -x "$TMP/project/out/app"
grep -qF "@import(\"$ROOT/flash/rt/rt.zag\")" "$TMP/project/app_shim.zag"
"$TMP/project/out/app" | grep -qF "gp_demo: ok"

echo "external_package_gate: ok"
