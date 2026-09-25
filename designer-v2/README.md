# 125A GUI Designer v0.2.0

This directory is the new foundation for a beginner-friendly 125A GUI asset builder.

## Product goals

- Simple Mode first: form, material, light, pointer, size, export.
- Expert Mode later: raw geometry/material/light/output parameters.
- One shared project format for both the Windows UI and automation/chat.
- VSTGUI-ready PNG filmstrip export.
- Renderer and UI are separate: the same project must render identically from UI and CLI.

## Current foundation

The first milestone establishes the shared `.125agui` project model and a dependency-free CLI validator/editor.

Examples:

```powershell
node designer-v2/cli/125a-gui.js validate designer-v2/examples/knob-dark-metal.125agui.json
node designer-v2/cli/125a-gui.js set designer-v2/examples/knob-dark-metal.125agui.json lighting.preset dramatic
node designer-v2/cli/125a-gui.js preset designer-v2/examples/knob-dark-metal.125agui.json studio-dark
```

The CLI writes to a new file unless `--in-place` is supplied.

## Next milestone

Integrate the MIT-licensed rendering core concepts from vstgui-edit behind this project model, then build the Simple Mode UI and Windows desktop wrapper.
