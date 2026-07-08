#!/usr/bin/env bash
# tests/flasht_bootstrap_gate.sh — Phase 6 gate: flasht hardware path never invokes znc.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN_DIR="${BIN_DIR:-$ROOT/.test_bin}"
FLASHT="${FLASHT:-$ROOT/flash/toolchain/flasht}"
FLASHC="${FLASHC:-$ROOT/flashc}"
TRAP_ZNC="$BIN_DIR/trap_znc_gate"

mkdir -p "$BIN_DIR"

fail() {
    echo "FAIL: flasht_bootstrap_gate — $1"
    exit 1
}

echo "== flasht_bootstrap_gate: flasht present =="
if [[ ! -f "$FLASHT" ]]; then
    fail "flash/toolchain/flasht missing"
fi
if [[ ! -x "$FLASHT" ]]; then
    fail "flash/toolchain/flasht is not executable"
fi
echo "PASS: flash/toolchain/flasht present and executable"

echo "== flasht_bootstrap_gate: znc trap installed =="
cat > "$TRAP_ZNC" <<'EOF'
#!/usr/bin/env bash
echo "FAIL: znc was invoked: $*" >&2
exit 99
EOF
chmod +x "$TRAP_ZNC"
export ZNC="$TRAP_ZNC"
export FLASHZ="$TRAP_ZNC"

if [[ ! -x "$FLASHC" ]]; then
    fail "flashc missing (required for hardware targets)"
fi

echo "== flasht_bootstrap_gate: --target fimg never calls znc =="
rm -f "$ROOT/examples/ternary_logic.fimg" "$ROOT/examples/flash.manifest"
if ! "$FLASHT" "$ROOT/examples/ternary_logic.flash" --emit-fimg --target fimg >/dev/null 2>&1; then
    fail "flasht --target fimg --emit-fimg failed"
fi
if [[ ! -f "$ROOT/examples/ternary_logic.fimg" ]]; then
    fail "ternary_logic.fimg not produced via flasht"
fi
if ! grep -q '# flash image v1' "$ROOT/examples/ternary_logic.fimg"; then
    fail "fimg missing v1 header"
fi
if ! grep -q 'target = "fimg"' "$ROOT/examples/ternary_logic.fimg"; then
    fail "fimg missing fimg target"
fi
echo "PASS: --target fimg completed without invoking znc"

echo "== flasht_bootstrap_gate: --target ppu32 never calls znc =="
rm -f "$ROOT/examples/ternary_logic.fimg" "$ROOT/examples/flash.manifest"
if ! "$FLASHT" "$ROOT/examples/ternary_logic.flash" --emit-fimg --target ppu32 >/dev/null 2>&1; then
    fail "flasht --target ppu32 --emit-fimg failed"
fi
if [[ ! -f "$ROOT/examples/ternary_logic.fimg" ]]; then
    fail "ternary_logic.fimg not produced for ppu32 target"
fi
if ! grep -q 'target = "ppu32"' "$ROOT/examples/ternary_logic.fimg"; then
    fail "fimg missing ppu32 target"
fi
echo "PASS: --target ppu32 completed without invoking znc"

echo "== flasht_bootstrap_gate: --target engine never calls znc =="
rm -f "$ROOT/examples/ternary_logic.fimg" "$ROOT/examples/flash.manifest"
if ! "$FLASHT" "$ROOT/examples/ternary_logic.flash" --emit-fimg --target engine >/dev/null 2>&1; then
    fail "flasht --target engine --emit-fimg failed"
fi
if [[ ! -f "$ROOT/examples/ternary_logic.fimg" ]]; then
    fail "ternary_logic.fimg not produced for engine target"
fi
if ! grep -q 'target = "engine"' "$ROOT/examples/ternary_logic.fimg"; then
    fail "fimg missing engine target"
fi
echo "PASS: --target engine completed without invoking znc"

echo "== flasht_bootstrap_gate: default path delegates to flashz =="
unset ZNC FLASHZ
if ! "$FLASHT" "$ROOT/tests/ternary_smoke.zag" -o "$BIN_DIR/flasht_smoke.out" >/dev/null 2>&1; then
    fail "flasht default delegation to flashz failed"
fi
if [[ ! -x "$BIN_DIR/flasht_smoke.out" ]]; then
    fail "flasht default path did not produce executable"
fi
if ! "$BIN_DIR/flasht_smoke.out" 2>&1 | grep -qF "ternary_smoke: ok"; then
    fail "flasht default smoke run failed"
fi
echo "PASS: default path delegates to bundled flashz"

echo "== flasht_bootstrap_gate: manifest_loader parses [engine] =="
ENGINE_FIMG="$BIN_DIR/engine_gate.fimg"
cat > "$ENGINE_FIMG" <<'EOF'
# flash engine image v1
FLASH_PLATFORM=engine
target = "engine"
name = "ternary_logic"
format = "engine-v1"
[engine]
platform = "engine"
FLASH_PLATFORM = "engine"
gpu = 1
compute_route = 2
boot_abi = "flash_rt-v1"
[fimg]
# flash image v1
target = "engine"
name = "ternary_logic"
format = "fimg-v1"
[manifest]
name = "ternary_logic"
version = "0.1.0"
peak_power_mw = 1000m
dominant_pol = 0
[paths]
segment 0 { op = "trit_add", from = (0m, 0m, 0m), to = (0m, 0m, 0m), power_mw = 1000m, pol = 0, pcm_row = 1, pcm_col = 0, phase_a_rad = 0u, phase_b_rad = 0u, phase_result_rad = 0u, lattice_op = 3 }
[pcm_ops]
op 0 { class = 1, tag = "trit_add", pcm_row = 1, pcm_col = 0, lattice_op = 3 }
EOF
if ! "$ROOT/flash/toolchain/flashz" "$ROOT/flash/rayos/manifest_loader.zag" -o "$BIN_DIR/manifest_loader_gate.out" >/dev/null 2>&1; then
    fail "manifest_loader.zag did not compile"
fi
cp -f "$ENGINE_FIMG" "$ROOT/examples/ternary_logic.fimg"
ML_OUT="$(cd "$ROOT" && "$BIN_DIR/manifest_loader_gate.out" 2>&1 || true)"
if ! grep -qF 'manifest_loader: engine platform, gpu=1' <<< "$ML_OUT"; then
    fail "manifest_loader did not report engine gpu metadata"
fi
if ! grep -qF 'compute_route=2' <<< "$ML_OUT"; then
    fail "manifest_loader did not report engine compute_route metadata"
fi
echo "PASS: manifest_loader parses [engine] section"

echo "== flasht_bootstrap_gate: emit_engine.flash present =="
if [[ ! -f "$ROOT/flasht/emit_engine.flash" ]]; then
    fail "flasht/emit_engine.flash missing"
fi
if ! grep -qF 'FLASH_PLATFORM=engine' "$ROOT/flasht/emit_engine.flash"; then
    fail "emit_engine.flash missing FLASH_PLATFORM=engine boot header"
fi
if ! grep -qF '[engine]' "$ROOT/flasht/emit_engine.flash"; then
    fail "emit_engine.flash missing [engine] section emitter"
fi
echo "PASS: flasht/emit_engine.flash ships engine boot metadata"

echo "PASS: flasht_bootstrap_gate"