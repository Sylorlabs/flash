#!/usr/bin/env bash
# tests/fuzz_lex_smoke.sh — feed random/garbage bytes through flashc --check (lexer smoke).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FLASHZ="${FLASHZ:-${ZNC:-$ROOT/flash/toolchain/flashz}}"
FLASHC="${FLASHC:-$ROOT/flashc}"
FUZZ_DIR="${TMPDIR:-/tmp}/flash_fuzz_lex_$$"
ROUNDS="${FUZZ_ROUNDS:-32}"
BYTES_MIN="${FUZZ_BYTES_MIN:-16}"
BYTES_MAX="${FUZZ_BYTES_MAX:-512}"

mkdir -p "$FUZZ_DIR"
cleanup() { rm -rf "$FUZZ_DIR"; }
trap cleanup EXIT

if [[ ! -x "$FLASHC" ]]; then
    echo "FAIL: fuzz_lex_smoke — flashc not found at $FLASHC"
    exit 1
fi

pass_round() {
    local path="$1"
    local rc=0
    (cd "$ROOT" && "$FLASHC" "$path" --check >/dev/null 2>&1) || rc=$?
    if [[ "$rc" -gt 1 ]]; then
        echo "FAIL: fuzz_lex_smoke — flashc crashed (signal/abort) on $path (rc=$rc)"
        exit 1
    fi
}

echo "== fuzz_lex_smoke: $ROUNDS rounds via $FLASHC =="

round=0
while ((round < ROUNDS)); do
    nbytes=$((BYTES_MIN + RANDOM % (BYTES_MAX - BYTES_MIN + 1)))
    blob="$FUZZ_DIR/garbage_${round}.flash"
    python3 - "$blob" "$nbytes" <<'PY'
import os, sys
path, nbytes = sys.argv[1], int(sys.argv[2])
open(path, "wb").write(os.urandom(nbytes))
PY
    pass_round "$blob"
    round=$((round + 1))
done

# Valid program with trailing garbage (lex must not crash on suffix bytes).
suffix="$FUZZ_DIR/trailing_garbage.flash"
printf 'fn main() i32 {\n    return 0;\n}\n' > "$suffix"
python3 -c "import os; open('$suffix','ab').write(os.urandom(128))" 2>/dev/null || true
pass_round "$suffix"

# Critic fixture + trailing garbage.
if [[ -f "$ROOT/tests/critic/harsh_lex_bytes.flash" ]]; then
    mixed="$FUZZ_DIR/harsh_lex_bytes_mixed.flash"
    cp "$ROOT/tests/critic/harsh_lex_bytes.flash" "$mixed"
    python3 -c "import os; open('$mixed','ab').write(os.urandom(64))" 2>/dev/null || true
    pass_round "$mixed"
fi

echo "PASS fuzz_lex_smoke ($((ROUNDS + 2)) inputs, no crash)"
exit 0