# camera_Camera_Pos2Polar3d__FP7PolarSmP4vec4N41 (0x1EC530, 0x1DC)

Matched 2026-09-23.

## Signature

`void Camera_Pos2Polar3d(PolarSm* res, vec4* pos, vec4* center, vec4* fwd, vec4* side, vec4* up)`
(cfront `__FP7PolarSmP4vec4N41` — repeated `vec4*` encodes as `N41`, 0-based index
of the first `vec4*` parameter). Deadlocked twin `Camera_Pos2Polar3d__FR7PolarSmR4vec4N41`
(reference/dl ~108519) confirms the name and the PIHALF - asin polar structure.

`PolarSm { float azimuth, elevation, radius; }` (code/include/camera.h). `vec4`
is the 16-byte vector from code/include/mobyutil.h.

## Behavior

Decomposes the position relative to `center` into a polar orientation:

- `v0 = pos - center`, `r = dot(v0, up)`, `upn = normalize(up, r)`,
  `v1 = v0 - upn` (the up-free component of the position).
- azimuth: `r1 = dot(fwd, v1)`, `len1 = |v1|` (clamped to 1e-4 if zero),
  `ang = PIHALF - asin(r1/len1)`; sign-flipped when `dot(side, normalize(v1)) < 0`.
  Written to `res->azimuth`.
- `func_00214890(ang, &tmp, fwd, up)` rebuilds a forward vector from the azimuth;
  `r2 = dot(tmp, v0)`, `len0 = |v0|` (clamped), `t = PIHALF - asin(r2/len0)`;
  `res->elevation = (dot(up, normalize(v0)) < 0) ? t : -t`.
- `res->radius = |v0|`.

Constants: PIHALF = 1.5707964f, EPS = 0.0001f, ONE = 1.0f.

Callees: `FastVecSub/Dot/Length/Normalize` (C linkage, game/fastfunc) and
`FastArcSin` (C++ `FastArcSin__Ff`) — prototypes added to code/include/common.h.
`func_00214890` is a still-unresolved mobyutil.cpp target (INCLUDE_ASM there);
declared `extern "C"` in camera.cpp against its unmangled symbol.

## The blocker: EGC 2.95.2 f21/f22 float-register swap

The only thing separating the natural C form from the original was which of the
two float constants `{0.0, PIHALF}` the compiler parks in callee-saved FPR f21
vs f22:

- original: f21=PIHALF (then overwritten in-place by `t`), f22=0.0
- local EGC: f21=0.0, f22=PIHALF

That one swap cascades into the whole tail: the original's first angle sub is
`sub.s $f20,$f21,$f0` (ang fresh in f20) and its second is `sub.s $f21,$f21,$f0`
(t in-place in f21, so block 2 is `neg.s $f20,$f21` / `mov.s $f20,$f21` / one
store). With PIHALF in f22 both subs become `sub.s $f20,$f22,$f0` and block 2
degenerates to a store-in-delay-slot + double store. 13 word diffs, all from
this swap.

Source forms tried (all on default camera.o flags): fresh PIHALF literal
(PIHALF->f22), splitting the asin into a temp (PIHALF->f22), PIHALF as a named
local `pihalf` in various declaration orders / lifetimes (PIHALF->f23), and
assigning PIHALF to the rotating value before the eps check (`ang = PIHALF;
ang = ang - asin;`, which DID put PIHALF in f21 but then `ang` itself lives in
f21 and the load hoists). The fundamental tension: the original RESERVES f21
for PIHALF (so 0.0 falls to f22 and the 2nd sub is in-place in f21) yet keeps
the PIHALF `lui/ori/mtc1` load LATE at 0x1EC5F0, right before the first asin.

## The matching form

An UNINITIALIZED `$f21` register variable reserves the FPR without forcing an
early load; the PIHALF constant is then assigned just before the first asin so
the load lands at the original late position:

```cpp
register float pihalf asm("$f21");          // uninitialized on purpose
...
pihalf = 1.5707964f;
ang = pihalf - FastArcSin(r1 / len1);       // sub.s $f20,$f21,$f0 (ang fresh in f20)
...
pihalf -= FastArcSin(r2 / len0);            // sub.s $f21,$f21,$f0 (t in-place in f21)
asm volatile("" : "+f"(pihalf));            // keep the second sub in-place in $f21
FastVecNormalize(&v1n, &v0, 1.0f);
d2 = FastVecDot(up, &v1n);
asm volatile("");                            // stop `ang = -pihalf` hoisting across the dot
ang = -pihalf;                              // neg.s $f20,$f21
if (d2 < 0.0f)
    ang = pihalf;                           // mov.s $f20,$f21 (bc1f delay-slot layout)
res->elevation = ang;                       // single swc1 $f20,4($20)
```

The `"+f"` zero-byte barrier defeats constant folding / register coalescing so
`pihalf -= asin(...)` updates `$f21` in place instead of landing in f20. The
`ang = -pihalf; if (d2<0) ang = pihalf;` default-plus-overwrite form (not the
ternary `(d2<0)?pihalf:-pihalf`) is what reproduces the `neg.s`-in-`bc1f`-delay
slot + `mov.s` + single-store layout.

A hard-initialized pin (`register float pihalf asm("$f21") = 1.5707964f;`) does
NOT work: the top-of-function initializer makes PIHALF live from the entry, so
EGC hoists the load to 0x1EC58C and the size grows to 0x1E0. The uninitialized
declaration + late assignment is the only form that reserves f21 while keeping
the load late.

## Verification

Standalone probe (default flags, `--define func_00214890=0x214890`) matched all
0x1DC bytes (0 diffs). Full build + `cmp build/boot_elf.elf assets/boot_elf.elf`
passes.
