#!/usr/bin/env bash
# An OS target must either produce executable kernel code or fail explicitly.
# Empty photonic images and hosted binaries are not acceptable fallbacks.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

cat >"$TMP/kernel.flash" <<'EOF'
fn main() i32 {
    return 0;
}
EOF

if "$ROOT/flashc" "$TMP/kernel.flash" --target ppu32 >"$TMP/ppu.log" 2>&1; then
    echo "FAIL: ordinary Flash program produced a PPU32 executable image"
    exit 1
fi
grep -qF 'general Flash functions and freestanding kernels are not yet lowered to PPU32' "$TMP/ppu.log"
test ! -e "$TMP/kernel.ppu32"

if "$ROOT/flashc" "$TMP/kernel.flash" --target rayos-ternary >"$TMP/rayos.log" 2>&1; then
    echo "FAIL: unavailable RayOS target was accepted"
    exit 1
fi
grep -qF 'RayOS freestanding kernel backend is not implemented' "$TMP/rayos.log"
grep -qF 'refusing a hosted or empty fallback' "$TMP/rayos.log"

echo "PASS: os_target_contract_gate (unsupported kernel targets fail closed)"
