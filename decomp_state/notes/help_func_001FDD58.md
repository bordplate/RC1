# Help_DisplayMessage (0x1FDD58) — MATCHED (2026-10-07)

Function: `code/game/help.cpp` : `Help_DisplayMessage` (VRAM 0x1FDD58, size 0x134,
308 bytes / 77 words, no-arg void). Splat placeholder was `func_001FDD58`; the state
machine `Help_Update` (func_001FE050, INCLUDE_ASM) `jal`s it at 0x1FDFB0 and 0x1FE0D8,
so the C++ definition keeps the label via `asm("func_001FDD58")`.

## Semantics
Displays the currently-active help message:

```
void Help_DisplayMessage(void):
    g_helpState.state = 1;             // active
    g_helpState.counter = 0;           // unconditional (sits in the bnez delay slot)
    if (sceneSoundFlag1 || sceneSoundFlag0)
        func_0022DB10(0, 1, 0);        // play the message sound (sound.cpp, still INCLUDE_ASM)

    FontWindow window;                          // 24-byte local at sp+0x10
    u8* text = (u8*)HelpMsgs[g_helpState.field_0x20].text;
    FontSetWindow(&window, 0xF0, 0x1E0, 0x2C, 0x1D4, 0x100, 0x168, 0x10, 7);
    FontPrintWindowMedium(&window, 0x80FFA888, text, -1);   // -1 = null-terminated

    // layout geometry, derived from the rendered text size + draw height
    int maxTextH = window.maxTextH;           // A = buf+0x0C
    int totalH   = window.totalH;             // B = buf+0x0E
    int base     = occlViewParams.paramY;     // draw height (0x13E504)
    int bottom   = base - 0x3C;
    int offset   = (totalH >> 1) + 5;
    g_helpState.field_0x08 = (maxTextH >> 1) + 10;
    g_helpState.field_0x10 = 0x100;
    g_helpState.field_0x0C = offset;
    g_helpState.field_0x18 = 8;
    g_helpState.field_0x1C = 8;
    g_helpState.field_0x14 = bottom;
    if ((base - 0xC) < bottom + offset) {
        int cap = (totalH >> 1) + 0x11;
        g_helpState.field_0x14 = base - cap;
    }
```

Callees:
- `func_0022DB10(int,int,int)` (0x22DB10, sound.cpp, still INCLUDE_ASM): the message
  sound channel alloc. Address-based name retained (still a placeholder); declared
  `extern "C"` (unmangled entry point).
- `FontSetWindow` (0x1F7668, 9 params) and `FontPrintWindowMedium` (0x1F75F0, 4 params),
  both in draw_post_post.cpp; prototypes + the shared `FontWindow` struct moved to the
  new `code/include/font.h`.
- `sceneSoundFlag0` (0x15EE1C) / `sceneSoundFlag1` (0x15EE1D): two per-scene u8 bytes,
  read here (absolute self-based `lui/lbu`) to gate the sound. Named tentatively — see
  below. Byte-level symbols live in `config/linker_aliases.ld` (see KEY LEARNING 5).

`sceneSoundFlag` naming is unconfirmed: both bytes sit in the core.lit region and are
cleared together at scene init (FUN_00226b08) and read here plus at 0x215734 / 0x1FE9C0.
The name reflects the observed use (gate a scene sound) but the exact semantics are not
established; kept as a `refactor.json` follow-up.

## KEY CODEGEN LEARNINGS
1. **9-arg call window.** EGC passes FontSetWindow's 9 int args as: 1-4 in a0-a3, 5-8 in
   t0-t3, and the 9th (flags=7) on the stack at sp+0 (the callee does `lw v0,0(sp)`).
   Confirmed from the FontSetWindow body.
2. **`||` form for the sound test.** `if (sceneSoundFlag1 != 0 || sceneSoundFlag0 != 0)`
   reproduces the original two-load + `or`-free test. An `&&`/single-variable form
   emits a `movz` and breaks the block. `counter = 0` is UNCONDITIONAL (it is the always
   -executing bnez delay slot), so it must not be folded into the branch.
3. **Color is 0x80FFA888** (NOT 0xA88880FF): `li a1,0x80ff; dsll 16; ori 0xa888`.
4. **Store order + conditional grouping.** The six `g_helpState` stores emit in machine
   order `0x08, 0x10, 0x1C, 0x0C, 0x18, [beqz -> 0x14(delay=bottom) / 0x14(base-cap)]`.
   Source order `field_0x0C; field_0x18; field_0x1C` (after 0x08/0x10) reproduces the
   `0x1C, 0x0C, 0x18` machine order — EGC right-rotates a naive `0x1C,0x0C,0x18` source
   order to `0x18,0x1C,0x0C`. The conditional store is "unconditional `= bottom` (lands in
   the beqz delay slot) then conditional overwrite". The overwrite value must be computed
   as `(totalH>>1)+0x11` FIRST then `base - it`; writing `base - ((totalH>>1)+0x11)`
   inline makes EGC re-associate to `(base-0x11)-(totalH>>1)` (swapped addiu/subu).
   Declaring `int cap = (totalH>>1)+0x11;` INSIDE the if-block keeps the addiu in the
   branch; hoisting it before the if disturbs the store schedule.
5. **Byte-level data symbols go in linker_aliases.ld, not symbols.txt.** 0x15EE1C/1D are
   in the core.lit (lit4) region; Splat 0.50 does not rename lit4 byte symbols from
   `symbol_addrs_path` (levelRoot@0x15EE4C in the same region is defined in
   `config/linker_aliases.ld`, as are the `menuItemEnabled_*` bytes). Declaring them in
   symbols.txt leaves the `.ld` with `D_` names and the link fails with "undefined
   reference". Defining them in `linker_aliases.ld` (`Name = 0xADDR;`, passed via `-T`)
   resolves the `extern u8` references; under `-G8` SN a plain `u8` extern in the gp
   window emits a bare pseudo ps2eeas expands to the self-based absolute `lui/lbu` the
   original uses.

## Verification
`decomp_probe.py working/help_func_001FDD58/cand.cpp <ref .s> func_001FDD58`: candidate
308 bytes, 0 differences. `make split && make -j2` then
`cmp build/boot_elf.elf assets/boot_elf.elf` => byte identical. Count 566 -> 565.

## Refactor: named window constants + flag rename (2026-10-07)
Cleared refactor entry `help_func_001FDD58_window_consts`. Two changes, both
byte-identical (verified: full `make split && make -j2`, `cmp` byte-identical,
`tu_assembler_diff help.o` 14/14 match, count 565 unchanged):

1. **Magic numbers named** (textual `#define` substitution above the function):
   - FontSetWindow args -> `HELP_MSG_WIN_TOP/BOT/LEFT/RIGHT`, `HELP_MSG_TEXT_X/Y`,
     `HELP_MSG_LINE_SPACING`, `HELP_MSG_FONT_FLAGS`. The first four are the window
     top/bottom/left/right bounds (NOT x/y/w/h): confirmed against Deadlocked's
     `FontSetWindow(FontWindow*, win_top, win_bot, win_left, win_right, text_x,
     text_y, line_spacing, flags)` (reference/dl/game_dl/fonts.cpp:1191). The values
     form a 424x240 box x[0x2C,0x1D4] y[0xF0,0x1E0], centered, lower screen.
   - Color `0x80FFA888` -> `HELP_MSG_TEXT_COLOR` (ARGB: 50% alpha over 0xFFA888; the
     renderer func_001FE980 reprints the same RGB each frame with alpha animated from
     the state counter). Length `-1` -> `HELP_MSG_TEXT_LENGTH` (null-terminated).
   - Layout geometry -> `HELP_MSG_CENTER_X` (0x100 = screen center X, field_0x10),
     `HELP_MSG_TEXT_Y_FROM_BOTTOM` (0x3C; text Y = drawH-0x3C, field_0x14 default),
     `HELP_MSG_BOTTOM_CLEARANCE` (0xC clamp margin), `HELP_MSG_TEXT_PAD_V` (5 = box
     half-height beyond totalH/2, field_0x0C), `HELP_MSG_TEXT_PAD_H` (10 = box
     half-width beyond maxTextH/2, field_0x08), `HELP_MSG_BOX_INIT_HALF` (8 =
     field_0x18/0x1C initial box half-size), `HELP_MSG_CLAMP_Y_FROM_BOTTOM` (0x11 =
     clearance + v pad, the clamped text-Y offset).
   - NOTE: `maxTextH` (FontWindow+0x0C) is actually the max line WIDTH and `totalH`
     (+0x0E) the total text HEIGHT, per DL's `max_width`/`max_height`. The FontWindow
     field names (x/y/w/h/maxTextH/totalH) in code/include/font.h are left as-is —
     renaming them is a separate font.h+draw_post_post.cpp concern, not this entry.

2. **sceneSoundFlag0/1 renamed** to `sceneHelpMsgFlag0/1` (help.cpp externs +
   config/linker_aliases.ld). Verified semantics (supersedes the "unconfirmed" note
   above): lit4 bytes, init 0x01, cleared together at scene init (0x226B08); they
   gate the whole help-message display — Help_DisplayMessage plays the message sound
   func_0022DB10(0,1,0) and the renderer func_001FE980 draws the box only when at
   least one is set — plus flag1 (0x15EE1D) holds the fade-out 4 ticks in Help_Update
   case 6, and flag0 (0x15EE1C) selects a sound param in FUN_002156D8. Generated .s
   files reference D_0015EE1C/D_0015EE1D (a separate data symbol), so the rename only
   touched the two source files.
