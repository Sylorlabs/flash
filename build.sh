#!/usr/bin/env bash
# build.sh — bootstrap flashc1 and run the comprehensive Flash/Zag test suite.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
FLASHT="${FLASHT:-$ROOT/flash/toolchain/flasht}"
FLASHZ="${FLASHZ:-$ROOT/flash/toolchain/flashz}"
# Bootstrap always uses flashz for .zag → native; flasht is for hw targets via flashc.
ZNC="${ZNC:-$FLASHZ}"
export FLASHT FLASHZ ZNC
export FLASH_RT="${FLASH_RT:-sim}"
export FLASH_PLATFORM="${FLASH_PLATFORM:-emulation}"
BIN_DIR="$ROOT/.test_bin"
FLASHC0="$BIN_DIR/flashc0"
FLASHC1_SHIM="$ROOT/selfhost/flashc1_shim.zag"

mkdir -p "$BIN_DIR"
rm -f /tmp/flash_rt_state.ptr

echo "== flash: bootstrap compiler (standalone) =="
if [[ ! -x "$FLASHZ" ]]; then
    if [[ -x "$FLASHT" ]]; then
        FLASHZ="$FLASHT"
        ZNC="$FLASHT"
    else
        echo "::error::flash/toolchain/flashz missing — Flash ships its own toolchain"
        exit 1
    fi
fi

cp -f "$FLASHC1_SHIM" "$BIN_DIR/flashc1_shim.good"

echo "step 1: building flashc1 (self-hosted compiler)"
BUILT=0
if [[ -x "$ROOT/flashc" ]] && "$ROOT/flashc" "$ROOT/selfhost/flashc1.flash" -o "$BIN_DIR/flashc1"; then
    BUILT=1
    echo "step 1: rebuilt flashc1 via existing self-hosted flashc"
elif [[ -x "$BIN_DIR/flashc1_old" ]] && "$BIN_DIR/flashc1_old" "$ROOT/selfhost/flashc1.flash" -o "$BIN_DIR/flashc1"; then
    BUILT=1
    echo "step 1: built flashc1 via cached seed"
fi
if [[ "$BUILT" -eq 0 ]]; then
    echo "step 1 (fallback): flashz → flashc1 from cached shim"
    cp -f "$BIN_DIR/flashc1_shim.good" "$FLASHC1_SHIM"
    "$FLASHZ" "$FLASHC1_SHIM" -o "$BIN_DIR/flashc1"
fi

cp -f "$BIN_DIR/flashc1" "$ROOT/flashc"
chmod +x "$ROOT/flashc"
# Refresh the cached good shim after a successful source rebuild.
if [[ "$BUILT" -eq 1 ]]; then
    # flashc1 writes fla_shim.zag; keep golden fixpoint reference in sync.
    if [[ -f "$ROOT/selfhost/fla_shim.zag" ]]; then
        cp -f "$ROOT/selfhost/fla_shim.zag" "$FLASHC1_SHIM"
    fi
    cp -f "$FLASHC1_SHIM" "$BIN_DIR/flashc1_shim.good"
fi
cp -f "$BIN_DIR/flashc1" "$BIN_DIR/flashc1_old"
echo "flashc is now flashc1 (self-hosted)"

echo
FLASHC="$ROOT/flashc" "$ROOT/tests/run_all.sh"