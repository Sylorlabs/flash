#!/usr/bin/env bash
# run_critic.sh — delegate to canonical tests/run_all.sh critic section.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
export FLASH_CRITIC_ONLY=1
exec "$ROOT/tests/run_all.sh"