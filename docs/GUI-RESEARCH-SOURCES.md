# GUI / Knob / Filmstrip Research Sources

Research pool for 125A GUI Designer v0.2.0 and later.

Goal: collect useful public/open tools, codebases, asset libraries and UX concepts. Prefer permissive sources for implementation. Do not copy code/assets from unclear, GPL-only or non-commercial sources into distributable 125A code without a separate license review.

## Tier A — direct technical research / likely reusable concepts

### VybeCode StripKit
- Source: https://github.com/Vybecode-LTD/stripkit
- License: MIT.
- Useful concepts:
  - frame-perfect filmstrip generation
  - true-centre rotation / anti-wobble alignment
  - supersampling and Mitchell cubic downsampling
  - vertical/horizontal/grid sprite layouts
  - HiDPI exports @2x/@3x/@4x
  - layered knob body + rotating pointer
  - filmstrip import / re-slice / resample
  - batch processing with progress/cancel
  - parameter-law mapping (linear/skew/log)
  - render presets
  - meter, button/toggle and fader generation
  - skin manifest and loader-code generation
- 125A use:
  - strong QA/reference for our exporter and future batch pipeline
  - add centre/alignment diagnostics
  - add supersampled export and resampling
  - add importer and batch mode after basic designer is stable

### g200kg KnobMan3D
- Source: https://github.com/g200kg/knobman3d
- License: MIT. Sample knobs are CC0.
- Tech: WebGL + THREE.js.
- Useful concepts:
  - knob construction from basic 3D objects
  - animation effects
  - direct PNG filmstrip export
  - object combinations and knurling-like forms
  - LED/object examples
- 125A use:
  - inspect primitive-composition model and geometry presets
  - expand beyond concentric cylinders
  - use CC0 samples as visual/QA references if useful

### g200kg JKnobMan
- Source: https://github.com/g200kg/KnobMan
- License: MIT.
- Note: recovered source based on decompilation of JKnobMan 1.3.3.
- Useful concepts:
  - layer-based procedural graphics
  - mature parameter model
  - effect stacking and frame generation
  - legacy project compatibility ideas
- 125A use:
  - study layer/effect vocabulary and UX
  - do not reproduce the old UI complexity in Simple Mode

### g200kg WebKnobMan
- Source: https://github.com/g200kg/webknobman
- License: MIT for application code.
- Gallery assets have per-item licenses (PD/CC0/CC-BY/CC-BY-SA/CC-BY-NC/etc.).
- Useful concepts:
  - browser-based knob editor
  - gallery/preset metadata
  - APNG support
  - user asset licensing metadata
- 125A use:
  - preset/gallery architecture
  - explicit license metadata if we ever ship community presets/assets
  - never bulk-import gallery assets without checking each item

### rolandzwaga/vstgui-edit
- Source: https://github.com/rolandzwaga/vstgui-edit
- Root license inspected separately: MIT.
- Useful concepts already being used:
  - realtime THREE.js 3D preview
  - 3-layer knob model
  - metallic/matte/brushed materials
  - lighting azimuth/elevation/AO
  - dot/line/notch/groove indicators
  - filmstrip output
  - project persistence, bitmap types, WYSIWYG editing
- 125A use:
  - current main renderer research foundation

### g200kg stl-knob-designer
- Source: https://github.com/g200kg/stl-knob-designer
- License: MIT.
- Purpose: physical potentiometer knobs / STL generation.
- Useful concepts:
  - shaft types
  - cap/body separation
  - knurled/D-shaft geometry vocabulary
  - physically plausible knob proportions
- 125A use:
  - geometry reference for future hardware-like shapes
  - useful inspiration for stepped, cap, skirt, shaft and knurl forms even though we only render GUI assets

## Tier B — UX / complete plugin GUI design references

### Faceplate
- Source: https://github.com/AllTheMachines/Faceplate
- Public/free tool; license must be verified before any code reuse.
- Useful concepts:
  - drag/drop visual plugin GUI editor
  - 60+ controls
  - realtime preview
  - SVG import + automatic layer detection
  - multi-window layouts
  - export of working JUCE WebView2 bundles
- 125A use:
  - excellent UX reference for future panel/layout designer
  - investigate component palette, property inspector, SVG import and code-export workflow
  - do not copy source until license is verified

### HISE Interface Designer + UI snippets
- Docs: https://docs.hise.dev/ui-components/
- Open-source ecosystem, but source licensing requires care; use primarily as UX/reference unless separately reviewed.
- Useful concepts:
  - WYSIWYG canvas + component list + property editor
  - edit/play modes
  - component hierarchy / z-order
  - filmstrip-based controls
  - CSS/look-and-feel styling
  - extensive UI snippet library including meters, knobs, filmstrip buttons and skeuomorphic examples
- 125A use:
  - reference for future full plugin-layout mode
  - edit/play toggle
  - component hierarchy and grouping
  - visual preview of interactive states

### AudioUI by Cutoff
- Source: https://github.com/cutoff/audio-ui
- License: GPL-3.0 / commercial dual license.
- Useful concepts:
  - professional audio-focused control APIs
  - knob/slider/button/cycle button
  - image-based and filmstrip controls
  - performance-focused interaction design
- 125A use:
  - API/interaction reference only unless license route changes
  - useful for control-state semantics and input behaviour

### g200kg input-knobs
- Source: https://github.com/g200kg/input-knobs
- License: MIT for library code; sample images may have separate licenses.
- Useful concepts:
  - knob/slider/switch control semantics
  - image-filmstrip fallback behaviour
  - touch input
  - sprite count conventions
- 125A use:
  - preview/runtime test harness ideas
  - import validation for frame counts and orientation

## Tier C — asset libraries / visual references / processing ideas

### Leslie Sanford KnobMan library
- Source: https://www.lesliesanford.com/vst/knobman/knobs.php
- Useful for:
  - downloadable KnobMan project examples
  - reverse-engineering how complex looks are built from simple layers/effects
  - 3D, metallic and stylised examples
- 125A use:
  - visual/preset research
  - verify individual usage terms before redistributing anything

### Audio UI Components Library / NAM UI assets fork
- Source: https://github.com/RustoMCSpit/Audio-UI-Components-Library
- License: CC BY 4.0.
- Useful for:
  - amplifier-style UI asset organization
  - standardized storage ideas
  - source/project asset concept
- 125A use:
  - asset-library folder conventions
  - attribution-aware import/catalog ideas

### Synaptik UI Toolkit
- Source: https://github.com/joshband/SynaptikUIToolkit
- License: verify before reuse.
- Useful concepts:
  - 156 categorized UI elements
  - 64/128/256/512 multi-size variants
  - transparent PNG extraction pipeline
  - interactive HTML asset catalog
  - automated ImageMagick processing
  - theme-based organization
  - catalog generation and asset auditing scripts
- 125A use:
  - future asset library/catalog
  - automated multi-size processing
  - theme/preset packaging
  - asset QA/audit workflow

## Feature candidates extracted from the research

### High priority for v0.2.x
1. Supersampled PNG render with high-quality downsampling.
2. True-centre / wobble diagnostic for rotating controls.
3. Filmstrip import with automatic frame/orientation detection.
4. Vertical, horizontal and sprite-grid export.
5. HiDPI export presets 1x/1.5x/2x/3x/4x.
6. Batch export from one project/preset.
7. Layered body + independently rotating pointer.
8. Better geometry presets: cap, skirt, stepped, knurled/grooved, tapered.
9. Preview scrub + frame-step + playback.
10. Export QA: dimensions, alpha, first/last frame, frame count, sweep.

### Medium priority
1. Fader/slider builder.
2. Meter builder.
3. Button/toggle multi-state builder.
4. SVG import and layer extraction.
5. Existing strip re-slice/resample.
6. Preset browser/catalog with thumbnails.
7. Theme packs.
8. Parameter-law frame mapping.
9. Asset metadata and per-item license fields.
10. Generated loader snippets / VSTGUI integration notes.

### Later / full GUI designer
1. Canvas-based plugin layout editor.
2. Component palette.
3. Hierarchy/z-order/grouping.
4. Grid, snap, guides, rulers.
5. Edit/preview modes.
6. Multi-window/pages.
7. Panel/background/frame designer.
8. Reusable global styles/themes.
9. Asset catalog and search.
10. Exportable project package.

## License rule for 125A

Safe default:
- MIT / BSD / Apache / CC0: eligible for direct technical reuse after preserving notices and checking dependencies/assets.
- CC-BY: assets can be used with proper attribution, but avoid making them default 125A-owned art.
- GPL / AGPL / unclear / non-commercial: research and independent reimplementation only unless a deliberate licensing decision is made.
- Every imported preset/gallery asset must keep its own provenance and license metadata.

## Research principle

We are not trying to clone one existing program. The goal is to combine the best ideas into a simpler 125A workflow:

**Form → Material → Light → Pointer → Details → Size → Export**

Technical controls remain available in Expert Mode and through the shared project/CLI model.
