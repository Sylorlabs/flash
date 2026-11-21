#!/usr/bin/env bash
# tests/ppu32_slice_gate.sh — P1 vertical slice: hand-verified state +
# differential check of qemu_ppu32 vs the independent ref_interp,
# including fault paths.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN_DIR="${BIN_DIR:-$ROOT/.test_bin}"
FLASHZ="${FLASHZ:-$ROOT/flash/toolchain/flashz}"
QEMU="$BIN_DIR/qemu_ppu32"
REF="$BIN_DIR/ref_interp"
mkdir -p "$BIN_DIR"
fail() { echo "FAIL: ppu32_slice_gate — $1"; exit 1; }

"$FLASHZ" "$ROOT/flasht/qemu_ppu32.zag" -o "$QEMU" >/dev/null 2>&1 || fail "qemu build"
"$FLASHZ" "$ROOT/flasht/ref_interp.zag" -o "$REF" >/dev/null 2>&1 || fail "ref_interp build"

FIX="$ROOT/tests/ppu32/slice.ppu32"
"$QEMU" "$FIX" --dump > "$BIN_DIR/slice.q.out" || fail "qemu run"
"$REF" "$FIX" > "$BIN_DIR/slice.r.out" || fail "ref run"
tail -n +2 "$BIN_DIR/slice.q.out" > "$BIN_DIR/slice.q2.out"   # drop "qemu_ppu32: ok"
diff -u "$BIN_DIR/slice.q2.out" "$BIN_DIR/slice.r.out" >/dev/null || {
    diff -u "$BIN_DIR/slice.q2.out" "$BIN_DIR/slice.r.out" | head -30
    fail "qemu/ref_interp machine-state divergence"
}
echo "PASS: qemu ≡ ref_interp on slice (all 14 classes)"

# hand-verified end state (independent of either interpreter)
OUT="$BIN_DIR/slice.r.out"
exp() { grep -qF "$1" "$OUT" || fail "expected state missing: $1"; }
exp 'treg[2]=6'      # 0t++ (4) + 0t+- (2)
exp 'treg[3]=8'      # 4 * 2
exp 'treg[5]=-4'     # neg(4)
exp 'treg[6]=3'      # cons(++,+-) = 0t+0
exp 'treg[9]=7'      # 0t+0(3)+0t++(4), trit0 steered by +couple: 3+4=7
exp 'pcm[5,2]=150'
exp 'laser[1]=750'
exp 'voxel[2,3,1] pol=1 phase=1500 amp=400 occ=1'
exp 'lattice[0,0,0] occ=1'
exp 'lattice[3,0,0] occ=1'
exp 'fault=0'
echo "PASS: slice end-state matches hand verification"

# treg[10] voxel word nonzero (packed read)
grep -qE 'treg\[10\]=[0-9]+' "$OUT" || fail "RawReadVoxel wrote nothing to treg[10]"
echo "PASS: RawReadVoxel"

# fault paths
set +e
"$QEMU" "$ROOT/tests/ppu32/slice_fault.ppu32" > "$BIN_DIR/f.q.out" 2>&1
qc=$?
"$REF" "$ROOT/tests/ppu32/slice_fault.ppu32" > "$BIN_DIR/f.r.out" 2>&1
rc=$?
set -e
[ "$qc" -eq 2 ] || fail "qemu fault exit=$qc want 2"
[ "$rc" -eq 2 ] || fail "ref fault exit=$rc want 2"
grep -q 'fault 3' "$BIN_DIR/f.q.out" || fail "qemu missing F_MODE"
grep -q 'fault=3' "$BIN_DIR/f.r.out" || fail "ref missing F_MODE"
set +e
"$QEMU" "$ROOT/tests/ppu32/slice_bounds.ppu32" > "$BIN_DIR/b.q.out" 2>&1; qc=$?
"$REF" "$ROOT/tests/ppu32/slice_bounds.ppu32" > "$BIN_DIR/b.r.out" 2>&1; rc=$?
set -e
[ "$qc" -eq 2 ] && grep -q 'fault 2' "$BIN_DIR/b.q.out" || fail "qemu bounds path"
grep -q 'fault=2' "$BIN_DIR/b.r.out" || fail "ref bounds path"
echo "PASS: fault paths (F_MODE, F_BOUNDS) identical on both interpreters"

echo "PASS: ppu32_slice_gate"
