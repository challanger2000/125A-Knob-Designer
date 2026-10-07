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


## 125A Chrome Ring family

Reusable mirror-polished chrome bezel assets for 125A SteelKnob-based controls.

### Multi-resolution exports

#### Chrome Ring L
- `exports/chrome/125A_ChromeRing_L_128px_100pct.png`
- `exports/chrome/125A_ChromeRing_L_192px_150pct.png`
- `exports/chrome/125A_ChromeRing_L_256px_200pct.png`
- `exports/chrome/125A_ChromeRing_L_384px_300pct.png`

#### Chrome Ring M
- `exports/chrome/125A_ChromeRing_M_96px_100pct.png`
- `exports/chrome/125A_ChromeRing_M_144px_150pct.png`
- `exports/chrome/125A_ChromeRing_M_192px_200pct.png`
- `exports/chrome/125A_ChromeRing_M_288px_300pct.png`

#### Chrome Ring S
- `exports/chrome/125A_ChromeRing_S_64px_100pct.png`
- `exports/chrome/125A_ChromeRing_S_96px_150pct.png`
- `exports/chrome/125A_ChromeRing_S_128px_200pct.png`
- `exports/chrome/125A_ChromeRing_S_192px_300pct.png`

### SteelKnob integration contract

- ring asset contains only the chrome bezel;
- center hole and outside area remain transparent;
- SteelKnob remains authoritative for center, radius, pointer, value arc, ticks, hitbox and automation;
- ring bitmap is drawn into the exact computed bezel rectangle, never manually offset;
- consuming plugins choose S / M / L from the control's physical size;
- consuming plugins choose 100 / 150 / 200 / 300 % from absolute UI/content scale;
- do not upscale a lower-resolution ring when a matching higher-resolution export exists;
- all variants use the same optical center and chrome lighting direction;
- integration must not change parameter IDs, defaults, state, DSP or automation.

### Intended consumers

The family is designed to be reused by:
- High Gain Guitar Finisher V3
- Bass Finisher
- Ultimate Finisher
- MixEngine
- future 125A SteelKnob-based products
