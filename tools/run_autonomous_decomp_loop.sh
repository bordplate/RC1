#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

if [[ ! -f .venv/bin/activate ]]; then
    printf '%s\n' 'Missing .venv; create and activate the project Python environment first.' >&2
    exit 1
fi
source .venv/bin/activate

headless_pid=""
headless_log=""
keep_headless_log=0

cleanup() {
    if [[ -n "$headless_pid" ]]; then
        kill -- "-$headless_pid" 2>/dev/null || kill "$headless_pid" 2>/dev/null || true
        wait "$headless_pid" 2>/dev/null || true
    fi
    if [[ -n "$headless_log" ]] && [[ "$keep_headless_log" -eq 0 ]]; then
        rm -f "$headless_log"
    fi
}

trap cleanup EXIT INT TERM

if ! curl --connect-timeout 1 --max-time 2 -fsS http://127.0.0.1:8080/methods >/dev/null 2>&1; then
    headless_log=$(mktemp)
    setsid tools/ghidra-mcp/run_headless.sh >"$headless_log" 2>&1 &
    headless_pid=$!
fi

ready=1
for attempt in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do
    if curl --connect-timeout 1 --max-time 2 -fsS http://127.0.0.1:8080/methods >/dev/null 2>&1; then
        ready=0
        break
    fi
    sleep 2
done

if [[ "$ready" -ne 0 ]]; then
    printf '%s\n' 'Headless Ghidra did not become ready on http://127.0.0.1:8080.' >&2
    if [[ -n "$headless_log" ]]; then
        printf 'Ghidra log: %s\n' "$headless_log" >&2
        keep_headless_log=1
    fi
    exit 1
fi

if [[ "${1:-}" == "--check-only" ]]; then
    printf '%s\n' 'Headless Ghidra is ready.'
    exit 0
fi

prompt='Continue the autonomous RC1 matching decompilation. Review AGENTS.md and existing decomp_state notes, select exactly one useful function, use Ghidra MCP and local build/diff tools, make progress, verify the function mechanically, run the full build/parity check, commit only that verified function and its intended state note, and update notes. Do not commit a function unless its object/assembly output matches and cmp build/boot_elf.elf assets/boot_elf.elf passes. Stop only if the full mechanical completion condition has passed.'

while true; do
    if python3 tools/decomp_status.py --complete; then
        printf '%s\n' '100% matching decompilation reached.'
        exit 0
    fi

    opencode run "$prompt"
    sleep 10
done
