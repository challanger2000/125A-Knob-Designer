# Shared project / renderer adapter

This module is the boundary between the beginner-facing 125A project model and the renderer model derived from the MIT-licensed vstgui-edit knob designer.

The Simple Mode never needs to expose renderer jargon. It selects a shape, material and lighting preset; this adapter expands those choices into the lower-level layer geometry, material and lighting fields expected by the 3D renderer.

Expert overrides are intentionally nested under `design.expert` and `lighting.expert`, so future expert UI and chat automation can access them without complicating the beginner workflow.

The next renderer-integration step can consume `toRendererDesign(project)` directly.
