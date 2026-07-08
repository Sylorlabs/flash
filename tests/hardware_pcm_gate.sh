#!/usr/bin/env bash
# tests/hardware_pcm_gate.sh — Wave 8: manifest segments carry PCM lattice fields.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FLASHC="${FLASHC:-$ROOT/flashc}"
BIN_DIR="${BIN_DIR:-$ROOT/.test_bin}"

fail() {
    echo "FAIL: hardware_pcm_gate — $1"
    exit 1
}

echo "== hardware_pcm_gate: emit manifest with PCM fields =="
rm -f "$ROOT/examples/ternary_logic.fimg" "$ROOT/examples/flash.manifest"
if ! "$FLASHC" "$ROOT/examples/ternary_logic.flash" --emit-manifest --emit-fimg --target ppu32 >/dev/null 2>&1; then
    fail "flashc could not emit ternary_logic manifest+fimg"
fi

mani="$ROOT/examples/flash.manifest"
if [[ ! -f "$mani" ]]; then
    fail "flash.manifest missing"
fi
if ! grep -q 'pcm_row = ' "$mani"; then
    fail "manifest missing pcm_row fields"
fi
if ! grep -q 'lattice_op = ' "$mani"; then
    fail "manifest missing lattice_op fields"
fi
if ! grep -q 'op = "trit_add"' "$mani"; then
    fail "manifest missing trit_add segment"
fi

fimg="$ROOT/examples/ternary_logic.fimg"
if [[ ! -f "$fimg" ]]; then
    fail "ternary_logic.fimg missing"
fi
if ! grep -q '# flash image v1' "$fimg"; then
    fail "fimg missing v1 header"
fi
if ! grep -q 'target = "ppu32"' "$fimg"; then
    fail "fimg missing ppu32 target"
fi
if ! grep -q '\[extern\]' "$fimg"; then
    fail "fimg missing extern table"
fi

echo "PASS: hardware_pcm_gate (PCM manifest + FIMG v1)"