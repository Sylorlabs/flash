#!/usr/bin/env bash
# tests/flasht_target_gate.sh — Phase 4: ppu32 lowers t-units to native tN names.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FLASHC="${FLASHC:-$ROOT/flashc}"
FIXTURE="$ROOT/tests/fixtures/tunit_target_gate.flash"
SHIM="$ROOT/tests/fixtures/tunit_target_gate_shim.zag"

fail() {
    echo "FAIL: flasht_target_gate — $1"
    exit 1
}

echo "== flasht_target_gate: --target ppu32 emits t9 (not i64) =="

rm -f "$SHIM" /tmp/flash_fe_target.name

if ! "$FLASHC" "$FIXTURE" --emit-zag --target ppu32 >/dev/null 2>&1; then
    fail "flashc could not emit tunit_target_gate with --target ppu32"
fi

if [[ ! -f "$SHIM" ]]; then
    fail "shim missing: $SHIM"
fi

if ! grep -q 't9' "$SHIM"; then
    fail "shim missing native t9 type"
fi

if grep -qE '\bt9\b.*i64|\bi64\b.*\bt9\b|fn t9_id\(v: i64\)|: i64\)' "$SHIM"; then
    fail "shim lowered t9 to i64 under --target ppu32"
    head -40 "$SHIM"
    exit 1
fi

echo "PASS: flasht_target_gate (ppu32 t-unit lowering)"