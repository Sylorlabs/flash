#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
bad="$(find "$ROOT/flash" "$ROOT/selfhost" "$ROOT/flasht" "$ROOT/tests" \
    -type f \( -name '*.c' -o -name '*.h' -o -name '*.py' -o -name '*.zig' \) \
    -print -quit)"
if [[ -n "$bad" ]]; then
    echo "FAIL: non-Flash/Zag source in supported tree: $bad" >&2
    exit 1
fi
echo "PASS: supported Flash tree contains only Flash/Zag implementation sources"
