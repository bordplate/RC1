# videoDecSetStream (0x0023CBD0, file 0x13DB50, 0x20 bytes)

## What it does

Registers a movie callback: `func_0012AEC8(self, a, b, cb, userdata); return 1;`.
func_0012AEC8 is a handwritten libSCE stub (code/_generated/sce/lib.s:24492)
that maintains a callback table at self+0x40: entries are 3-word
(key1, key2, payload) records keyed by (a, b); on a hit it overwrites the
record with (key1, DAT_00132EE0 + a*0x10, cb) or (key1, ..., userdata).
Callers (initAll__Fiii at 0x23A8D0/0x23A8F4):
videoDecSetStream(movie, 0, movie, videoCallback, 0) and
videoDecSetStream(movie, 3, movie, pcmCallback, s3) — the "movie" arg is
*(0x16120C)+0x10.

## Mangled-name decoding (function pointers)

`videoDecSetStream__FP8VideoDeciiPFP7sceMpegP13sceMpegCbDataPv_iPv` =
(VideoDec*, int, int, **int (\*)(sceMpeg\*, sceMpegCbData\*, void\*)**,
void*). cfront encodes a function-pointer type as `PF` + <param types> +
`_` + <return type in underscore form: _v void, _i int, _l long>. The
trailing `Pv` after `_i` is the 5th OUTER parameter (void* userdata), not a
callback parameter — a 4-param reading drops the userdata arg the callers
actually pass (a4=0 / a4=s3). Verified empirically:
`int f(int (*)(A*,B*,void*), int, void*)` mangles to `__FPFP7...Pv_iiPv` —
funccptr token `PFP...Pv_i` (params + `_` + int return), then plain outer
`iPv`; void/long returns give `_v`/`_l`. The underscore forms only appear
for the return type inside the funccptr token; outer params keep plain
forms. The 3-param callback matches the game's callback family
(mpegNodata__FP7sceMpegP13sceMpegCbDataPv = (SceMpeg*, SceMpegCbData*,
void*) style).

## Implementation

```cpp
struct sceMpeg;
struct sceMpegCbData;
typedef int (*videoDecCallback)(struct sceMpeg*, struct sceMpegCbData*, void*);
extern "C" int func_0012AEC8(VideoDec* self, int a, int b,
    videoDecCallback cb, void* userdata);

int videoDecSetStream(VideoDec* self, int a, int b, videoDecCallback cb, void* userdata) {
    func_0012AEC8(self, a, b, cb, userdata);
    return 1;
}
```

5-arg passthrough (a0-a4 untouched, nop in jal delay slot), `li v0,1`
scheduled between lq and jr. 0x10 frame.

## Verification

- Object: 8/8 words identical; R_MIPS_26 on func_0012AEC8 resolves to the
  original jal word 0x0C04AB2B (target 0x12AEC8).
- Full `make` + `cmp`: identical. Count 805 -> 804. decomp-verifier MATCH.
