# Code review and fixes in this update

- Replaced the three-level table with exactly 20 separate level definitions.
- Added a music module ID and displayed song title for every level.
- Added Maxmod initialisation, playback, pause, resume and stop handling, plus
  loaded WAV cues for level entry, level exit, crash and level completion.
- Added a NitroFS soundbank build step. The audio modules are not downloaded at
  runtime; they are embedded in the `.nds` at build time.
- Changed level selection wraparound from 3 to 20 entries and added L/R controls.
- Added song preview when browsing levels.
- Added an S-Block marker object to the data/rendering path. As the current Sky
  Dash prototype has only Cube physics and no dash-orb mode, this S-Block is
  visual/non-lethal only; it is not yet able to cancel dash-orb effects.
- Improved procedural backgrounds with a parallax skyline, additional star
  detail, cube face plate, and clearer obstacle highlights. Added an original
  4x2 editable sprite sheet for cube, spikes, block, S-block, orb, pad and portal.
- Kept the project offline: no Wi-Fi library, remote API, network calls or online
  level browser are included.

## Checks

- Verified that all 20 level rows refer to defined object arrays and generated
  Maxmod IDs present in the supplied audio folder.
- Verified that each configured music module exists in `assets/audio/`.
- Checked level bounds, unique level numbering and the source file structure.
- Ran GCC and Clang host-side `-fsyntax-only` checks with temporary API stubs;
  both passed with `-Wall -Wextra -Werror`. This checks C syntax, not target ABI
  or link compatibility.
- This environment does not have devkitPro/devkitARM or `mmutil` installed, so
  an actual `.nds` compile and hardware/emulator test could not be performed.
- GNU Make dry-run parsed the project and showed the generated soundbank command
  includes all 34 XM modules and all four WAV effects, plus the icon-conversion
  step. This was a dry-run with a stub `ds_rules`, not a target build.
- Compared all 38 bundled audio files against the supplied Geometry Dash Advance ZIP; all audio bytes match the originals (only the four WAV basenames were uppercased for Maxmod identifier consistency).
