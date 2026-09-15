#!/usr/bin/env python3
"""Test a symbolic assembler-expansion hypothesis, NOT production matching.

Run from the repository root with the venv and completed split:
  python tools/research_symbolic_addresses.py --out /tmp/opencode/symbolic-addresses

Compiles ordinary symbolic C++ with the historical GNU-as flags. Records
the native diff using decomp_probe.py, then separately models absolute expansion
of small-symbol memory macros outside .set noreorder. The model never consults
reference instructions to decide what to emit. It is deliberately incomplete;
two VU generalization probes fail. No game source, oracle, or build is modified.

This is evidence about a possible original assembler boundary, not proof of the
original toolchain and not permission to mark these functions newly matched.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parent.parent
PROBES = ROOT / "decomp_state/probes/symbolic_addresses"
LINKER = ROOT / "tools/mips64r5900el-ps2-elf/usr/bin/mips64r5900el-ps2-elf-ld"
CASES = [
    ("draw", "mobyfunc/DrawMobys.s", "DrawMobys", [
        "D_0015FF18=0x0015FF18", "D_0015FF14=0x0015FF14",
        "D_00160F08=0x00160F08"]),
    ("process", "mobyfunc/ProcessMobyAnimData__Fv.s", "ProcessMobyAnimData__Fv", []),
    ("menu", "menu_post_pages/func_00208E68.s", "menu_post_openInventory__Fv", []),
    ("vuref", "vuchain/VU1_addDataRef__FPvi.s", "VU1_addDataRef__FPvi", []),
    ("vunormal", "vuchain/VU1_gsRegsNormal__Fv.s", "VU1_gsRegsNormal__Fv", []),
]


def run(command, log, env=None, allow_diff=False):
    result = subprocess.run(list(map(str, command)), cwd=ROOT, env=env,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True)
    with log.open("a") as stream:
        stream.write(json.dumps(list(map(str, command))) + "\n" + result.stdout)
    if result.returncode and not (allow_diff and result.returncode == 1):
        print(result.stdout)
        result.check_returncode()


def function_bytes(path, name):
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        symbol = elf.get_section_by_name(".symtab").get_symbol_by_name(name)[0]
        section = elf.get_section(symbol["st_shndx"])
        start = symbol["st_value"] - section["sh_addr"]
        return section.data()[start:start + symbol["st_size"]]


def expansion_model(source):
    """Experimental rule only: leave noreorder and already split accesses alone."""
    small = {name for name, size in re.findall(
        r"\.extern\s+(\w+),\s*(\d+)\s*$", source, re.M) if 0 < int(size) <= 8}
    output = []
    noreorder = False
    count = 0
    for line in source.splitlines():
        directive = re.fullmatch(r"\s*\.set\s+(no)?reorder\s*", line)
        if directive:
            noreorder = bool(directive[1])
        match = re.fullmatch(
            r"\s*(lw|lwu|lh|lhu|lb|lbu|ld|sw|sh|sb|sd)\s+(\$\d+),\s*(\w+)\s*",
            line)
        if match and not noreorder and match[3] in small:
            op, reg, symbol = match.groups()
            base = "$1" if op.startswith("s") else reg
            output.extend([f"\tlui\t{base},%hi({symbol})",
                           f"\t{op}\t{reg},%lo({symbol})({base})"])
            count += 1
        else:
            output.append(line)
    return "\n".join(output) + "\n", count


def probe(case, out):
    name, reference, symbol, definitions = case
    folder = out / name
    folder.mkdir(parents=True, exist_ok=True)
    log = folder / "research.log"
    log.write_text("")
    command = [sys.executable, ROOT / "tools/decomp_probe.py", PROBES / (name + ".cpp"),
               ROOT / "code/_generated/matchings/game" / reference, symbol,
                "--out", folder, "--assembler", "gnu"]
    for definition in definitions:
        command.extend(["--define", definition])
    run(command, log, allow_diff=True)
    native = json.loads((folder / "candidate.json").read_text())
    if "candidate_size" not in native:
        raise RuntimeError(f"{name}: native compile/link failed; inspect {log}")
    assembly, count = expansion_model((folder / "candidate.s").read_text())
    (folder / "model.s").write_text(assembly)
    env = dict(os.environ, WINEPREFIX=str(ROOT / "tools/wineprefix"), WINEDEBUG="-all")
    run([ROOT / "tools/wine/bin/wine", ROOT / "tools/cc/ee/bin/as.exe",
         "-EL", "-m5900", "-G8", "-o", folder / "model.o", folder / "model.s"], log, env)
    commands = [json.loads(line) for line in (folder / "candidate.log").read_text().splitlines()
                if line.startswith('["')]
    link = commands[-1]
    if not link[0].endswith("-ld"):
        raise RuntimeError("Native probe did not reach linking")
    link = [s.replace("candidate.o", "model.o").replace("candidate.elf", "model.elf")
            for s in link]
    run(link, log)
    actual = function_bytes(folder / "model.elf", symbol)
    reference_text = (folder / "reference.s").read_text().split("endlabel", 1)[0]
    # decomp_probe already validated these reference bytes against the asset.
    expected = bytes.fromhex("".join(re.findall(
        r"/\*\s*[0-9a-fA-F]+\s+[0-9a-fA-F]+\s+([0-9a-fA-F]{8})\s*\*/", reference_text)))
    result = dict(symbol=symbol, native_match=native["match"],
                  native_size=native["candidate_size"], original_size=len(expected),
                  model_size=len(actual), model_match=actual == expected,
                  expanded_macros=count, model_object_sha256=hashlib.sha256(
                      (folder / "model.o").read_bytes()).hexdigest(),
                  model_differing_word_offsets=[i for i in range(0, max(len(actual), len(expected)), 4)
                                               if actual[i:i+4] != expected[i:i+4]])
    (folder / "model.json").write_text(json.dumps(result, indent=2) + "\n")
    print(f"{symbol}: native {result['native_size']}/{len(expected)} B "
          f"match={result['native_match']}; model {len(actual)}/{len(expected)} B "
          f"match={result['model_match']}")
    return result


def draw_mask(word):
    # Only for locating corresponding overlay copies, never for final comparison.
    opcode = word >> 26
    if opcode in (2, 3):
        return word & 0xfc000000
    if opcode in (15, 35, 43, 4, 9):
        return word & 0xffff0000
    return word


def overlay_test(out):
    original = (ROOT / "assets/boot_elf.elf").read_bytes()
    pattern = tuple(map(draw_mask, struct.unpack_from("<32I", original, 0x10e3e0)))
    model = out / "draw/model.o"
    results = []
    for path in sorted((ROOT / "assets/levels").glob("*/overlay.elf")):
        with path.open("rb") as stream:
            elf = ELFFile(stream)
            text = elf.get_section_by_name(".text")
            data, base = text.data(), text["sh_addr"]
        words = struct.unpack_from(f"<{len(data)//4}I", data)
        hits = [i for i in range(len(words)-31)
                if tuple(map(draw_mask, words[i:i+32])) == pattern]
        if len(hits) != 1:
            raise RuntimeError(f"{path}: expected one DrawMobys candidate, got {hits}")
        index = hits[0]
        w = words[index:index+32]
        address = base + index*4

        def absolute(hi, lo):
            low = w[lo] & 65535
            return ((w[hi] & 65535) << 16) + (low if low < 32768 else low-65536)

        # Corresponding operands establish each experiment-only link address.
        # The old D_ names identify BOOT slots; their OVERLAY values differ.
        definitions = {name: absolute(hi, lo) for name, hi, lo in [
            ("D_0018A2D8", 4, 5), ("D_0015FF18", 10, 11), ("D_0015FF14", 13, 14),
            ("vu1ChainHead", 17, 18), ("D_00160F08", 19, 20),
            ("vuChainOverflowMessage", 24, 26)]}
        definitions.update({name: (w[n] & 0x3ffffff) << 2 for name, n in [
            ("DrawMobysSetup__Fv", 2), ("InitMobyClassDists__Fv", 8),
            ("MobyProc", 15), ("STUB_printf", 25), ("DrawMobysCleanUp", 27)]})
        folder = out / "overlays" / path.parent.name
        folder.mkdir(parents=True, exist_ok=True)
        script = folder / "link.ld"
        script.write_text(f"SECTIONS {{ .text {hex(address)} : {{ *(.text) }} }}\n"
                          "_gp = 0x166c00;\n")
        command = [LINKER, "-EL", "-m", "elf32lr5900", "-T", script, "-e", "DrawMobys"]
        for name, value in definitions.items():
            command += ["--defsym", f"{name}={hex(value)}"]
        command += [model, "-o", folder / "model.elf"]
        run(command, folder / "link.log")
        actual = function_bytes(folder / "model.elf", "DrawMobys")
        expected = data[index*4:index*4+128]
        result = dict(level=path.parent.name, address=hex(address),
                      match=actual == expected, definitions=definitions,
                      overlay_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
        results.append(result)
        print(f"  {path.parent.name}: {hex(address)} same model.o, match={result['match']}")
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()
    out = args.out.resolve()
    if out.is_relative_to(ROOT / "code") or out.is_relative_to(ROOT / "build"):
        parser.error("Use an experiment directory outside code/ and build/")
    out.mkdir(parents=True, exist_ok=True)
    # Invalidate a previous summary before starting potentially failing commands.
    summary = out / "summary.json"
    summary.write_text('{"status": "incomplete"}\n')
    cases = [probe(case, out) for case in CASES]
    overlays = overlay_test(out)
    summary.write_text(json.dumps(dict(status="research-complete", cases=cases,
                                       overlays=overlays), indent=2) + "\n")
    print("Research only: model matches are NOT production matches or full-overlay parity.")


if __name__ == "__main__":
    main()
