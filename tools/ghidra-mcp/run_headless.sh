#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
GHIDRA_HOME="$ROOT/tools/ghidra/ghidra_11.3.2_PUBLIC"

exec "$GHIDRA_HOME/support/analyzeHeadless" \
    "$ROOT/tools/ghidra-project" RC1 \
    -process boot_elf.elf \
    -noanalysis \
    -scriptPath "$ROOT/tools/ghidra-mcp" \
    -postScript StartGhidraMCP.java
