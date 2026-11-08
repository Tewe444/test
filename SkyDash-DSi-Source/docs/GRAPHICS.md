# Sky Dash graphics assets

All artwork in this project is original Sky Dash artwork, not copied from the
Geometry Dash Advance or Web Dashers archives.

- `data/icon.png`: 128x128 launcher artwork.
- `data/icon.bmp`: 32x32 banner icon input for the devkitPro `grit` conversion.
- `assets/graphics/sky_dash_spritesheet.png`: transparent 4x2 sprite sheet with
  32x32 tiles, ordered left-to-right and top-to-bottom:

  1. Sky cube
  2. Single spike
  3. Block
  4. S-block marker
  5. Green orb concept
  6. Jump-pad concept
  7. Portal concept
  8. Double spike

The current DS renderer draws the in-game cube, spikes, blocks, S-block and neon
background procedurally in `source/render.c`; the sprite sheet is included as
editable source art for future tile/sprite rendering and additional game modes.
