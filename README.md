# 125A-Knob-Designer

125A Systems tool for designing and exporting high-quality VSTGUI knob filmstrips and reusable UI assets.

## Source of truth

This repository is the authoritative source for reusable rendered 125A GUI controls such as knobs, switches, selectors, LEDs and their multi-resolution exports.

Branding and logo masters remain authoritative in `challanger2000/125A-Branding`.

## Current successful MixEngine multi-resolution set

Approved package:

`125A-Knob-Designer-v0.1.0-MultiRes-success.zip`

Filmstrips are vertical, 128 frames each.

### MixEngine Analog L
- `exports/125A_MixEngine_Analog_L_128px_100pct_128f.png` — 128 × 16384
- `exports/125A_MixEngine_Analog_L_192px_150pct_128f.png` — 192 × 24576
- `exports/125A_MixEngine_Analog_L_256px_200pct_128f.png` — 256 × 32768
- `exports/125A_MixEngine_Analog_L_384px_300pct_128f.png` — 384 × 49152

### MixEngine Analog M
- `exports/125A_MixEngine_Analog_M_96px_100pct_128f.png` — 96 × 12288
- `exports/125A_MixEngine_Analog_M_144px_150pct_128f.png` — 144 × 18432
- `exports/125A_MixEngine_Analog_M_192px_200pct_128f.png` — 192 × 24576
- `exports/125A_MixEngine_Analog_M_288px_300pct_128f.png` — 288 × 36864

### MixEngine Analog S
- `exports/125A_MixEngine_Analog_S_64px_100pct_128f.png` — 64 × 8192
- `exports/125A_MixEngine_Analog_S_96px_150pct_128f.png` — 96 × 12288
- `exports/125A_MixEngine_Analog_S_128px_200pct_128f.png` — 128 × 16384
- `exports/125A_MixEngine_Analog_S_192px_300pct_128f.png` — 192 × 24576

## Integration contract

- preserve all 128 frames and their order;
- preserve S / M / L as distinct design sizes;
- preserve 100 / 150 / 200 / 300 % exports as a real multi-resolution set;
- do not replace the approved set with a scaled low-resolution strip;
- consuming plugins select the appropriate representation from the absolute UI/content scale;
- 0 % maps to frame 0 and 100 % maps to frame 127;
- integration must not alter DSP, parameter IDs, defaults, state or automation;
- always integrate into the current development branch/HEAD of the consuming plugin's authoritative Final repository.
