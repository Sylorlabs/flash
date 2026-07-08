#!/usr/bin/env bash
# tests/flasht_ppu32_gate.sh — flasht Phase 3: emit .ppu32 and run qemu interpreter.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FLASHC="${FLASHC:-$ROOT/flashc}"
FLASHZ="${FLASHZ:-${ZNC:-$ROOT/flash/toolchain/flashz}}"
BIN_DIR="${BIN_DIR:-$ROOT/.test_bin}"

fail() {
    echo "FAIL: flasht_ppu32_gate — $1"
    exit 1
}

mkdir -p "$BIN_DIR"

echo "== flasht_ppu32_gate: emit ppu32 for ternary_logic =="
rm -f "$ROOT/examples/ternary_logic.ppu32" "$ROOT/examples/ternary_logic.fimg"
if ! "$FLASHC" "$ROOT/examples/ternary_logic.flash" --target ppu32 --emit-fimg >/dev/null 2>&1; then
    fail "flashc could not emit ternary_logic .ppu32"
fi

ppu="$ROOT/examples/ternary_logic.ppu32"
if [[ ! -f "$ppu" ]]; then
    fail "ternary_logic.ppu32 missing"
fi
if ! grep -q '# ppu32 assembly v1' "$ppu"; then
    fail "ppu32 missing v1 header"
fi
if ! grep -q 'PcmPhaseShift' "$ppu"; then
    fail "ppu32 missing PcmPhaseShift insn"
fi
if ! grep -q 'LatticeRoute' "$ppu"; then
    fail "ppu32 missing LatticeRoute insn"
fi

fimg="$ROOT/examples/ternary_logic.fimg"
if [[ ! -f "$fimg" ]]; then
    fail "ternary_logic.fimg missing"
fi
if ! grep -q '# section .flash.manifest' "$fimg"; then
    fail "fimg missing ELF section marker"
fi
if ! grep -q '\[pcm_ops\]' "$fimg"; then
    fail "fimg missing [pcm_ops] section"
fi
if ! grep -q '\[ir\]' "$fimg"; then
    fail "fimg missing [ir] section"
fi

echo "== flasht_ppu32_gate: run qemu_ppu32 interpreter =="
qemu_bin="$BIN_DIR/qemu_ppu32"
rm -f "$qemu_bin"
if ! "$FLASHZ" "$ROOT/flasht/qemu_ppu32.zag" -o "$qemu_bin" >/dev/null 2>&1; then
    fail "flashz could not build qemu_ppu32"
fi
if [[ ! -x "$qemu_bin" ]]; then
    fail "qemu_ppu32 binary missing"
fi
if ! "$qemu_bin" "$ppu" 2>&1 | grep -qF 'qemu_ppu32: ok'; then
    fail "qemu_ppu32 did not report ok"
fi

echo "PASS: flasht_ppu32_gate (ppu32 emit + qemu interpreter)"