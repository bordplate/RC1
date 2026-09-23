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
opencode_pid=""
opencode_log=""
keep_opencode_log=0
opencode_url="http://127.0.0.1:4096"

opencode_is_ready() {
    if [[ -n "${OPENCODE_SERVER_PASSWORD:-}" ]]; then
        curl --user "${OPENCODE_SERVER_USERNAME:-opencode}:$OPENCODE_SERVER_PASSWORD" \
            --connect-timeout 1 --max-time 2 -fsS "$opencode_url/global/health" >/dev/null 2>&1
    else
        curl --connect-timeout 1 --max-time 2 -fsS "$opencode_url/global/health" >/dev/null 2>&1
    fi
}

./brrr.py "Starting decomp loop" --title "RC1 decomp"

cleanup() {
    if [[ -n "$headless_pid" ]]; then
        kill -- "-$headless_pid" 2>/dev/null || kill "$headless_pid" 2>/dev/null || true
        wait "$headless_pid" 2>/dev/null || true
    fi
    if [[ -n "$opencode_pid" ]]; then
        kill -- "-$opencode_pid" 2>/dev/null || kill "$opencode_pid" 2>/dev/null || true
        wait "$opencode_pid" 2>/dev/null || true
    fi
    if [[ -n "$headless_log" ]] && [[ "$keep_headless_log" -eq 0 ]]; then
        rm -f "$headless_log"
    fi
    if [[ -n "$opencode_log" ]] && [[ "$keep_opencode_log" -eq 0 ]]; then
        rm -f "$opencode_log"
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

if ! opencode_is_ready; then
    opencode_log=$(mktemp)
    setsid opencode serve --hostname 127.0.0.1 --port 4096 >"$opencode_log" 2>&1 &
    opencode_pid=$!
fi

ready=1
for attempt in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do
    if opencode_is_ready; then
        ready=0
        break
    fi
    sleep 1
done

if [[ "$ready" -ne 0 ]]; then
    printf 'OpenCode did not become ready on %s.\n' "$opencode_url" >&2
    if [[ -n "$opencode_log" ]]; then
        printf 'OpenCode log: %s\n' "$opencode_log" >&2
        keep_opencode_log=1
    fi
    exit 1
fi

if [[ "${1:-}" == "--check-only" ]]; then
    printf 'Headless Ghidra and OpenCode at %s are ready.\n' "$opencode_url"
    exit 0
fi

~/.local/bin/vectorcode update

prompt='Continue the autonomous RC1 matching decompilation. Review AGENTS.md and existing decomp_state notes. Select exactly one target in the AGENTS.md priority order: the first open entry in decomp_state/refactor.json (apply its documented parity-preserving refactor and clear the entry when verified) before falling back to the next nonmatching INCLUDE_ASM in decomp_state/queue.json. If you encounter code that violates STYLEGUIDE.md or the code-formatting rules (hardcoded data addresses, magic numbers, stale workarounds), record it in decomp_state/refactor.json. Use Ghidra MCP and local build/diff tools, make progress, verify the target mechanically, run the full build/parity check, commit only that verified target and its intended state note, and update notes. Do not commit a function unless its object/assembly output matches and cmp build/boot_elf.elf assets/boot_elf.elf passes.'
iteration=0

while true; do
    if python3 tools/decomp_status.py --complete; then
        printf '%s\n' '100% matching decompilation reached.'
        exit 0
    fi

    ((iteration += 1))
    # Omitting --continue and --session makes opencode create a fresh server-backed session.
    opencode run --attach "$opencode_url" --dir "$PWD" --title "RC1 autonomous iteration $iteration" --thinking --auto "$prompt"
    ~/.local/bin/vectorcode update
    sleep 10
done

./brrr.py "RC1 decomp has stopped!" --title "RC1 decomp"
