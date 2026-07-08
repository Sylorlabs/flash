#!/usr/bin/env bash
# tests/flasht_ir_gate.sh — Phase 1 gate: flasht IR (.fir) emission for PPU32 targets.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FLASHC="${FLASHC:-$ROOT/flashc}"
BIN_DIR="${BIN_DIR:-$ROOT/.test_bin}"

fail() {
    echo "FAIL: flasht_ir_gate — $1"
    exit 1
}

echo "== flasht_ir_gate: ppu32 target emits .fir IR =="
rm -f "$ROOT/examples/ternary_logic.fir" "$ROOT/examples/ternary_logic_shim.zag"

if ! "$FLASHC" "$ROOT/examples/ternary_logic.flash" --target ppu32 >/dev/null 2>&1; then
    fail "flashc could not compile ternary_logic with --target ppu32"
fi

fir="$ROOT/examples/ternary_logic.fir"
if [[ ! -f "$fir" ]]; then
    fail "ternary_logic.fir missing (auto-emit for ppu32 target)"
fi
if ! grep -q '# flash ir v1' "$fir"; then
    fail ".fir missing v1 header"
fi
if ! grep -q 'target = "ppu32"' "$fir"; then
    fail ".fir missing ppu32 target"
fi
if ! grep -q '\[ir\]' "$fir"; then
    fail ".fir missing [ir] section"
fi
if ! grep -q 'kind = "route"' "$fir"; then
    fail ".fir missing route IR nodes"
fi
if ! grep -q 'op = "trit_add"' "$fir"; then
    fail ".fir missing trit_add route node"
fi

echo "PASS: flasht_ir_gate (ppu32 .fir IR emission)"