# RC1 Autonomous Matching Decompilation Agent

This repository is a matching decompilation of Ratchet & Clank 1 for the PS2.
The objective is to replace decompilable `INCLUDE_ASM` functions with matching
C/C++ while preserving byte-for-byte parity with the original boot ELF.

The agent may inspect assembly and Ghidra, edit source, build, diff, and record
state autonomously. The agent must not decide that a function or the project
matches by inspection. Compiler output, object/assembly diffs, and the final
binary comparison are the oracles.

## Runtime Layout And Overlays

The boot ELF contains resident runtime code and the first `start` / main-menu
level. Normal levels are loaded as separate overlay executables and replace
that initial level code. Do not assume that every referenced function belongs
in the resident binary.

Before moving or renaming a symbol, distinguish resident shared code from
level-overlay code. The current build and parity oracle cover only the boot
ELF. If overlay decompilation is added later, each overlay needs its own split,
build, diff, and parity oracle.

## Ground Truth

- `assets/boot_elf.elf` is the original NTSC `SCUS_971.99` boot ELF.
- `code/_generated/` is produced by Splat from the original ELF.
- `make` plus `cmp build/boot_elf.elf assets/boot_elf.elf` is the parity oracle.
- Generated assembly is the function-level source of truth.

Splat comment quirk (verified 2026-09-04): in generated `.s` files the third
comment field `/* <fileoff> <vram> <hex> */` prints the raw on-disk bytes as a
big-endian hex string, i.e. the little-endian instruction word reversed. A
standard `jr $ra` (word 0x03E00008, stored LE as `08 00 E0 03`) shows up as
`0800E003`. Read those fields back as LE (or just objdump the object / read
raw ELF bytes) before diffing instruction words.
- Compiler output and object/assembly diffs judge candidate quality.
- `tools/decomp_status.py --complete` is the mechanical completion check.

## Local Toolchain

- EEGCC 2.95.2 runs through the local Wine wrapper configured in
  `userconfig.mk`.
- MIPS binutils are under `tools/mipsel-linux-gnu/` and the PS2 linker tools
  are under `tools/mips64r5900el-ps2-elf/`.
- Wrench 0.5 is under `tools/wrench-release/`.
- Ghidra 11.3.2 is under `tools/ghidra/ghidra_11.3.2_PUBLIC/`.
- Emotion Engine Reloaded is installed under
  `tools/ghidra/ghidra_11.3.2_PUBLIC/Ghidra/Extensions/ghidra-emotionengine-reloaded/`.
- The Ghidra project is `tools/ghidra-project/RC1.gpr` and contains
  `boot_elf.elf`.
- Python dependencies are in `.venv`; activate it before using Splat or the
  build tools.

## Headless Ghidra

There is no GUI or X server on this machine. Do not use `ghidraRun`, Swing, or
the original GUI GhidraMCP `PluginTool` path.

Start the headless Ghidra HTTP backend from the repository root:

```sh
source .venv/bin/activate
tools/ghidra-mcp/run_headless.sh
```

The launcher runs `analyzeHeadless` against the existing `RC1` project, opens
`boot_elf.elf`, loads `tools/ghidra-mcp/StartGhidraMCP.java`, and intentionally
keeps the process alive. The Java script exposes the Ghidra API over HTTP on
`127.0.0.1:8080` for the Python MCP bridge.

The original GhidraMCP plugin requires a GUI `PluginTool` and fails under
`analyzeHeadless`. `StartGhidraMCP.java` is the headless-compatible transport;
it directly uses the current headless `Program` for function listing,
decompilation, disassembly, references, symbols, comments, and supported
renames.

Emotion Engine Reloaded is active when headless startup prints:

```text
Language: r5900:LE:32:default
```

That language ID is supplied by Emotion Engine Reloaded, not stock Ghidra MIPS.
Verify the backend from another terminal:

```sh
curl http://127.0.0.1:8080/methods
```

The local MCP adapter is configured in `.opencode/opencode.jsonc`. It is a
stdio MCP server that forwards calls to the HTTP backend. `opencode mcp list`
only verifies that the adapter process starts; the `curl` check verifies that
headless Ghidra is actually running.

## Required Local Inputs

Before starting the autonomous loop, the local machine must have:

- the original NTSC `SCUS_971.99` / `boot_elf.elf` in `assets/`
- EEGCC 2.95.2 and its Wine32 runtime under `tools/cc/` and `tools/wine/`
- MIPS binutils and the PS2 linker tools
- Wrench under `tools/wrench-release/` if ISO or asset validation is needed
- Python dependencies installed in `.venv`
- the headless Ghidra backend running with this project loaded
- opencode installed

For a fresh checkout, create `userconfig.mk` from the template and adjust local
paths, then activate the Python environment:

```sh
cp userconfig.template.mk userconfig.mk
source .venv/bin/activate
```

Do not commit local tool installations, credentials, generated assembly, build
output, or `userconfig.mk`.

## Build And Split

From the repository root:

```sh
source .venv/bin/activate
make split
make
cmp build/boot_elf.elf assets/boot_elf.elf
```

For a clean verification:

```sh
source .venv/bin/activate
make clean && make split && make -j2
cmp build/boot_elf.elf assets/boot_elf.elf
```

Never enable `ALLOW_NONMATCHING` globally. Do not remove an assembly fallback
until the compiled output has been compared mechanically.

The Makefile has no dependency from a C/C++ object to the generated `.s` files
it `INCLUDE_ASM`s. If you rename a symbol that other included assembly
references, also `touch` those source files before rebuilding, or stale objects
survive and linking fails with an undefined reference to the old name (see
decomp_state/notes/audiodec_func_0023AEE0.md).

The compiler is correct, but compiler flags may not necessarily match what Insomniac used yet. Try to identify compiler flags when you encounter a larger function that otherwise won't match. Update this when you're confident compiler flags are correct.

Observed EGC 2.95.2 scheduling habits (with the project flags): in small
functions that end with independent stores plus a constant return, the stores
are emitted in SOURCE statement order and the last one is pulled into the
`jr $ra` delay slot, while a `addiu $v0,$0,N` for the return value is hoisted
above the stores. Reordering assignments in the C source flips which store
lands in the delay slot, so match the original store sequence by choosing the
statement order accordingly (see decomp_state/notes/strfile_func_0023BA48.md).

Exception observed 2026-09-04: when both trailing stores are CONSTANT stores
sharing one %hi/%lo-computed global base (e.g. zeroing two struct fields),
EGC emitted them in REVERSE source order instead — the first statement's store
landed in the delay slot and the second before `jr $ra` (both orders tested;
see decomp_state/notes/stream_func_00217020.md). For such tails try both
statement orders and let the object diff decide. Also: to keep both stores as
direct `base+offset`, declare struct fields at the exact offsets; raw
pointer arithmetic on an `u8[]` base makes EGC materialize a second
`addiu` for the other offset (see same note).

Extension observed 2026-09-04: with THREE independent constant stores sharing
one %hi/%lo global base, EGC emits them in the fixed permutation
`stmt3; stmt1; jr $ra; <delay slot: stmt2>` regardless of other context (all
six statement permutations compiled standalone give this same mapping). So to
reproduce an original tail `sh X; sh Y; jr $ra; <sh Z>`, write the statements
in source order `Y, Z, X` (see decomp_state/notes/music_Unpause__Fv.md for the
permutation table and the matched function).

Extension observed 2026-09-04 (register base): the same fixed permutation
`stmt3; stmt1; jr $ra; <delay slot: stmt2>` applies to three independent
constant stores sharing a REGISTER base (e.g. `sw zero,off(a0)`), not just a
%hi/%lo global base. Source order `f40, f50, f3c` reproduces an original tail
`sw 0x3c; sw 0x40; jr $ra; <sw 0x50>`; natural order `f3c, f40, f50` instead
emits `0x50, 0x3c, jr, <0x40>` (wrong). The `return 0;` still hoists
`daddu $v0,$0,$0` above the stores (see
decomp_state/notes/pause_func_002223D8.md).

Observation observed 2026-09-04 (single-call wrappers): a zero-arg C function
whose only statement is a call passing literal 0 (`callee(0);`) compiles to a
0x10 frame with `sq/lq $ra` at 0(sp) and the argument load `daddu $4,$0,$0`
(word 0x0000202D — objdump prints it as `move a0,zero`) hoisted into the `jal`
delay slot; the epilogue is the standard `lq; jr; <ds addiu>`. The callee MUST
be declared with a parameter (e.g. `int`) in the extern prototype — a `(void)`
prototype suppresses the argument setup and the delay slot comes out as `nop`,
breaking the match (see
decomp_state/notes/stash_func_00232CE0.md for the matched function).

Observation observed 2026-09-04 (branch-delay-slot scheduling for constant
stores): for `if (cond) GLOBAL = N;` where GLOBAL is a plain `int` declared
with `__attribute__((section(".data")))`, EGC puts the STORE-ADDRESS `lui` in
the branch delay slot (`beqz; <lui>; li; sw; jr; nop`). The original menu
family (0x208E68/90/ED8/F00 and 0x208EB8) instead schedules `li $v0,N` into
the delay slot and the `lui $at` after. Writing the store as a constant
address cast — `*(int*)0xADDR = N;` — flips EGC to the original
`<li>; lui; sw` order and matches byte-for-byte (also changes the base
register to `$at`). Related: a plain `extern int` with no section attribute
makes EGC emit GP-relative accesses for any address inside the gp window
(gp=0x166C00, ±32K), and for out-of-window undefined syms the link fails with
`relocation truncated to fit: R_MIPS_GPREL16` + "small-data section exceeds
64KB" — declare such globals with `__attribute__((section(".data")))`.
Precedent for the cast idiom: `endDisplay()` in code/game/movie/disp.cpp
(see decomp_state/notes/menu_func_00208EB8.md for the matched function).
Load-side extension (2026-09-04): in the same menu family, a plain global
LOAD in the condition (`if (D_0015EEB4 & 0x40) return;`) allocates the
address base to $v1 (`lui v1; lw v0,off(v1)`), while the original reuses
$v0 for base and value (`lui v0,0x16; lw v0,-4428(v0)`). Casting the load
address too — `if (*(int*)0x15EEB4 & 0x40) return; *(int*)0x15EEB0 = 3;` —
 reproduces the original byte-for-byte, so this family needs constant-cast
 accesses on BOTH sides (see decomp_state/notes/menu_func_00208E68.md; the
 byte-identical clone family func_00208E68/90/ED8/F00 is fully matched).

Observation observed 2026-09-05 (EGC dead-code tails — ~46 "phantom functions"):
the original binary contains ~46 unreachable 4-byte fragments that spimdis splits
into their own 4-byte "functions" (named `func_XXXX` in the queue, e.g.
func_001FDD50, func_001FDC90, and the four `sw v0,-0x3FE0(gp)` in vuchain). Each
sits at an 8-aligned address right after a preceding function's `jr ra; <delay>`
epilogue and contains one instruction + nop: a stack deallocate (`addiu sp,sp,N`,
~28x), a store of the return value or an arg field (`sw/sh/lw/daddu/andi`), or
bare nops. They are dead: nothing jumps to them (verified for 0x1FDD50 by
raw-encoding search for jal/j targets and absolute words across core.text, .text
and the overlays, plus the jumptable entries — and Ghidra has no function there).
Mechanism (verified by compiling standalone candidates with the project EGC): with
two stores before a `return`, EGC keeps the FIRST alive (before `jr ra`, often in
the delay slot) and emits the SECOND after the `jr ra` — unreachable. Strategy:
when matching the preceding function, look for the C form that makes EGC
regenerate the tail bytes and drop the orphan INCLUDE_ASM in the same commit; if
no form is found, keep the orphan INCLUDE_ASM (it supplies the 8 bytes and keeps
parity) and block the orphan entry explaining it is a dead tail, not a function
(see decomp_state/notes/help_msg_string__Fi.md — msg_string__Fi matched with its
 orphan func_001FDD50 retained).

Observation observed 2026-09-05 (EGC auto-emits `jal __main` for C++ main): a
C++ `main` gets a compiler-inserted `jal __main; nop` at the top of the body
(verified: writing an explicit `__main();` statement in the source emits a
SECOND jal — 21 words vs the original 19; no explicit call + no extern
declaration reproduces the original). This held in a TU with no global
constructors, so treat it as unconditional for C++ main in this EGC build.
Also: a `jal` to a function INCLUDE_ASM'd earlier in the same file can
relocation-reference the `.text` section symbol (addend 0) instead of the
function symbol when the callee sits at the object's section origin — it links
to the identical address; judge such relocs by the link result, not the symbol
name (see decomp_state/notes/boot_main.md for the matched main).

Observation observed 2026-09-05 (constant-store register depends on return
type): for a function that stores constants and RETURNS an int, EGC reserves
v0 for the return value and materializes the body's constant stores into v1 —
one `li v1,N` for the body plus a separate `li v0,N` for the return (no
hoisting, 8-instruction shape for the 3-store func_0022E188 pattern). For a
VOID function the same stores get `li v0,N` hoisted to the top of the body,
which the stores reuse — the 7-instruction original shape. A standalone EGC
-G8 -O2 matrix of 8 int-returning forms (inline literals, `int x = 1` at top
or middle, chained `a = b = 1`, `return (b = (a = 1))`) all compiled to the
same 2-`li` shape, so when an original has a single hoisted constant load used
by stores AND left in v0 at `jr ra`, the function is almost certainly void and
the v0 value is a leftover, not a return value — check callers for return-use
before assuming `return N` (see decomp_state/notes/space_func_0022E188.md).
Related scheduling for two constant-cast stores + one gp-symbol store: source
order `castA; gpC; castB` reproduces machine order `castA; castB; jr;
<gpC in delay slot>`; the naive `castA; castB; gpC` order puts the gp store
second instead (same note).

## OpenCode Model And MCP Configuration

The repository-local `.opencode/opencode.jsonc` selects the requested local
OpenAI-compatible provider as the default model:

- Provider ID: `qwen-local`
- Base URL: `http://127.0.0.1:8081`
- Model ID: `qwen3.8-27b`
- Context limit: `262144`
- Configured output limit: `32768`

The full model reference is `qwen-local/qwen3.8-27b`. The inference endpoint
advertises `qwen3.8-27b` through `/models`.
OpenCode reads configuration at startup, so restart it after configuration
changes.

For a one-shot autonomous iteration, run from the repository root after
starting headless Ghidra:

```sh
opencode run "Autonomously continue the RC1 matching decompilation. Work on exactly one function. Use headless Ghidra MCP, generated assembly, compiler output, object/asm diffs, and full binary comparison as ground truth. Replace one decompilable nonmatching INCLUDE_ASM function with matching C/C++, verify its object/assembly output and full ELF parity, then commit only that function and its intended state note. Never declare success unless the mechanical completion condition passes."
```

## Durable State

`decomp_state/` is agent memory, not the success oracle:

- `queue.json` contains the enumerated targets.
- `matched.json` may record durable matched-function notes.
- `blocked.json` maps target IDs to concrete blocker notes.
- `attempts/` stores optional per-attempt records.
- `notes/` stores investigation and strategy notes.

The status tool enumerates source files directly, so stale queue entries cannot
make the project appear complete. Every blocked target must have a non-empty
`note` or `reason` in `blocked.json`.

Useful status commands:

```sh
source .venv/bin/activate
python3 tools/decomp_status.py --count
python3 tools/decomp_status.py --complete
```

The completion check fails while any nonmatching `INCLUDE_ASM` remains active,
while a blocker lacks a note, when the build fails, or when the final ELF does
not compare equal to `assets/boot_elf.elf`.

## Function Workflow

### Subagent Delegation

- Use `decomp-researcher` before implementing an unfamiliar target when Ghidra,
  assembly, caller, or data-layout research can be performed independently.
- Use `explore` for quick read-only repository searches that do not require the
  RC1-specific research report.
- Keep source edits and iteration in the primary agent. Do not have multiple
  agents modify the same function concurrently.
- Use `decomp-verifier` after implementation for an independent mechanical
  object/assembly and full-ELF parity check before committing.
- Subagents must stay within the one function selected for the current attempt.

For each selected function:

1. Find its nonmatching `INCLUDE_ASM` placeholder.
2. Read the corresponding generated assembly.
3. Use headless Ghidra MCP for decompiler output, signature, xrefs, globals,
   callers, callees, and nearby functions. Find out what the function does and give it a name accordingly.
4. Read nearby source and relevant headers.
5. Use `python3 tools/m2ctx/m2ctx.py <source-file>` when more context helps.
6. Replace only the selected placeholder with compatible C/C++.
7. Compile and inspect compiler errors.
8. Diff the candidate object/assembly against the original.
9. Iterate until the function matches or record a concrete blocker.
10. Run the full build/parity check before treating progress as durable.
11. Send the status push notification (see Mobile Status Notification).

The resulting binary MUST match byte-for-byte. You can not just match intent, behavior, or even same behavior but with a different instruction. It must be a perfect match.

Prefer small leaf functions and one function at a time. Preserve old compiler
compatibility and existing project style. Do not rewrite unrelated code. If
repeated attempts fail, record the concrete blocker and continue elsewhere.

For an experimental candidate that needs a temporary assembly fallback, retain
the project's existing `INCLUDE_ASM` path while iterating. Only remove it after
the candidate object or function assembly has been compared mechanically.

In C++ files, avoid creating `extern "C"` prefixed functions with manualled mangled names and instead create them as pure C++ functions and let the compiler mangle the names like it should.

Mangling note (verified 2026-09-04): this EGC v2.73a build uses old cfront-style
mangling. For parameterized methods the established recipe works — a free C++
function named `className_method` taking the object pointer first mangles to the
binary's `className_method__F...` name (e.g. `readBufCreate(ReadBuf*)` ->
`readBufCreate__FP7ReadBuf`). For ZERO-argument methods (`...__Fv`) that recipe
fails: a free function named exactly `music_Unpause__Fv` mangles to
`music_Unpause__Fv__Fv`, and a real class method mangles cfront-style
(`Unpause__3music`). To emit the exact binary name for such methods, declare a
real class method with an asm label on the IN-CLASS declaration only (EGC's
parser rejects `asm()` on the out-of-class definition):

```cpp
class music { public: void Unpause() asm("music_Unpause__Fv"); };
void music::Unpause() { ... }
```

The codegen is then a true C++ instance method (`this` in `$a0`, unused unless
referenced); the label does not affect register allocation.

## Commit Discipline

Create one commit per successfully decompiled function. A function is not ready
to commit merely because the whole ELF still matches while the assembly fallback
remains active. Before committing, require all of the following:

1. The replacement compiles.
2. The candidate object or function assembly matches the original mechanically.
3. The full build succeeds and `cmp build/boot_elf.elf assets/boot_elf.elf`
   passes.
4. The function's durable state or investigation note is updated if needed.

Before each commit, inspect `git status`, `git diff`, and
`git log --oneline -10`. Stage only the selected function's intended source and
state files. Never stage generated assembly, build output, `userconfig.mk`,
local tools, credentials, or unrelated existing changes. Use a concise commit
message such as `decomp: match FunctionName`. If parity or the function-level
diff fails, do not commit; record the concrete blocker and continue without
claiming the function is matched.

## Mobile Status Notification

After each finished function attempt — matched, blocked, or bailed on — send
exactly one push notification to the owner's phone from the repo root using
`brrr.py` (stdlib only, works without the venv):

```sh
python3 brrr.py -t "RC1 decomp" -s "matched strfile_findpos" \
  "Matching decomp of string function to find position of a character. 57 nonmatching left"
```

Format rules — mobile pushes get truncated, keep the whole thing under ~200
characters:

- `--title`: always `RC1 decomp`.
- `--subtitle`: the function name plus its outcome: `matched <name>`, or
  `blocked <name>: <one-phrase reason>` (e.g. `blocked func_0023AEE0: EGC
  3-store tail reorder`). If no function was finished, use a short
  description of the session outcome instead.
- message body: description of the work done and the nonmatching count from
  `python3 tools/decomp_status.py --count`.

Do not paste diffs, decompiler output, or verbose note contents into the notification.
If `brrr.py` fails (network down, rotated token), note it in the attempt
record and continue; the notification is not part of the completion oracle.

## Autonomous Loop

The checked-in outer loop is `tools/run_autonomous_decomp_loop.sh`. It activates
`.venv`, reuses or starts the headless Ghidra backend, waits for its HTTP API,
invokes `tools/decomp_status.py --complete`, and only stops after the
mechanical completion check passes. It then runs `opencode run` for the next
iteration.

Start the complete autonomous process with one command:

```sh
tools/run_autonomous_decomp_loop.sh
```

The script can validate headless setup without starting opencode:

```sh
tools/run_autonomous_decomp_loop.sh --check-only
```

If the script starts Ghidra itself, it cleans that process up when stopped. An
already-running backend is reused and is not stopped by the script.


Update this document as you learn about the project, and new and better strategies to progress with decompilation. 
