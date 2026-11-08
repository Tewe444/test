# Reference project notes

The supplied `geometry_dash_advance-main.zip` is a Game Boy Advance demake. Its
Makefile targets `libgba`/`libtonc` and links GBA libraries, so its ROM output is
a `.gba`, not a Nintendo DS `.nds`.

Sky Dash follows the useful project organisation of the reference (separate
engine, menus, level data, audio, graphics and automated builds), but uses the
Nintendo DS ARM9 application model with `libnds`, devkitARM `ds_rules`, NitroFS
and Maxmod. The browser-based Web Dashers ZIP is not part of the native runtime.

The 20 Sky Dash layouts are original, editable data in `source/levels.c`. The
audio modules are copied unchanged from the Geometry Dash Advance archive that
the user provided, with the source archive's credit list preserved in
`GDA_AUDIO_CREDITS.md`. See the rights note in `README.md` before public release.

Current game mode: Cube. The S-Block marker is included as a non-lethal visual
object; its Stop effect needs a Dash Orb mechanic, which is not implemented yet.

Useful upstream build references:
- https://github.com/devkitPro/nds-examples/blob/master/templates/arm9/Makefile
- https://github.com/blocksds/mmutil
- https://blocksds.skylyrac.net/docs/libs/core_libs/
