#!/usr/bin/env python3
"""Check autonomous decompilation state and the mechanical parity oracle."""

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
STATE = ROOT / "decomp_state"
INCLUDE_RE = re.compile(
    r'INCLUDE_ASM\("code/_generated/nonmatchings/([^"\n]+)",\s*([^\)\s]+)\)'
)


def targets():
    result = []
    for source in sorted((ROOT / "code").rglob("*")):
        if source.suffix not in {".c", ".cpp"}:
            continue
        for line_number, line in enumerate(source.read_text(encoding="utf-8").splitlines(), 1):
            match = INCLUDE_RE.search(line)
            if match:
                result.append(
                    {
                        "id": f"{source.relative_to(ROOT)}:{line_number}:{match.group(2)}",
                        "source": str(source.relative_to(ROOT)),
                        "line": line_number,
                        "asm": f"code/_generated/nonmatchings/{match.group(1)}/{match.group(2)}.s",
                        "name": match.group(2),
                        "status": "todo",
                    }
                )
    return result


def read_json(name, default):
    path = STATE / name
    if not path.exists():
        return default
    return json.loads(path.read_text(encoding="utf-8"))


def blocked_entries():
    blocked = read_json("blocked.json", {})
    if isinstance(blocked, dict):
        return blocked
    return {entry["id"]: entry for entry in blocked}


def target_key(target_id):
    # Historical state IDs include mutable source line numbers.
    parts = target_id.rsplit(":", 2)
    if len(parts) == 3 and parts[1].isdigit():
        return f"{parts[0]}:{parts[2]}"
    return target_id


def run_parity():
    build = subprocess.run(["make"], cwd=ROOT)
    if build.returncode:
        return False
    return subprocess.run(
        ["cmp", "-s", "build/boot_elf.elf", "assets/boot_elf.elf"], cwd=ROOT
    ).returncode == 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--count", action="store_true")
    parser.add_argument("--init", action="store_true")
    parser.add_argument("--complete", action="store_true")
    args = parser.parse_args()

    found = targets()
    if args.init:
        STATE.mkdir(exist_ok=True)
        (STATE / "queue.json").write_text(json.dumps(found, indent=2) + "\n", encoding="utf-8")
        print(f"Initialized {len(found)} decompilation targets.")
        return 0

    if args.count:
        print(len(found))
        return 0

    blocked = {target_key(key): value for key, value in blocked_entries().items()}
    active = [entry for entry in found if target_key(entry["id"]) not in blocked]
    print(f"targets={len(found)} active={len(active)} blocked={len(found) - len(active)}")

    if not args.complete:
        return 0
    missing_notes = [
        key
        for key, value in blocked.items()
        if not (
            isinstance(value, str) and value.strip()
            or isinstance(value, dict)
            and any(isinstance(value.get(field), str) and value[field].strip()
                    for field in ("note", "reason"))
        )
    ]
    if missing_notes:
        print("Incomplete: blocked targets need blocker notes:", file=sys.stderr)
        print("\n".join(missing_notes), file=sys.stderr)
        return 1
    if found:
        print("Incomplete: nonmatching INCLUDE_ASM targets remain (blocked is not matched).", file=sys.stderr)
        return 1
    if not run_parity():
        print("Incomplete: build/parity check failed.", file=sys.stderr)
        return 1
    print("Complete: no nonmatching targets remain and parity passes.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
