# Simple Decompilation Automation

`tools/auto_decomp_simple.py` handles a deliberately narrow set of trivial
functions. It never treats source-level similarity as a match.

## Safety Model

The default invocation only scans generated assembly:

```sh
python3 tools/auto_decomp_simple.py
```

`--verify` writes candidates under a temporary directory and passes each one
through `tools/decomp_probe.py`. That probe compiles with the owning
translation unit's scheduler flags, resolves relocations at the original
address and runtime GP, and compares every function byte to the original boot
image. Generated candidates may pass verified experiment-only symbol addresses
with `decomp_probe.py --define`; this does not modify the project symbol map.

`--apply` first requires a matching baseline. It then verifies the standalone
candidate, replaces exactly one `INCLUDE_ASM`, rebuilds, and runs the full
`cmp build/boot_elf.elf assets/boot_elf.elf` parity oracle. A failed integrated
candidate is replaced with its exact original placeholder and the matching
baseline is rebuilt. Successful applications are recorded in
`decomp_state/matched.json` and send the normal `brrr.py` status notification.

The tool refuses to apply changes to a dirty source file by default. It also
refuses unknown unmangled signatures and forwarding wrappers whose return type
cannot be inferred from the old C++ symbol. These checks can only be relaxed
explicitly after reviewing callers and types.

## Recognized Shapes

- Empty leaves.
- Integer constant returns and identity returns.
- Direct byte, halfword, word, and doubleword field getters.
- Direct zero/nonzero field predicates.
- Direct field setters and old-value exchanges.
- Seven-instruction forwarding wrappers with unchanged arguments or a constant
  adjustment to the first pointer argument.
- Movie-style wrappers that call a function on a fixed offset from a global
  object pointer.
- Simple printf callbacks that log one callback-data word and return a constant.
- Integer multiply/divide scaling leaves.

Global accesses, multi-store functions, indirect callbacks, wrappers with
constant argument setup, branches, and dead-tail fragments are intentionally
unsupported. Those shapes need semantic/global-address research or source
permutations that cannot be inferred safely from a few instructions.

## Examples

Scan only getters and setters:

```sh
python3 tools/auto_decomp_simple.py --kind field-getter --kind field-setter
```

Compile and compare one candidate without editing game source:

```sh
source .venv/bin/activate
python3 tools/auto_decomp_simple.py videoDecEndPut__FP8VideoDec --verify
```

Apply a known-signature leaf after all checks:

```sh
source .venv/bin/activate
python3 tools/auto_decomp_simple.py function__FP8TypeName --apply
```

Forwarding wrapper symbols do not encode their return type. Inspect callers
and the callee prototype, then select it explicitly:

```sh
python3 tools/auto_decomp_simple.py videoDecEndPut__FP8VideoDec \
  --verify --wrapper-return void
```

Use `--allow-unknown-signature` only after checking all callers of an unmangled
function; register usage cannot reveal unused parameters. Use `--allow-dirty`
only when the existing source edits have been reviewed and should be retained.
