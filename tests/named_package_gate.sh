#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FLASHC="${FLASHC:-$ROOT/flashc}"
PROJECT="$ROOT/tests/fixtures/named_package_project"
BIN="$ROOT/.test_bin/named_package_consumer"

cd "$PROJECT"
FLASH_HOME="$ROOT" "$FLASHC" app.flash --check
FLASH_HOME="$ROOT" "$FLASHC" app.flash -o "$BIN"
"$BIN" | grep -qF 'named_package_consumer: ok'
grep -qF '"sp.spark_new"' app_shim.zag
if grep -qF 'sp__' app_shim.zag; then
    echo "FAIL: named package alias leaked into flattened output"
    exit 1
fi
echo "named_package_gate: ok"
