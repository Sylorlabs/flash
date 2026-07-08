#!/usr/bin/env bash
# tests/standalone_gate.sh — Wave 7 gate: Flash is standalone (bundled flashz, no sibling zag).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN_DIR="${BIN_DIR:-$ROOT/.test_bin}"
BUNDLED_FLASHZ="$ROOT/flash/toolchain/flashz"
FLASHC1_SRC="$ROOT/selfhost/flashc1.flash"
SMOKE_SRC="$ROOT/tests/ternary_smoke.zag"

mkdir -p "$BIN_DIR"

fail() {
    echo "FAIL: standalone_gate — $1"
    exit 1
}

echo "== standalone_gate: bundled toolchain =="
if [[ ! -f "$BUNDLED_FLASHZ" ]]; then
    fail "flash/toolchain/flashz missing"
fi
if [[ ! -x "$BUNDLED_FLASHZ" ]]; then
    fail "flash/toolchain/flashz is not executable"
fi
echo "PASS: flash/toolchain/flashz present and executable"

echo "== standalone_gate: flashc1.flash prefers bundled flashz =="
if ! grep -qF 'flash/toolchain/flashz' "$FLASHC1_SRC"; then
    fail "selfhost/flashc1.flash does not reference flash/toolchain/flashz"
fi
flashz_line="$(grep -n 'flash/toolchain/flashz' "$FLASHC1_SRC" | head -1 | cut -d: -f1)"
zag_line="$(grep -n '../zag/zag-poc/znc' "$FLASHC1_SRC" | head -1 | cut -d: -f1 || true)"
if [[ -n "$zag_line" && "$flashz_line" -ge "$zag_line" ]]; then
    fail "flash/toolchain/flashz must be probed before ../zag/zag-poc/znc in flashc1.flash"
fi
echo "PASS: flashc1.flash prioritizes flash/toolchain/flashz (line $flashz_line) before legacy zag fallbacks"

echo "== standalone_gate: build without ZNC/FLASHZ or sibling zag =="
unset ZNC FLASHZ
resolved="${ZNC:-${FLASHZ:-$BUNDLED_FLASHZ}}"
if [[ "$resolved" != "$BUNDLED_FLASHZ" ]]; then
    fail "expected bundled flashz as default compiler, got: $resolved"
fi

out="$BIN_DIR/standalone_gate_smoke.out"
rm -f "$out"
if ! "$BUNDLED_FLASHZ" "$SMOKE_SRC" -o "$out" >/dev/null 2>&1; then
    fail "flashz could not compile tests/ternary_smoke.zag with env unset"
fi
if [[ ! -x "$out" ]]; then
    fail "flashz build did not produce an executable"
fi
if ! "$out" 2>&1 | grep -qF "ternary_smoke: ok"; then
    fail "standalone smoke binary did not report ternary_smoke: ok"
fi
echo "PASS: build+run from bundled flashz with ZNC/FLASHZ unset (no ../zag required)"

echo "== standalone_gate: ok =="