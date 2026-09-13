# RC1 Style Guide

This guide covers hand-written source in the RC1 matching decompilation.
`AGENTS.md` defines the full decompilation and verification workflow.

The primary goal is a byte-for-byte match with the original NTSC boot ELF.
When a style rule conflicts with verified compiler output, the match takes
priority. Keep any necessary exception local and explain why it exists.

## Core Principles

- Write the smallest readable source that accurately describes the original
  behavior.
- Treat the original binary, compiler output, and mechanical comparisons as
  evidence. Plausible behavior alone is not a match.
- Improve names and types when evidence supports them. State uncertainty rather
  than presenting a guess as fact.
- Do not restyle or refactor matching code without a concrete reason. Equivalent
  source can produce different EEGCC output.
- Do not use magic numbers in any way. There should not be magic numbers in 
  command, pointers, struct offsets, or similar. If a magic number is absolutely
  necessary for matching decompilation, then it should be in a descriptive variable
  or define. Exceptions apply to padding bytes and buffer sizes where intention is clear.
- Static addresses are **never** allowed for pointers. You cannot assume a particular memory 
layout because it defeats the purpose of a decompilation project.

This is **NOT** allowed:
```
    if ((*(int*)0x15EEB4 ^ 1) & 1) {
        *(int*)0x15EEB0 = 3;
```

Instead add the symbols to `symbols.txt`:
```
menuPostCallbackIndex = 0x0015EEB0;
menuPostFlags = 0x0015EEB4;
```

And reference them by `extern`:
```
extern int menuPostCallbackIndex __attribute__((section(".data")));
extern int menuPostFlags __attribute__((section(".data")));

[...]
    if ((*(int*)menuPostFlags ^ 1) & 1) {
        *(int*)menuPostCallbackIndex = 3;
```


Violations of this style guide that already exist in the codebase are not excuses, 
reasons, or precedence for further violations. 

## Language And Toolchain

Game code under `code/game/` is generally C++. The sound library at
`code/989snd/ee/989snd.c` is C. Keep the language assigned by
`config/RC1.yaml` unless binary evidence justifies changing it.

The project uses SN Systems EEGCC 2.95.2:

- Do not use C++11 or newer features.
- Use the integer aliases in `code/include/types.h`.
- Under this ABI, `int` and pointers are 32 bits, `long` is 64 bits, and
  `long long` is 128 bits.
- Avoid unnecessary library abstractions, constructors, templates, exceptions,
  and RTTI. They can introduce hidden code or unsupported features.

## Formatting

- Indent with four spaces. Do not use tabs in C or C++ source.
- Put opening braces on the same line as declarations and control statements.
- Put one space after control-flow keywords and around binary operators.
- Write pointer and reference declarators next to the type: `MobyInstance* moby`
  and `u8** data`.
- Keep lines near 100 columns when practical. Wrap long argument lists in the
  style of the surrounding file.
- Use braces for nested, multiline, or potentially ambiguous control-flow
  bodies. They may be omitted for one short, clear statement.
- Do not run an automatic formatter over a translation unit.

Give code room to breathe:

- Put one blank line between top-level functions and declaration groups.
- Within a function, use blank lines to separate logical phases such as input
  checks, setup, the main operation, and cleanup or return handling.
- Separate local declarations from a following block of unrelated work when it
  improves scanning.
- Do not insert a blank line after every statement or split a short, cohesive
  operation into fragments.

```cpp
int readBufBeginPut(ReadBuf* buf, u8** out) {
    int space = buf->capacity - buf->count;

    if (space) {
        *out = buf->data + buf->putPos;
    }

    return space;
}
```

Match-sensitive statement order and expression shape take precedence over these
spacing preferences.

## Includes And Declarations

- Use quoted project includes and include `common.h` first in game translation
  units.
- Use include guards following the existing `NAME_H` convention.
- Put shared, confirmed structures and APIs in `code/include/`. Keep incomplete
  or translation-unit-specific types local.
- Declare every function with its complete, evidence-based prototype. Return
  type, signedness, and unused parameters can change generated code.
- Group declarations by subsystem or purpose, with a blank line between groups.

## Names And Unknown Symbols

Preserve known original names and established subsystem prefixes. The original
code is not expected to follow one modern naming convention.

- Use descriptive lower camel case for local variables.
- Name unknown structure fields by offset, such as `field_0x18`, and padding by
  offset, such as `pad_04`.
- Keep meaningful addresses, offsets, masks, and sizes in hexadecimal.
- Do not invent a descriptive name without supporting evidence.

Whenever touching an unknown function or global, including symbols named
`func_<address>`, `D_<address>`, or `DAT_<address>`, the agent MUST investigate
its behavior and references and try to give it an accurate descriptive name.
Update symbol configuration and all affected references when renaming it.

If its purpose cannot be inferred, retain the address-based name and add a
comment immediately above its declaration, definition, or `INCLUDE_ASM`
placeholder. The comment MUST state what is known and why the symbol remains
unknown. Do not use a generic comment such as `// Unknown function`.

## C And C++ Linkage

In a `.cpp` file, assume a free function is C++ unless the binary provides
evidence of C linkage.

- Write natural C++ identifiers and let EEGCC produce cfront-style mangling.
- Use exact mangled names only where tooling requires them, such as an
  `INCLUDE_ASM` placeholder.
- Use `extern "C"` only for a function confirmed to have C linkage.
- Every `extern "C"` function declaration or definition MUST have a comment
  immediately above it that states the evidence or ABI reason for C linkage.
- Every use of `asm("symbol")` MUST have a comment immediately above it that
  explains why natural linkage or mangling cannot produce the required symbol.
- Do not use either construct merely to make linking easier.

```cpp
// C linkage: the implementation is supplied by generated sce/lib.s, and the
// original call target is the unmangled SDK symbol FlushCache.
extern "C" void FlushCache(int mode);

// C linkage: this helper is implemented as handwritten VU assembly in the
// generated fast-function region; its entry point is not cfront-mangled.
extern "C" void draw_transformVector(void* out, void* in, void* matrix);

// C linkage: this memory-card routine is implemented in generated sce/lib.s;
// memcard_Init calls its unmangled entry point at 0x001233F0.
extern "C" int memcard_queryStatus(void);

// Symbol override: the ELF uses an unmangled callback name despite C++ types.
void RefreshPointLight(int index) asm("RefreshPointLight");
```

Apply the same evidence standard to function-pointer types, repeated parameter
types, and return values. Confirm emitted names with `nm` when necessary.

## Types And Data Layout

- Match the observed size and signedness of every field and access.
- Order structure fields by observed offset and represent unknown gaps with byte
  arrays rather than invented members.
- Keep offset-based field names until their purpose is supported across known
  uses.
- Add `packed`, `volatile`, unions, unusual alignment, or section attributes
  only when layout or compiler-output evidence requires them.
- Avoid `bool` unless its ABI and access width are established.
- Preserve exact global addresses and section behavior. A wrong but linkable
  global can differ only in relocation bytes.
- Distinguish resident boot symbols from level-overlay symbols before moving
  declarations into shared code.

Use short factual comments for uncertain layouts, for example:

```cpp
// Layout is known through offset 0x58; remaining fields are unconfirmed.
```

## Match-Sensitive Code

- Preserve statement order, expression grouping, temporary placement,
  signedness, and prototypes when they affect output.
- Use explicit register variables, `volatile`, inline assembly barriers, or
  constant-address casts only after a comparison proves they are needed.
- Add a short comment above a non-obvious matching construct explaining the
  compiler constraint.
- Keep compiler flags at translation-unit scope. Prefer an exact Splat boundary
  and private object flag over project-wide or per-function hacks.
- Never patch emitted instructions, hide a mismatch behind raw assembly, or
  enable `ALLOW_NONMATCHING` globally.

## Assembly And Generated Files

Keep unmatched functions in original order with `INCLUDE_ASM`. Replace only the
selected placeholder and remove it only after a mechanical function comparison.

- Do not edit or commit `code/_generated/` or `build/`.
- Change `config/RC1.yaml` and `config/symbols.txt` deliberately, then rerun
  `make split`.
- Keep standalone probes outside `code/` so they cannot enter the game build.

## Comments

- Explain recovered intent, layout constraints, hardware behavior, uncertainty,
  or non-obvious matching requirements.
- Do not narrate straightforward assignments.
- Use `unknown`, `likely`, or `unconfirmed` when appropriate.
- Keep detailed experiments in `decomp_state/` and production comments concise.

## Verification

A replacement is matched only after its object or function bytes and the full
boot image compare successfully:

```sh
source .venv/bin/activate
make clean && make split && make -j2
cmp build/boot_elf.elf assets/boot_elf.elf
python3 tools/decomp_status.py --count
```

`python3 tools/decomp_status.py --complete` is the project-wide completion
check. A successful build that still uses an assembly fallback does not prove a
candidate C or C++ implementation matches.
