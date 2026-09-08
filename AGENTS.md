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

Splat misdecode quirk (verified 2026-09-08): Splat's disassembler prints
`daddu $2, $0, $0` for the instruction word 0x0000102D, which is actually
`move v0,zero` (or v0,zero,$0). So every "daddu $2,$0,$0" in generated `.s`
files is a zero-move, not a daddu (all 313 occurrences in the boot ELF). The
EGC `return 0;` / `int x = 0;` idiom is this move encoding; objdump of
`assets/boot_elf.elf` is ground truth whenever Splat's mnemonic and the hex
field disagree (see decomp_state/notes/memcard_TestChecksum.md).
- Compiler output and object/assembly diffs judge candidate quality.

Ghidra Gp-name pitfall (verified 2026-09-05): Ghidra names gp-relative data
accesses as `iGp/PuGpffffXXXX` where `ffffXXXX = 0x100000000 - offset`, i.e.
the value is `gp + (0xffffXXXX as signed)` — e.g. `puGpffff8080` is gp - 0x7F80
= 0x15EC80, NOT 0x15EE80 (the `D_0015EE80` label in `.lit` is a different
symbol 0x200 away). Always convert the offset to an absolute address (gp =
0x166C00) before naming the extern. Declaring the wrong-but-valid label links
cleanly and fails parity at exactly the GPREL16 displacement bytes (see
decomp_state/notes/snd_PrepareReturnBuffer.md).
- `tools/decomp_status.py --complete` is the mechanical completion check.

## Local Toolchain

Before retrying a blocked function, follow `decomp_state/compiler_workflow.md`.
It provides executable isolated probes, known-good/known-bad controls, and a
decision tree. Historical blocker notes are hypotheses, not ground truth:
`scTag2` was unblocked by correcting a misread shift, and the menu edge test
by a per-translation-unit flag with the existing compiler. Neither required
an Insomniac compiler patch. Read any `correction` field before an older note.

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

The Makefile now tracks same-stem generated matching/nonmatching assembly,
top-level project headers, Makefile, and userconfig.mk for C/C++ objects.
Cross-directory/nested includes and command-line flag overrides are not fully
tracked: use a clean verification (or `make -B`) after such experiments.
After renaming symbols, rerun split and force the affected objects to rebuild.

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
`move v0,zero` (0x0000102D; Splat misdecodes it as `daddu $v0,$0,$0`)
above the stores (see decomp_state/notes/pause_func_002223D8.md).

Observation observed 2026-09-04 (single-call wrappers): a zero-arg C function
whose only statement is a call passing literal 0 (`callee(0);`) compiles to a
0x10 frame with `sq/lq $ra` at 0(sp) and the argument load `daddu $4,$0,$0`
(word 0x0000202D — objdump prints it as `move a0,zero`) hoisted into the `jal`
delay slot; the epilogue is the standard `lq; jr; <ds addiu>`. The callee MUST
be declared with a parameter (e.g. `int`) in the extern prototype — a `(void)`
 prototype suppresses the argument setup and the delay slot comes out as `nop`,
 breaking the match (see
 decomp_state/notes/stash_func_00232CE0.md for the matched function).

 Observation observed 2026-09-05 (prologue/body interleaving depends on the
 body kind): whether EGC emits the `sq $ra` prologue store BEFORE or AFTER an
 independent body instruction is not fixed. An INDEPENDENT constant store (not
 feeding a call argument), e.g. `GLOBAL = 0; callee();` with a plain `extern
 int GLOBAL;` (GP-relative `sw zero,off(gp)`), is hoisted BEFORE the `sq ra`:
 `addiu sp; sw zero,off(gp); sq ra; jal; nop; lq; jr; addiu sp` — matches the
 original snd_UnkFunction_0012eb00 (see
 decomp_state/notes/989snd_snd_UnkFunction_0012eb00.md). But a body that
 MATERIALIZES A CALL ARGUMENT (a `lui/lw` loading a pointer to pass), e.g.
 `callee(*(void**)0xADDR);`, keeps the `sq ra` FIRST: `addiu sp; sq ra; lui;
 lw; jal` — that ordering could NOT be made to match the original PutDispBuffer,
 whose `lui/lw` sit before the `sq ra` (blocked, see
 decomp_state/notes/framebuf_PutDispBuffer.md). So: independent-store bodies
 match, argument-load-body prologue order does not (yet).

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
Also: a `jal` to a function that is DEFINED LATER IN THE SAME TU (whether a
later C definition or a later `INCLUDE_ASM` block) relocation-references the
`.text` section symbol (addend 0) instead of the function symbol (verified
2026-09-05 with a standalone EGC+ld test: every same-section call to a
later-defined target gets this reloc, at any offset). Mechanism: EGC writes
the 26-bit field as the target's offset within the section (>>2), and
ps2-elf-ld resolves R_MIPS_26 on the section symbol as
final_field = (section_VMA + old_field<<2) >> 2, so the linked word is the
exact section-absolute jal — byte-identical to the original. Judge such
relocs by the link result (full `cmp`), not the symbol name (see
decomp_state/notes/boot_main.md and
decomp_state/notes/989snd_snd_StopAllStreams.md).

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

Observation observed 2026-09-06 (signed vs unsigned comparison fixes the field
type): when a `(a < b) ? a : b` (or `a < b` in a branch) compiles to `sltu`
(funct 0x2B) but the original uses `slt` (funct 0x2A) — a single-word diff in an
otherwise byte-identical function — the compared struct field is SIGNED, not
unsigned. Declaring it `int` instead of `u32` flips EGC to `slt` and matches.
Also: the inline form (`int ret=(n<fld)?n:fld; fld-=ret; return ret;`) can match
where a separate `int avail=fld;` local mis-allocates (hoists the param into v0,
loads the field into v1, emits `movz` instead of `movn`) — prefer the inline
accessor form for small min/clamp helpers (see decomp_state/matched.json
entry readBufEndGet__FP7ReadBufi, matched with count as `int`).

Observation observed 2026-09-06 (EGC 8-arg register ABI — 7th/8th int args in
t2/t3): this EGC 2.95.2 build passes the 7th and 8th INTEGER parameters in
**t2/t3 ($10/$11)** — the register arg window is a0-a5, t0-t3, NOT the usual
a0-a5 + stack. Verified with snd_PlaySoundVolPanPMPB (0x12E308, 8-int wrapper
that forwards g/h into the IOP x/y): the original forwards t2/t3, and BOTH
callers (0x22D65C, 0x22EB84-8C) load t2/t3 as their FINAL call-setup right
before the jal. A standalone probe of the exact C (6 params to buf[6] + 2
forwarded) compiled with the project flags emitted all 18 words byte-identical,
including EGC's idiosyncratic `move v0,a3` (save arg4 before a3 is reused),
`sw t1,20(sp)` in the `jal` delay slot, and `move t0,t3` (arg8→a4) clobbering
arg5's register after its buf store. So when a function has 7+ int params,
declare all 8 and expect the 7th/8th to arrive in t2/t3 — do not model them as
stack args (see decomp_state/notes/989snd_snd_PlaySoundVolPanPMPB.md).

## Durable State

`decomp_state/` is agent memory, not the success oracle:

- `queue.json` contains the enumerated targets.
- `matched.json` may record durable matched-function notes.
- `blocked.json` maps target IDs to concrete blocker notes.
- `attempts/` stores optional per-attempt records.
- `notes/` stores investigation and strategy notes.

The status tool enumerates source files directly, so stale queue entries cannot
make the project appear complete. Every blocked target must have a non-empty
`note` or `reason` in `blocked.json`. Before adding any new blocker, the primary
agent must invoke `last-resort-decompiler` for that exact target and try any
concrete recommendation it returns. The blocker note must summarize that final
escalation and its result; if the usage-limited agent could not run because of
quota, authentication, or configuration, record the failed invocation instead.

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

Verified callee-prototype effect (2026-09-06): an ignored return value still
affects EGC register allocation. snd_SendCurrentBatch's 276-byte body matches
with `int sceSifCallRpc(...)`; declaring that callee `void` alone produces ten
epilogue word differences despite identical code before the call. Confirm
CALLEE return types, not just the selected function's return type, before
blaming allocator tie-breaks. See the snd_batch positive/negative probes.

Verified address-splitting scope (2026-09-06): `-mno-split-addresses` with
SYMBOLIC .data loads reproduces VU1_addDataRef's first 68 bytes, including all
five self-based lui/lw sequences. It does not fix repeated constant-address
casts, nor the final GP-relative store. No vuchain flag was enabled in the
production build. See its updated blocker note.

### Subagent Delegation

- Use `decomp-researcher` before implementing an unfamiliar target when Ghidra,
  assembly, caller, or data-layout research can be performed independently.
- Use `explore` for quick read-only repository searches that do not require the
  RC1-specific research report.
- Keep source edits and iteration in the primary agent. Do not have multiple
  agents modify the same function concurrently.
- Use `decomp-verifier` after implementation for an independent mechanical
  object/assembly and full-ELF parity check before committing.
- `last-resort-decompiler` uses the usage-limited GPT-5.6 Sol model. Do not use
  it for routine targets or initial research. Invoke it only after the primary
  agent has exhausted normal source, assembly, Ghidra, compiler-probe, and
  `decomp-researcher` work and is otherwise ready to add a blocker.
- No new entry may be added to `decomp_state/blocked.json` until
  `last-resort-decompiler` has been tried on that exact target. Apply and test
  its concrete recommendations before deciding the target remains blocked.
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
9. Iterate with normal tools until the function matches or all ordinary routes
   are exhausted.
10. Before recording a new blocker, invoke `last-resort-decompiler` for the
    target and implement and mechanically test any concrete recommendation.
11. Only then, if it still does not match, record a concrete blocker including
    the last-resort result or failed invocation.
12. Run the full build/parity check before treating progress as durable.
13. Send the status push notification (see Mobile Status Notification).

The resulting binary MUST match byte-for-byte. You can not just match intent, behavior, or even same behavior but with a different instruction. It must be a perfect match.

You should rename unnamed functions and globals when it becomes apparent what they do.

Prefer small leaf functions and one function at a time. Preserve old compiler
compatibility and existing project style. Do not rewrite unrelated code. If
repeated attempts fail, record the concrete blocker and continue elsewhere.

For an experimental candidate that needs a temporary assembly fallback, retain
the project's existing `INCLUDE_ASM` path while iterating. Only remove it after
the candidate object or function assembly has been compared mechanically.

In C++ files, avoid creating `extern "C"` prefixed functions with manualled mangled names and instead create them as pure C++ functions and let the compiler mangle the names like it should.

Mangling correction (verified 2026-09-06): this EGC v2.73a build uses old
cfront-style mangling. Free functions work for both parameterized and
zero-argument names: `readBufCreate(ReadBuf*)` produces
`readBufCreate__FP7ReadBuf`; `void PutDispBuffer(void)` produces
`PutDispBuffer__Fv` (verified by the linked probe). Remove the `__F...` suffix
when writing the source identifier; do not include it in the C++ name.
Do not invent a class/unused `this` merely to obtain a zero-argument symbol.
The older workaround below is only appropriate when evidence establishes a
real instance method and an unusual linker label is genuinely necessary:

```cpp
class music { public: void Unpause() asm("music_Unpause__Fv"); };
void music::Unpause() { ... }
```

The codegen is then a true C++ instance method (`this` in `$a0`, unused unless
referenced); the label does not affect register allocation.

Instruction-decoding corrections (2026-09-06): `dsll32 r,r,0` shifts LEFT by
32, and only a subsequent `dsrl32 r,r,0` completes zero extension. `or` works
on the 64-bit GPR value, not just its low word. `slti ...,0xBE` compares with
+190 (16-bit sign extension), not -66. See the scTag2 and menu threshold
probes before diagnosing an optimization bug from these instructions.

Verified flag scope (2026-09-06): the menu Splat segment is split at exact
function boundaries so conflicting scheduler requirements stay local.
`menu.cpp` (file 0x1078F8..0x109797) and `menu_post.cpp`
(0x109850..0x109FAF) use `-fno-schedule-insns`; `menu_callbacks.cpp`
(0x109798..0x10984F) uses `-fno-schedule-insns2`. This matches
menu_pointIsClockwise and menu_restoreSelection while preserving every prior
menu match and full boot parity. Sound-library and other files retain defaults.
It does NOT establish the original build's flags. Revalidate future candidates
with their owning TU's flag, and do not apply either scheduler flag globally.
When adjacent functions require conflicting verified flags, prefer this Splat
boundary split over per-function compiler hacks: add boundaries at function
file offsets in `config/RC1.yaml`, move source/INCLUDE_ASM paths to consecutive
TUs, assign object-private flags in Makefile, rerun split, inspect linker order,
and require a clean full-image comparison.

Mangling note (verified 2026-09-05): repeated parameter types are encoded
`Tn` with **n = 0-based index of the first parameter** — `T1` means "same
type as parameter 2", `T2` "same type as parameter 3", etc. (and `T0` =
parameter 1). So `videoDecBeginPut__FP8VideoDecPPUcPiT1T2` decodes to
(VideoDec*, u8**, int*, **u8**\*, **int**\*) and `cpy2area__FPUciT0iT0iT0i`
to (u8*, i, u8*, i, u8*, i, u8*, i). Do NOT read Tn 1-based: the wrong
signature (a) mangles to T0T1 instead of T1T2 and (b) trips a cfront parser
bug — declaring the callee with `u8**` params while calling it with a u8**
variable as one of several args fails with "type `X' is not a base type for
type `Y'" (bogus; e.g. "ViBuf is not a base type for VideoDec"). The correct
0-based signature compiles cleanly. Always confirm the mangled name with `nm`
on the built object before trusting a signature (see
decomp_state/notes/videodec_videoDecBeginPut.md).

Mangling note (verified 2026-09-05): a function-pointer type is encoded
`PF` + <parameter types> + `_` + <return type in underscore form
(`_v` void, `_i` int, `_l` long)>. So
`PFP7sceMpegP13sceMpegCbDataPv_iPv` is a 5th outer `Pv` param FOLLOWING the
callback token `PFP7sceMpegP13sceMpegCbDataPv_i` — the callback is
`int (*)(sceMpeg*, sceMpegCbData*, void*)`, 3 params. Do not absorb the
trailing `Pv` into the callback: the call sites (initAll__Fiii at
0x23A8D0/F4) pass 5 args (a0-a4). The underscore forms only appear for the
RETURN type inside the funccptr token (the `_` separator before it); outer
param types keep their plain forms — another reason to nm-check every
signature (see decomp_state/notes/videodec_videoDecSetStream.md).

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
- If `last-resort-decompiler` was invoked, include the exact phrase
  `last-resort GPT-5.6 Sol used` in the message body, whether it found a match,
  confirmed a blocker, or failed because the model was unavailable.

Do not paste diffs, decompiler output, or verbose note contents into the notification.
If `brrr.py` fails (network down, rotated token), note it in the attempt
record and continue; the notification is not part of the completion oracle.

## Closing notes

Update this document as you learn about the project, and new and better strategies to progress with decompilation. 
