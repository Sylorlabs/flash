#!/usr/bin/env bash
# tests/run_all.sh — comprehensive Flash/Zag test runner (canonical entry point).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FLASHZ="${FLASHZ:-${ZNC:-$ROOT/flash/toolchain/flashz}}"
ZNC="$FLASHZ"
BIN_DIR="${BIN_DIR:-$ROOT/.test_bin}"
FLASHC="${FLASHC:-$ROOT/flashc}"
RT_STATE_CACHE_GLOB="/tmp/flash_rt_state.*.ptr"

mkdir -p "$BIN_DIR"
rm -f $RT_STATE_CACHE_GLOB /tmp/flash_rt_state.ptr

TESTS_RUN=0
TESTS_PASSED=0
FAILURES=0

pass() {
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

fail() {
    local name="$1"
    echo "FAIL: $name"
    FAILURES=$((FAILURES + 1))
}

assert_output_contains() {
    local name="$1"
    local pattern="$2"
    local output="$3"
    if ! grep -qF "$pattern" <<< "$output"; then
        echo "FAIL: $name — expected output to contain: $pattern"
        echo "--- output ---"
        echo "$output"
        echo "--------------"
        FAILURES=$((FAILURES + 1))
        return 1
    fi
    return 0
}

run_zag_test() {
    local name="$1"
    local src="$2"
    local out="$BIN_DIR/$(basename "$src" .zag).out"
    shift 2

    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    rm -f "$out"
    if ! "$ZNC" "$src" -o "$out"; then
        fail "$name (compile)"
        return 1
    fi

    rm -f $RT_STATE_CACHE_GLOB /tmp/flash_rt_state.ptr
    local run_output
    if ! run_output="$("$out" 2>&1)"; then
        fail "$name (run)"
        echo "$run_output"
        return 1
    fi

    if (("$#" > 0)); then
        local pat
        for pat in "$@"; do
            if ! assert_output_contains "$name" "$pat" "$run_output"; then
                fail "$name (assertion)"
                return 1
            fi
        done
    fi

    pass
    echo
}

run_flashc_example() {
    local name="$1"
    local src="$2"
    shift 2

    local stem
    stem="$(basename "$src")"
    stem="${stem%.flash.zag}"
    stem="${stem%.flash}"
    local out="$BIN_DIR/${stem}.flash.out"

    echo "== test [$((TESTS_RUN + 1))]: flashc $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    rm -f $RT_STATE_CACHE_GLOB /tmp/flash_rt_state.ptr
    local run_output
    if ! run_output="$(cd "$ROOT" && "$FLASHC" "$src" -o "$out" --emit-manifest --run 2>&1)"; then
        fail "flashc $name"
        echo "$run_output"
        return 1
    fi

    if (("$#" > 0)); then
        local pat
        for pat in "$@"; do
            if ! assert_output_contains "flashc $name" "$pat" "$run_output"; then
                fail "flashc $name (assertion)"
                return 1
            fi
        done
    fi

    pass
    echo
}

run_flashc_check_reject() {
    local name="$1"
    local src="$2"

    echo "== test [$((TESTS_RUN + 1))]: flashc --check reject $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    if (cd "$ROOT" && "$FLASHC" "$src" --check >/dev/null 2>&1); then
        fail "flashc --check reject $name (expected failure)"
        return 1
    fi

    pass
    echo
}

run_flashc_check_accept() {
    local name="$1"
    local src="$2"

    echo "== test [$((TESTS_RUN + 1))]: flashc --check accept $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    if ! (cd "$ROOT" && "$FLASHC" "$src" --check >/dev/null 2>&1); then
        fail "flashc --check accept $name"
        return 1
    fi

    pass
    echo
}

run_flashc_critic_suite() {
    local CRITIC_DIR="$ROOT/tests/critic"
    if [[ ! -d "$CRITIC_DIR" ]]; then
        echo "== flashc critic: (not present — skipped) =="
        echo
        return 0
    fi

    if [[ ! -x "$FLASHC" ]]; then
        echo "== building flashc =="
        "$ZNC" "$ROOT/flash/compiler/flashc.zag" -o "$FLASHC" 2>/dev/null || true
        if [[ ! -x "$FLASHC" ]] && [[ -x "$ROOT/.test_bin/flashc1" ]]; then
            cp "$ROOT/.test_bin/flashc1" "$FLASHC"
            chmod +x "$FLASHC"
        fi
        if [[ ! -x "$FLASHC" ]] && [[ -x "$ROOT/.test_bin/flashc1_old" ]]; then
            "$ROOT/.test_bin/flashc1_old" "$ROOT/selfhost/flashc1.flash" -o "$FLASHC"
            chmod +x "$FLASHC"
        fi
        echo
    fi

    if [[ ! -x "$FLASHC" ]]; then
        echo "== flashc critic: (flashc missing — skipped) =="
        echo
        return 0
    fi

    run_flashc_check_reject "harsh_group_scope" \
        "$CRITIC_DIR/harsh_group_scope.flash"
    run_flashc_check_reject "harsh_catch_type" \
        "$CRITIC_DIR/harsh_catch_type.flash"
    run_flashc_check_reject "harsh_defer" \
        "$CRITIC_DIR/harsh_defer.flash"
    run_flashc_check_accept "harsh_error_sugar" \
        "$CRITIC_DIR/harsh_error_sugar.flash"
    run_flashc_check_reject "harsh_error_catch" \
        "$CRITIC_DIR/harsh_error_catch.flash"
}

run_fuzz_lex_smoke() {
    local script="$ROOT/tests/fuzz_lex_smoke.sh"
    if [[ ! -f "$script" ]]; then
        echo "== fuzz_lex_smoke: (not present — skipped) =="
        echo
        return 0
    fi

    echo "== test [$((TESTS_RUN + 1))]: fuzz_lex_smoke =="
    TESTS_RUN=$((TESTS_RUN + 1))

    if ! (cd "$ROOT" && FLASHC="$FLASHC" FLASHZ="$FLASHZ" bash "$script"); then
        fail "fuzz_lex_smoke"
        return 1
    fi

    pass
    echo
}

run_critic_suite() {
    local CRITIC_DIR="$ROOT/tests/critic"
    if [[ ! -d "$CRITIC_DIR" ]]; then
        echo "== tests/critic: (not present — skipped) =="
        echo
        return 0
    fi

    shopt -s nullglob
    local critic_tests=("$CRITIC_DIR"/*.zag)
    shopt -u nullglob
    if ((${#critic_tests[@]} == 0)); then
        echo "== tests/critic: (directory present, no .zag tests) =="
        echo
        return 0
    fi

    for critic_src in "${critic_tests[@]}"; do
        run_zag_test "critic/$(basename "$critic_src")" "$critic_src"
    done
}

echo "== flash: comprehensive test runner =="
echo "root: $ROOT"
echo "znc:  $ZNC"
echo

if [[ "${FLASH_CRITIC_ONLY:-0}" == "1" ]]; then
    run_critic_suite
    run_flashc_critic_suite
    run_fuzz_lex_smoke
    echo "============================================"
    echo "RESULT: $TESTS_PASSED / $TESTS_RUN passed"
    if [[ "$FAILURES" -gt 0 ]]; then
        echo "FAILURES: $FAILURES"
        exit 1
    fi
    echo "== flash: critic tests passed =="
    exit 0
fi

# ── Zag unit / smoke tests ───────────────────────────────────────────────────
run_zag_test "ternary_smoke" \
    "$ROOT/tests/ternary_smoke.zag" \
    "ternary_smoke: ok"

run_zag_test "trit_math_correct" \
    "$ROOT/tests/integration/trit_math_correct.zag" \
    "trit_math_correct: ok"

run_zag_test "pipeline_parity" \
    "$ROOT/tests/integration/pipeline_parity.zag" \
    "pipeline_parity: ok"

run_zag_test "runtime_test" \
    "$ROOT/flash/runtime/runtime_test.zag" \
    "runtime_test: ok"

run_zag_test "flash_lex_test"             "$ROOT/flash/compiler/flash_lex_test.zag" "flash_lex_test: ok (33 assertions)"
run_zag_test "flash_parse_test"           "$ROOT/flash/compiler/flash_parse_test.zag" "flash_parse_test: ok"
run_zag_test "flash_sema_test"            "$ROOT/flash/compiler/flash_sema_test.zag" "flash_sema_test: all passed"
run_zag_test "flash_codegen_test"         "$ROOT/flash/compiler/flash_codegen_test.zag" "flash_codegen_test: ok"
run_zag_test "flashc_test"                "$ROOT/flash/compiler/flashc_test.zag" "flashc_test: ok (3 programs + 3 reject checks + --run)"
run_zag_test "catalog_test"               "$ROOT/flash/types/numeric/catalog_test.zag" "catalog_test: ok"
run_zag_test "matrix_test"                "$ROOT/flash/runtime/matrix_test.zag" "matrix_test: ok"
run_zag_test "spatial_test"               "$ROOT/zag-spatial/spatial_test.zag"

if [[ -f "$ROOT/tests/optical_mat_test.zag" ]]; then
    run_zag_test "optical_mat_test" \
        "$ROOT/tests/optical_mat_test.zag" \
        "optical_mat_test: ok"
else
    echo "== optical_mat_test: (not present — skipped) =="
    echo
fi

if [[ -f "$ROOT/tests/optical_mat_manifest_test.zag" ]]; then
    run_zag_test "optical_mat_manifest_test" \
        "$ROOT/tests/optical_mat_manifest_test.zag" \
        "optical_mat_manifest_test: ok"
else
    echo "== optical_mat_manifest_test: (not present — skipped) =="
    echo
fi

# ── tests/critic/* (optional) ────────────────────────────────────────────────
run_critic_suite

# ── flashc --check adversarial critic fixtures ───────────────────────────────
run_flashc_critic_suite
run_fuzz_lex_smoke

# ── flashc example programs ──────────────────────────────────────────────────
if [[ ! -x "$FLASHC" ]]; then
    echo "== building flashc =="
    "$ZNC" "$ROOT/flash/compiler/flashc.zag" -o "$FLASHC"
    echo
fi

run_flashc_example "full_demo" \
    "$ROOT/examples/full_demo.flash" \
    "voxel pol=" \
    "trit sum=0" \
    "full_demo: ok"

run_flashc_example "numeric_spectrum" \
    "$ROOT/examples/numeric_spectrum.flash" \
    "numeric sat_i16 i_lane=42" \
    "numeric sat_i16 u_lane=32000" \
    "numeric_spectrum: ok"

run_flashc_example "ternary_logic" \
    "$ROOT/examples/ternary_logic.flash" \
    "trit final=0" \
    "ternary_logic: ok"

run_flashc_example "raw_chamber" \
    "$ROOT/examples/raw_chamber.flash" \
    "voxel pol=-1" \
    "phase_milli=1570" \
    "amp_milli=1" \
    "raw_chamber: ok"

run_flashc_example "gp_demo (full language)" \
    "$ROOT/examples/gp_demo.flash" \
    "fib10=55" \
    "words=5" \
    "head=flash" \
    "hits=2" \
    "gp trit s=0" \
    "gp_demo: ok"

run_flashc_example "widen_demo (unions/methods/generics/optionals)" \
    "$ROOT/examples/widen_demo.flash" \
    "doubled=4" \
    "first=9" \
    "opt=41,7" \
    "cap=41" \
    "area=27" \
    "area2=16" \
    "widen_demo: ok"

run_flashc_example "lang_ext_demo (pub/extern/error sets)" \
    "$ROOT/examples/lang_ext_demo.flash" \
    "greet=pub_fn" \
    "point=3,4" \
    "color=0" \
    "errors=NotFound,BadInput" \
    "lang_ext_demo: ok"

run_flashc_example "annot_demo (@photonic fn / @realtime struct)" \
    "$ROOT/examples/annot_demo.flash" \
    "annot=42" \
    "annot_demo: ok"

run_flashc_example "error_catch_demo (try/catch/orelse/!T)" \
    "$ROOT/examples/error_catch_demo.flash" \
    "try_val=10" \
    "catch_fb=-1" \
    "catch_cap=1" \
    "orelse_val=4" \
    "orelse_fb=99" \
    "errors=DivideByZero,Negative" \
    "error_catch_demo: ok"

run_flashc_example "gfx_demo (softgpu fallback driver)" \
    "$ROOT/examples/gfx_demo.flash" \
    "engine: platform=emulation" \
    "gfx=96x64" \
    "checksum=15156706" \
    "gfx_demo: ok"

run_flashc_example "ui80_demo (1980s UI via softgpu)" \
    "$ROOT/examples/ui80_demo.flash" \
    "checksum=11214189" \
    "ui80_demo: ok"

run_flashc_example "modern_demo (vgpu competitive tier)" \
    "$ROOT/examples/modern_demo.flash" \
    "engine: platform=emulation" \
    "checksum=8202754" \
    "modern_demo: ok"

run_flashc_example "engine_control (vgpu v2: transforms/radial/blur/arcs)" \
    "$ROOT/examples/engine_control.flash" \
    "checksum=575624" \
    "engine_control: ok"

run_flashc_example "texture_demo (vgpu v3: clipping + textures)" \
    "$ROOT/examples/texture_demo.flash" \
    "roundtrip=480x300" \
    "checksum=12322534" \
    "texture_demo: ok"

run_flashc_example "layers_demo (vgpu v4: strokes + layers)" \
    "$ROOT/examples/layers_demo.flash" \
    "checksum=13604817" \
    "layers_demo: ok"

run_flashc_example "type_demo (vgpu v5: vector text + miter joins)" \
    "$ROOT/examples/type_demo.flash" \
    "checksum=7240638" \
    "type_demo: ok"

run_flashc_example "tunit_demo (native balanced-ternary integer units)" \
    "$ROOT/examples/tunit_demo.flash" \
    "t9_sum=0" \
    "t9_max=9841" \
    "t27_prod=96845281" \
    "t27_sum=0" \
    "Type{ name=t9 kind=13 width=9 signed=1 storage=2 }" \
    "Type{ name=t27 kind=13 width=27 signed=1 storage=8 }" \
    "tunit_demo: ok"

run_flashc_example "face_demo (vgpu v6: filled faces + unified dash + damage)" \
    "$ROOT/examples/face_demo.flash" \
    "damage_valid=1" \
    "damage=403,23,436,56" \
    "damage_checksum=6623998" \
    "checksum=706490" \
    "face_demo: ok"

run_flashc_example "ui_engine_demo (retained UI + damage tracking)" \
    "$ROOT/examples/ui_engine_demo.flash" \
    "f0 full=153600" \
    "f1 damage=27952 (18 pct of full)" \
    "f2 damage=0" \
    "checksum=5176704" \
    "ui_engine_demo: ok"

run_flashc_example "selfhost lexer (Flash-in-Flash)" \
    "$ROOT/selfhost/lexer.flash" \
    "words=12" \
    "total=19" \
    "selfhost_lexer: ok"

run_flashc_example "selfhost parser (Flash-in-Flash)" \
    "$ROOT/selfhost/parser.flash" \
    "program=demo" \
    "stmts=7" \
    "selfhost_parser: ok"

# examples/stdlib_demo.flash uses @import("std/...") — resolve via selfhost/std.
if [[ ! -e "$ROOT/examples/std" ]]; then
    ln -sf ../selfhost/std "$ROOT/examples/std"
fi

run_flashc_example "stdlib_demo (fmt + vec stdlib)" \
    "$ROOT/examples/stdlib_demo.flash" \
    "len=3" \
    "mid=8" \
    "x+y" \
    "stdlib_demo: ok"

if [[ -f "$ROOT/examples/optical_mat.flash" ]]; then
    run_flashc_example "optical_mat" \
        "$ROOT/examples/optical_mat.flash" \
        "mat3_segments=9" \
        "trit result=0" \
        "optical_mat: ok"
else
    echo "== flashc optical_mat: (not present — skipped) =="
    echo
fi

# ── spatial-library port parity (Flash ports vs Zag originals) ──────────────
run_port_parity() {
    local name="port parity (selfhost/lib Flash ports vs zag-spatial originals)"
    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    if ! "$ZNC" "$ROOT/tests/portparity/spatial_probe0.zag" -o "$BIN_DIR/spatial_probe0.out" >/dev/null 2>&1; then
        fail "$name (znc build of probe0)"
        return 1
    fi
    if ! "$BIN_DIR/spatial_probe0.out" > "$BIN_DIR/probe0.txt" 2>&1; then
        fail "$name (probe0 run)"
        return 1
    fi
    local build_output
    if ! build_output="$(cd "$ROOT" && "$FLASHC" selfhost/lib/spatial_probe.flash -o "$BIN_DIR/spatial_probe1.out" 2>&1)"; then
        fail "$name (flash port build)"
        echo "$build_output"
        return 1
    fi
    if ! "$BIN_DIR/spatial_probe1.out" > "$BIN_DIR/probe1.txt" 2>&1; then
        fail "$name (probe1 run)"
        return 1
    fi
    if ! cmp -s "$BIN_DIR/probe0.txt" "$BIN_DIR/probe1.txt"; then
        fail "$name (output diff)"
        diff "$BIN_DIR/probe0.txt" "$BIN_DIR/probe1.txt" | head -10
        return 1
    fi
    pass
    echo
}
run_port_parity

# ── stage-1 self-hosted pipeline (flashc1 written in Flash) ─────────────────
run_annot_emit_test() {
    local name="stage-1 annot emit (@photonic/@realtime in --emit-zag shim)"
    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    if [[ ! -x "$ROOT/.test_bin/flashc1" ]]; then
        if ! (cd "$ROOT" && "$FLASHC" selfhost/flashc1.flash -o .test_bin/flashc1 >/dev/null 2>&1); then
            fail "$name (build flashc1)"
            return 1
        fi
    fi

    if ! (cd "$ROOT" && .test_bin/flashc1 examples/annot_demo.flash --emit-zag >/dev/null 2>&1); then
        fail "$name (stage-1 emit)"
        return 1
    fi

    local shim="$ROOT/examples/annot__shim.zag"
    if ! grep -qF "@realtime" "$shim"; then
        fail "$name (@realtime missing from shim)"
        head -20 "$shim"
        return 1
    fi
    if ! grep -qF "@photonic" "$shim"; then
        fail "$name (@photonic missing from shim)"
        head -20 "$shim"
        return 1
    fi

    if ! (cd "$ROOT" && .test_bin/flashc1 examples/annot_demo.flash --check >/dev/null 2>&1); then
        fail "$name (--check)"
        return 1
    fi

    pass
    echo
}
run_annot_emit_test

run_stage1_pipeline() {
    local name="stage-1 pipeline (flashc1 vs stage-0: shims + manifests, all examples)"
    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    local build_output
    if ! build_output="$(cd "$ROOT" && "$FLASHC" selfhost/flashc1.flash -o .test_bin/flashc1 2>&1)"; then
        fail "$name (build flashc1)"
        echo "$build_output"
        return 1
    fi

    local ex
    for ex in full_demo numeric_spectrum ternary_logic raw_chamber optical_mat; do
        # stage-0 reference artifacts
        if ! (cd "$ROOT" && "$FLASHC" "examples/$ex.flash" --emit-zag --emit-manifest >/dev/null 2>&1); then
            fail "$name (stage-0 emit $ex)"
            return 1
        fi
        cp "$ROOT/examples/${ex}_shim.zag" "$ROOT/.test_bin/${ex}_s0_shim.zag"
        cp "$ROOT/examples/flash.manifest" "$ROOT/.test_bin/${ex}_s0.manifest"
        # stage-1 artifacts (same paths, then compare against the copies)
        if ! (cd "$ROOT" && .test_bin/flashc1 "examples/$ex.flash" --emit-zag --emit-manifest >/dev/null 2>&1); then
            fail "$name (stage-1 emit $ex)"
            return 1
        fi
        if ! cmp -s "$ROOT/examples/${ex}_shim.zag" "$ROOT/.test_bin/${ex}_s0_shim.zag"; then
            fail "$name ($ex shim parity)"
            diff "$ROOT/.test_bin/${ex}_s0_shim.zag" "$ROOT/examples/${ex}_shim.zag" | head -10
            return 1
        fi
        if ! cmp -s "$ROOT/examples/flash.manifest" "$ROOT/.test_bin/${ex}_s0.manifest"; then
            fail "$name ($ex manifest parity)"
            diff "$ROOT/.test_bin/${ex}_s0.manifest" "$ROOT/examples/flash.manifest" | head -10
            return 1
        fi
    done

    # stage-1 drives the full toolchain itself: compile + run raw_chamber
    rm -f $RT_STATE_CACHE_GLOB /tmp/flash_rt_state.ptr
    local run_output
    if ! run_output="$(cd "$ROOT" && .test_bin/flashc1 examples/raw_chamber.flash -o .test_bin/rc_stage1.out --run 2>&1)"; then
        fail "$name (stage-1 end-to-end drive)"
        echo "$run_output"
        return 1
    fi
    if ! assert_output_contains "$name" "voxel pol=-1" "$run_output"; then
        fail "$name (assertion)"
        return 1
    fi
    if ! assert_output_contains "$name" "raw_chamber: ok" "$run_output"; then
        fail "$name (assertion)"
        return 1
    fi

    pass
    echo
}
run_stage1_pipeline

# ── stage-1 language extensions (group/defer/numeric underscores) ─────────────
run_flashc1_example() {
    local name="$1"
    local src="$2"
    shift 2

    local stem
    stem="$(basename "$src")"
    stem="${stem%.flash.zag}"
    stem="${stem%.flash}"
    local out="$BIN_DIR/${stem}.flash1.out"

    echo "== test [$((TESTS_RUN + 1))]: flashc1 $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    rm -f $RT_STATE_CACHE_GLOB /tmp/flash_rt_state.ptr
    local run_output
    if ! run_output="$(cd "$ROOT" && .test_bin/flashc1 "$src" -o "$out" --run 2>&1)"; then
        fail "flashc1 $name"
        echo "$run_output"
        return 1
    fi

    if (("$#" > 0)); then
        local pat
        for pat in "$@"; do
            if ! assert_output_contains "flashc1 $name" "$pat" "$run_output"; then
                fail "flashc1 $name (assertion)"
                return 1
            fi
        done
    fi

    pass
    echo
}

run_flashc1_example "grouping_demo (group blocks + numeric underscores)" \
    "$ROOT/examples/grouping_demo.flash" \
    "sum=13" \
    "million=1000000" \
    "mask=255" \
    "cleanup" \
    "grouping_demo: ok"

# ── bootstrap fixpoint: flashc1 compiles itself, result recompiles itself ───
run_fixpoint() {
    local name="bootstrap fixpoint (flashc1 self-compile → flashc2 → same bytes)"
    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    # stage-1 compiles its own sources (stage-0's shim is the reference)
    local out1
    if ! out1="$(cd "$ROOT" && .test_bin/flashc1 selfhost/flashc1.flash .test_bin/self_shim1.zag 2>&1)"; then
        fail "$name (stage-1 self-compile)"
        echo "$out1"
        return 1
    fi
    if ! cmp -s "$ROOT/selfhost/flashc1_shim.zag" "$ROOT/.test_bin/self_shim1.zag"; then
        fail "$name (stage-1 self-shim differs from stage-0)"
        diff "$ROOT/selfhost/flashc1_shim.zag" "$ROOT/.test_bin/self_shim1.zag" | head -20
        return 1
    fi

    # stage-2: build from stage-1's own output, recompile the same source
    if ! "$ZNC" "$ROOT/.test_bin/self_shim1.zag" -o "$ROOT/.test_bin/flashc2" >/dev/null 2>&1; then
        fail "$name (znc build of flashc2)"
        return 1
    fi
    local out2
    if ! out2="$(cd "$ROOT" && .test_bin/flashc2 selfhost/flashc1.flash .test_bin/self_shim2.zag 2>&1)"; then
        fail "$name (stage-2 self-compile)"
        echo "$out2"
        return 1
    fi
    if ! cmp -s "$ROOT/.test_bin/self_shim1.zag" "$ROOT/.test_bin/self_shim2.zag"; then
        fail "$name (fixpoint: stage-2 output differs from stage-1)"
        return 1
    fi

    # stage-2 must also still match stage-0 on surface programs
    local out3
    if ! out3="$(cd "$ROOT" && .test_bin/flashc2 examples/ternary_logic.flash .test_bin/tl2_shim.zag 2>&1)"; then
        fail "$name (stage-2 surface compile)"
        echo "$out3"
        return 1
    fi
    if ! cmp -s "$ROOT/.test_bin/tl2_shim.zag" "$ROOT/examples/ternary_logic_shim.zag"; then
        fail "$name (stage-2 surface parity with stage-0)"
        return 1
    fi

    # stage-2 must handle the widened full-language subset (enums + switch):
    # compile the enum/switch-using lexer self-test and run it
    local out4
    if ! out4="$(cd "$ROOT" && .test_bin/flashc2 selfhost/lexer.flash .test_bin/lexer_s2_shim.zag 2>&1)"; then
        fail "$name (stage-2 enum/switch compile)"
        echo "$out4"
        return 1
    fi
    if ! "$ZNC" "$ROOT/.test_bin/lexer_s2_shim.zag" -o "$ROOT/.test_bin/lexer_s2.out" >/dev/null 2>&1; then
        fail "$name (znc build of stage-2 lexer shim)"
        return 1
    fi
    local out5
    if ! out5="$("$ROOT/.test_bin/lexer_s2.out" 2>&1)"; then
        fail "$name (stage-2 lexer run)"
        echo "$out5"
        return 1
    fi
    if ! assert_output_contains "$name" "selfhost_lexer: ok" "$out5"; then
        fail "$name (stage-2 lexer assertion)"
        return 1
    fi

    pass
    echo
}
run_fixpoint

# ── Wave 7: standalone gate (bundled flashz, no sibling zag) ────────────────
run_standalone_gate() {
    local name="standalone gate (bundled flashz, no sibling zag)"
    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    if ! "$ROOT/tests/standalone_gate.sh"; then
        fail "$name"
        return 1
    fi

    pass
    echo
}
run_standalone_gate

# ── Wave 8: hardware PCM + FIMG gate ─────────────────────────────────────────
run_hardware_pcm_gate() {
    local name="hardware PCM manifest + FIMG v1 gate"
    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    if ! "$ROOT/tests/hardware_pcm_gate.sh"; then
        fail "$name"
        return 1
    fi

    pass
    echo
}
run_hardware_pcm_gate

# ── Phase 4: target-aware t-unit lowering (ppu32/engine → tN) ────────────────
run_flasht_target_gate() {
    local name="flasht target gate (ppu32 lowers t9, not i64)"
    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    if ! "$ROOT/tests/flasht_target_gate.sh"; then
        fail "$name"
        return 1
    fi

    pass
    echo
}
run_flasht_target_gate

# ── Phase 6: flasht bootstrap gate (hardware targets never invoke znc) ───────
run_flasht_bootstrap_gate() {
    local name="flasht bootstrap gate (--target fimg never calls znc)"
    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    if ! "$ROOT/tests/flasht_bootstrap_gate.sh"; then
        fail "$name"
        return 1
    fi

    pass
    echo
}
run_flasht_bootstrap_gate

# ── flasht Phase 2-3: PPU32 emit + qemu interpreter gate ─────────────────────
run_flasht_ppu32_gate() {
    local name="flasht PPU32 emit + qemu interpreter gate"
    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    if ! "$ROOT/tests/flasht_ppu32_gate.sh"; then
        fail "$name"
        return 1
    fi

    pass
    echo
}
run_flasht_ppu32_gate

# ── Phase 1: Flash IR lowering gate (.fir emission) ────────────────────────────
run_flasht_ir_gate() {
    local name="flasht IR gate (ppu32 emits .fir with [ir] section)"
    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))

    if ! "$ROOT/tests/flasht_ir_gate.sh"; then
        fail "$name"
        return 1
    fi

    pass
    echo
}
run_flasht_ir_gate

# ── Large photonic FIR and pure-source gates ─────────────────────────────────
run_photonic_massive_gate() {
    local name="64-op balanced-ternary photonic FIR"
    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))
    if ! "$FLASHC" "$ROOT/examples/photonic_massive.flash" --target ppu32 >/dev/null 2>&1; then
        fail "$name (compile)"
        return 1
    fi
    local count
    count="$(grep -c 'kind = "route".*lattice_op = [2357]' "$ROOT/examples/photonic_massive.fir" || true)"
    if [[ "$count" != "64" ]]; then
        fail "$name (expected 64 operations, got $count)"
        return 1
    fi
    pass
    echo
}
run_photonic_massive_gate

run_pure_flash_tree_gate() {
    local name="pure Flash/Zag supported source tree"
    echo "== test [$((TESTS_RUN + 1))]: $name =="
    TESTS_RUN=$((TESTS_RUN + 1))
    if ! "$ROOT/tests/pure_flash_tree.sh"; then
        fail "$name"
        return 1
    fi
    pass
    echo
}
run_pure_flash_tree_gate

echo "============================================"
echo "RESULT: $TESTS_PASSED / $TESTS_RUN passed"
if [[ "$FAILURES" -gt 0 ]]; then
    echo "FAILURES: $FAILURES"
    exit 1
fi
echo "== flash: all tests passed =="
exit 0
