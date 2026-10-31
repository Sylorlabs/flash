#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FLASHC="${FLASHC:-$ROOT/flashc}"
PROJECT="$ROOT/tests/fixtures/named_collision_project"

cd "$PROJECT"
if FLASH_HOME="$ROOT" "$FLASHC" app.flash --check >/tmp/named_collision_gate.out 2>&1; then
    echo "named_collision_gate: unexpectedly passed"
    cat /tmp/named_collision_gate.out
    exit 1
fi
if ! grep -q 'flashc1: module collision: duplicate function `collision`' /tmp/named_collision_gate.out; then
    echo "named_collision_gate: missing duplicate-top-level diagnostic"
    cat /tmp/named_collision_gate.out
    exit 1
fi

rm -f /tmp/named_collision_gate.out
echo "named_collision_gate: ok"
