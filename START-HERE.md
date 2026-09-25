# 125A Knob Designer — START HERE

## Purpose
Create a small Windows desktop tool for designing and exporting high-quality rotary-control filmstrips for VSTGUI-based 125A Systems plugins.

## Non-goals
- Do not copy JKnobMan source code.
- Do not copy JKnobMan example artwork or presets.
- Do not tie the renderer to a specific plugin.
- Do not modify unrelated 125A repositories from this repository.

## Development rules
1. `main` is release/stable only.
2. Active development happens on version branches such as `v0.1.0`.
3. Renderer quality comes before editor complexity.
4. Every visual asset must be reproducible from source parameters.
5. Export must remain compatible with VSTGUI filmstrip workflows.
6. Prefer deterministic rendering and minimal external dependencies.
7. GitHub Actions should only be used when they provide real QA value.

## v0.1.0 target
- Native Windows x64 executable
- 5 built-in 125A knob families
- 128-frame vertical PNG filmstrip export
- Configurable size and rotation range
- Alpha-capable PNG output
- Clear separation between rendering core and future editor UI

## Initial knob families
- Black Studio
- Gunmetal
- Brushed Aluminium
- Industrial Steel
- Minimal Dark

## Next milestone
After the renderer is visually accepted, add the interactive editor, preset persistence, batch sizing, and reusable UI-asset generation.
