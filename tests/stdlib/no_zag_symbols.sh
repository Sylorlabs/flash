#!/usr/bin/env bash
# tests/stdlib/no_zag_symbols.sh — W2: verify no _zag_* symbols in user code.
#
# User .flash files (in examples/) must not call _zag_print, _zag_println,
# or _zag_i64_to_str directly. They should use the Flash-native stdlib
# (flash_print, flash_println, flash_println_int, etc.) instead.
#
# Exceptions:
#   - selfhost/std/flash_std.flash (the stdlib wrapper itself)
#   - selfhost/std/*.flash (other stdlib modules that wrap _zag_*)
#   - flash/stdlib/*.zag (runtime implementation)
#   - files with extern fn declarations (the declaration is fine, calls are not)

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

# Check examples/*.flash for _zag_print/_zag_println/_zag_i64_to_str CALLS
# (not extern declarations)
VIOLATIONS=""
for f in "$ROOT"/examples/*.flash; do
    # Skip comments and extern fn declarations
    MATCHES=$(grep -n '_zag_print\|_zag_println\|_zag_i64_to_str' "$f" 2>/dev/null | grep -v '^\s*//' | grep -v 'extern fn' || true)
    if [[ -n "$MATCHES" ]]; then
        VIOLATIONS="${VIOLATIONS}\n$f:\n$MATCHES\n"
    fi
done

if [[ -n "$VIOLATIONS" ]]; then
    echo "FAIL: _zag_* print symbols found in user code:"
    echo -e "$VIOLATIONS"
    exit 1
fi

echo "no_zag_symbols: ok"
