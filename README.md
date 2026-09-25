# 125A Knob Designer

**125A Systems tool for designing and exporting high-quality VSTGUI knob filmstrips and reusable UI assets.**

The project is intentionally renderer-first: before building a large editor, the rendering core must prove that it can create professional-looking, deterministic controls suitable for real 125A plugins.

## v0.1.0 development target

- Windows x64
- Native C++17
- Procedural rendering — no copied third-party artwork
- Five built-in 125A knob families
- 128-frame vertical PNG filmstrip export
- Configurable cell size and frame count
- VSTGUI-oriented output
- Future editor will use the exact same rendering core

### Initial families

1. Black Studio
2. Gunmetal
3. Brushed Aluminium
4. Industrial Steel
5. Minimal Dark

## Build

Requirements:

- Windows 10/11
- Visual Studio 2022 with Desktop development with C++
- CMake 3.20+

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

## Run

Default export:

```powershell
.\build\Release\125A_Knob_Designer.exe
```

Optional arguments:

```text
125A_Knob_Designer.exe [output-directory] [cell-size] [frame-count]
```

Example:

```powershell
.\125A_Knob_Designer.exe exports 128 128
```

This creates five vertical PNG filmstrips in the selected directory.

## Development policy

Read [START-HERE.md](START-HERE.md) before changing the project.

JKnobMan may be studied as a workflow/reference application, but this project does **not** copy its source code, presets or artwork. Rendering and asset designs are implemented independently for 125A Systems.
