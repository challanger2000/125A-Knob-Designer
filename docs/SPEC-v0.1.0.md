# 125A Knob Designer v0.1.0 — Technical Specification

## Architecture
The first version is deliberately renderer-first.

```
Application / CLI
      |
      v
Preset model
      |
      v
KnobRenderer
      |
      v
Frame bitmap
      |
      v
Vertical PNG filmstrip
```

The future editor must call the same renderer. The editor must not contain a second rendering implementation.

## Rendering model
A knob is composited from procedural layers:

1. transparent canvas
2. cast/drop shadow
3. outer bezel
4. body
5. body edge/rim
6. material highlight
7. optional center cap
8. rotating indicator
9. optional accent detail

The initial implementation uses native Windows GDI+ so that the proof-of-concept has no third-party runtime dependency.

## Animation
Default:
- Frames: 128
- Start angle: -135 degrees
- End angle: +135 degrees
- Frame order: minimum to maximum
- Strip orientation: vertical

For frame `i`:

```
t = i / (frameCount - 1)
angle = startAngle + t * (endAngle - startAngle)
```

## Output
Default knob cell:
- 128 x 128 px

Default filmstrip:
- width = cell size
- height = cell size * frame count
- pixel format = 32-bit ARGB PNG

## Quality requirements
- No visible clipping at maximum shadow extent.
- Indicator must remain readable at 100% and typical VSTGUI zoom values.
- Geometry must stay centered to sub-pixel-safe integer coordinates where possible.
- Filmstrip frame dimensions must be constant.
- First and last frames must exactly represent start/end angles.
- No embedded copyrighted third-party artwork.

## Built-in visual families

### Black Studio
Neutral professional black polymer body, restrained highlight, bright indicator.

### Gunmetal
Dark metallic rim and body with stronger edge definition.

### Brushed Aluminium
Light metallic body suitable for clean/mastering-style products.

### Industrial Steel
Heavier mechanical appearance with darker bezel and stronger body contrast.

### Minimal Dark
Reduced, flatter appearance for compact technical interfaces.

## Future editor
The editor will add:
- live preview
- material controls
- bezel/body/indicator editing
- frame count
- angle range
- size
- vertical/horizontal strip mode
- JSON preset save/load
- batch export S/M/L/XL
- switches, LEDs and fader caps after the knob workflow is stable
